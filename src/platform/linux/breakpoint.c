#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>

#include <platform/linux/breakpoint.h>

#define PLACE_INT3_IN_FIRST_BYTE(word) ((word) & ~0xFFULL) | 0xCC

int fork_child(const char *path, char *const *argv, char *const *envp) {
  pid_t pid = fork();

  if (pid < 0) {
    perror("fork");
    _exit(EXIT_FAILURE);
  } else if (pid == 0) {
    long ptrace_status = ptrace(PTRACE_TRACEME, 0, NULL, NULL);

    if (ptrace_status < 0) {
      perror("ptrace");
      exit(EXIT_FAILURE);
    }

    execve(path, argv, envp);

    perror("execve");
    exit(EXIT_FAILURE);
  } else {
    int status = 0;

    if (waitpid(pid, &status, 0) == -1) {
      perror("waitpid");
      exit(EXIT_FAILURE);
    }

    if (WIFEXITED(status)) {
      fprintf(stderr, "Child exited with %d\n", WEXITSTATUS(status));
      exit(EXIT_FAILURE);
    }

    if (WIFSIGNALED(status)) {
      fprintf(stderr, "Child killed by signal %d\n", WTERMSIG(status));
      exit(EXIT_FAILURE);
    }

    if (!WIFSTOPPED(status)) {
      fprintf(stderr, "Unexpected wait status\n");
      exit(EXIT_FAILURE);
    }
  }

  return pid;
}

void create_breakpoint(breakpoint_t *breakpoint, pid_t pid, void *hook_addr) {
  breakpoint->pid = pid;
  breakpoint->hook_addr = hook_addr;

  long peek_result = ptrace(PTRACE_PEEKTEXT, pid, hook_addr, NULL);

  if (peek_result == -1 && errno != 0) {
    perror("ptrace");
    exit(EXIT_FAILURE);
  }

  uint64_t word = (uint64_t)peek_result;
  breakpoint->old_word = word;

  word = PLACE_INT3_IN_FIRST_BYTE(word);

  if (ptrace(PTRACE_POKETEXT, pid, hook_addr, word) == -1) {
    perror("ptrace");
    exit(EXIT_FAILURE);
  }
}

void execute_up_to_breakpoint(breakpoint_t *breakpoint) {
  struct user_regs_struct regs;

  if (ptrace(PTRACE_GETREGS, breakpoint->pid, NULL, &regs) == -1) {
    perror("ptrace");
    exit(EXIT_FAILURE);
  }

  if (regs.rip == (uintptr_t)breakpoint->hook_addr + 1) {
    if (ptrace(PTRACE_POKETEXT, breakpoint->pid, breakpoint->hook_addr, breakpoint->old_word) ==
        -1) {
      perror("ptrace");
      exit(EXIT_FAILURE);
    }

    --regs.rip;

    if (ptrace(PTRACE_SETREGS, breakpoint->pid, NULL, &regs) == -1) {
      perror("ptrace");
      exit(EXIT_FAILURE);
    }

    if (ptrace(PTRACE_SINGLESTEP, breakpoint->pid, NULL, NULL) == -1) {
      perror("ptrace");
      exit(EXIT_FAILURE);
    }

    int status = 0;

    if (waitpid(breakpoint->pid, &status, 0) == -1) {
      perror("waitpid");
      exit(EXIT_FAILURE);
    }

    if (WIFEXITED(status) || WIFSIGNALED(status)) {
      printf("Success after singlestep!\n");
      exit(EXIT_SUCCESS);
    }

    if (ptrace(PTRACE_POKETEXT, breakpoint->pid, breakpoint->hook_addr,
               PLACE_INT3_IN_FIRST_BYTE(breakpoint->old_word)) == -1) {
      perror("ptrace");
      exit(EXIT_FAILURE);
    }
  }

  if (ptrace(PTRACE_CONT, breakpoint->pid, NULL, NULL) == -1) {
    perror("ptrace");
    exit(EXIT_FAILURE);
  }

  int status = 0;

  if (waitpid(breakpoint->pid, &status, 0) == -1) {
    perror("waitpid");
    exit(EXIT_FAILURE);
  }

  if (WIFEXITED(status) || WIFSIGNALED(status)) {
    printf("Success continuing!\n");
    exit(EXIT_SUCCESS);
  }
}
