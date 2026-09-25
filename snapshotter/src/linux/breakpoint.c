#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>

#include <linux/breakpoint.h>
#include <log.h>

#define PLACE_INT3_IN_FIRST_BYTE(word) ((word) & ~0xFFULL) | 0xCC

int fork_child(const char *path, char *const *argv, char *const *envp) {
  pid_t pid = fork();

  if (pid < 0) {
    LOG_FATAL_ERRNO("fork failed");
  } else if (pid == 0) {
    long ptrace_status = ptrace(PTRACE_TRACEME, 0, NULL, NULL);

    if (ptrace_status < 0) {
      LOG_FATAL_ERRNO("ptrace(PTRACE_TRACEME) failed");
    }

    LOG_INFO("child: exec'ing '%s'", path);
    execve(path, argv, envp);

    LOG_FATAL_ERRNO("execve('%s') failed", path);
  } else {
    int status = 0;

    if (waitpid(pid, &status, 0) == -1) {
      LOG_FATAL_ERRNO("waitpid(%d) failed", pid);
    }

    if (WIFEXITED(status)) {
      LOG_FATAL(
          "child %d exited with status %d before reaching the "
          "traceme stop",
          pid, WEXITSTATUS(status));
    }

    if (WIFSIGNALED(status)) {
      LOG_FATAL(
          "child %d was killed by signal %d before reaching the "
          "traceme stop",
          pid, WTERMSIG(status));
    }

    if (!WIFSTOPPED(status)) {
      LOG_FATAL("child %d: unexpected wait status %d", pid, status);
    }

    LOG_DEBUG("child %d stopped after PTRACE_TRACEME + execve", pid);
  }

  return pid;
}

void create_breakpoint(breakpoint_t *breakpoint, pid_t pid, void *hook_addr) {
  breakpoint->pid = pid;
  breakpoint->hook_addr = hook_addr;

  errno = 0;
  long peek_result = ptrace(PTRACE_PEEKTEXT, pid, hook_addr, NULL);

  if (peek_result == -1 && errno != 0) {
    LOG_FATAL_ERRNO("ptrace(PTRACE_PEEKTEXT, pid=%d, addr=%p) failed", pid,
                    hook_addr);
  }

  uint64_t word = (uint64_t)peek_result;
  breakpoint->old_word = word;

  word = PLACE_INT3_IN_FIRST_BYTE(word);

  if (ptrace(PTRACE_POKETEXT, pid, hook_addr, word) == -1) {
    LOG_FATAL_ERRNO(
        "ptrace(PTRACE_POKETEXT, pid=%d, addr=%p) failed while "
        "arming breakpoint",
        pid, hook_addr);
  }

  LOG_INFO("breakpoint armed at %p (pid %d)", hook_addr, pid);
}

void execute_up_to_breakpoint(breakpoint_t *breakpoint) {
  struct user_regs_struct regs;

  if (ptrace(PTRACE_GETREGS, breakpoint->pid, NULL, &regs) == -1) {
    LOG_FATAL_ERRNO("ptrace(PTRACE_GETREGS, pid=%d) failed", breakpoint->pid);
  }

#ifdef __x86_64__
  unsigned long long int ip = regs.rip;
#elif defined(__i386__)
  long int ip = regs.eip;
#endif

  if (ip == (uintptr_t)breakpoint->hook_addr) {
    LOG_DEBUG("pid %d already at breakpoint %p, single-stepping past it",
              breakpoint->pid, breakpoint->hook_addr);

    if (ptrace(PTRACE_SINGLESTEP, breakpoint->pid, NULL, NULL) == -1) {
      LOG_FATAL_ERRNO("ptrace(PTRACE_SINGLESTEP, pid=%d) failed",
                      breakpoint->pid);
    }

    int status = 0;

    if (waitpid(breakpoint->pid, &status, 0) == -1) {
      LOG_FATAL_ERRNO("waitpid(%d) failed", breakpoint->pid);
    }

    if (WIFEXITED(status) || WIFSIGNALED(status)) {
      LOG_INFO("pid %d exited during single-step past the breakpoint",
               breakpoint->pid);
      exit(EXIT_SUCCESS);
    }

    if (ptrace(PTRACE_POKETEXT, breakpoint->pid, breakpoint->hook_addr,
               breakpoint->old_word) == -1) {
      LOG_FATAL_ERRNO(
          "ptrace(PTRACE_POKETEXT, pid=%d, addr=%p) failed "
          "while restoring the original instruction",
          breakpoint->pid, breakpoint->hook_addr);
    }
  }

  if (ptrace(PTRACE_CONT, breakpoint->pid, NULL, NULL) == -1) {
    LOG_FATAL_ERRNO("ptrace(PTRACE_CONT, pid=%d) failed", breakpoint->pid);
  }

  int status = 0;

  if (waitpid(breakpoint->pid, &status, 0) == -1) {
    LOG_FATAL_ERRNO("waitpid(%d) failed", breakpoint->pid);
  }

  if (WIFEXITED(status) || WIFSIGNALED(status)) {
    LOG_INFO("pid %d exited before reaching the breakpoint", breakpoint->pid);
    exit(EXIT_SUCCESS);
  }

  if (ptrace(PTRACE_GETREGS, breakpoint->pid, NULL, &regs) == -1) {
    LOG_FATAL_ERRNO("ptrace(PTRACE_GETREGS, pid=%d) failed", breakpoint->pid);
  }

#ifdef __x86_64__
  unsigned long long int *pip = &regs.rip;
#elif defined(__i386__)
  long int *pip = &regs.eip;
#endif

  if (*pip != (uintptr_t)breakpoint->hook_addr + 1) {
    LOG_WARN("pid %d stopped at an unexpected address: got %" PRIxPTR
             ", expected %" PRIxPTR
             " (int3 landing address); the trap may "
             "not be the one we armed",
             breakpoint->pid, (uintptr_t)*pip,
             (uintptr_t)breakpoint->hook_addr + 1);
  }

  --(*pip);

  if (ptrace(PTRACE_SETREGS, breakpoint->pid, NULL, &regs) == -1) {
    LOG_FATAL_ERRNO("ptrace(PTRACE_SETREGS, pid=%d) failed", breakpoint->pid);
  }

  if (ptrace(PTRACE_POKETEXT, breakpoint->pid, breakpoint->hook_addr,
             breakpoint->old_word) == -1) {
    LOG_FATAL_ERRNO(
        "ptrace(PTRACE_POKETEXT, pid=%d, addr=%p) failed while "
        "restoring the original instruction",
        breakpoint->pid, breakpoint->hook_addr);
  }

  LOG_INFO("pid %d reached breakpoint %p and was rewound to it",
           breakpoint->pid, breakpoint->hook_addr);
}
