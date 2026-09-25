#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#include <linux/breakpoint.h>

#include "ptrace_probe.h"
#include "test_util.h"

#ifndef PB_TEST_TARGET_PATH
#error "PB_TEST_TARGET_PATH must be defined by the build system"
#endif

static int find_symbol_address(const char *binary_path, const char *symbol,
                               uintptr_t *out_addr) {
  char cmd[1024];
  int written = snprintf(cmd, sizeof(cmd), "nm '%s' 2>/dev/null | grep ' %s$'",
                         binary_path, symbol);
  if (written < 0 || (size_t)written >= sizeof(cmd)) {
    return 0;
  }

  FILE *p = popen(cmd, "r");
  if (p == NULL) {
    return 0;
  }

  char line[512];
  int found = 0;
  if (fgets(line, sizeof(line), p) != NULL) {
    char *end = NULL;
    unsigned long long addr = strtoull(line, &end, 16);
    if (end != line) {
      *out_addr = (uintptr_t)addr;
      found = 1;
    }
  }

  pclose(p);
  return found;
}

TEST(fork_child_stops_at_traceme) {
  if (!ptrace_traceme_is_usable()) {
    SKIP_RETURN("ptrace() is not usable in this environment");
  }

  char *argv[] = {(char *)PB_TEST_TARGET_PATH, NULL};
  extern char **environ;

  pid_t pid = fork_child(PB_TEST_TARGET_PATH, argv, environ);
  ASSERT_TRUE(pid > 0);

  ASSERT_TRUE(kill(pid, SIGKILL) == 0 || errno == ESRCH);
  waitpid(pid, NULL, 0);
}

TEST(breakpoint_stops_execution_exactly_at_target_and_restores_byte) {
  if (!ptrace_traceme_is_usable()) {
    SKIP_RETURN("ptrace() is not usable in this environment");
  }

  uintptr_t target_addr = 0;
  if (!find_symbol_address(PB_TEST_TARGET_PATH, "target_func", &target_addr)) {
    SKIP_RETURN("could not resolve target_func address via nm");
  }

  char *argv[] = {(char *)PB_TEST_TARGET_PATH, NULL};
  extern char **environ;

  pid_t pid = fork_child(PB_TEST_TARGET_PATH, argv, environ);
  ASSERT_TRUE(pid > 0);

  breakpoint_t bp;
  create_breakpoint(&bp, pid, (void *)target_addr);

  errno = 0;
  long word = ptrace(PTRACE_PEEKTEXT, pid, (void *)target_addr, NULL);
  ASSERT_TRUE(!(word == -1 && errno != 0));
  ASSERT_EQ_INT(word & 0xFF, 0xCC);

  execute_up_to_breakpoint(&bp);

  errno = 0;
  long restored_word = ptrace(PTRACE_PEEKTEXT, pid, (void *)target_addr, NULL);
  ASSERT_TRUE(!(restored_word == -1 && errno != 0));
  ASSERT_EQ_INT(restored_word & 0xFF, bp.old_word & 0xFF);

  struct user_regs_struct regs;
  ASSERT_TRUE(ptrace(PTRACE_GETREGS, pid, NULL, &regs) == 0);
#if defined(__x86_64__)
  ASSERT_EQ_UINT64(regs.rip, (uint64_t)target_addr);
#elif defined(__i386__)
  ASSERT_EQ_UINT64((uint32_t)regs.eip, (uint32_t)target_addr);
#endif

  kill(pid, SIGKILL);
  waitpid(pid, NULL, 0);
}

TEST_MAIN()
