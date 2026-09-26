#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

typedef void (*fatal_probe_fn)(void *arg);

static int expect_fatal_exit(fatal_probe_fn fn, void *arg) {
  pid_t pid = fork();

  if (pid < 0) {
    return 0;
  }

  if (pid == 0) {
    freopen("/dev/null", "w", stderr);
    fn(arg);
    _exit(EXIT_SUCCESS);
  }

  int status = 0;
  if (waitpid(pid, &status, 0) != pid) {
    return 0;
  }

  return WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE;
}

typedef int (*isolated_check_fn)(void);

static int run_isolated(isolated_check_fn fn) {
  pid_t pid = fork();

  if (pid < 0) {
    return 0;
  }

  if (pid == 0) {
    freopen("/dev/null", "w", stderr);
    int rc = fn();
    _exit(rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
  }

  int status = 0;
  if (waitpid(pid, &status, 0) != pid) {
    return 0;
  }

  return WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS;
}

#define CHECK_TRUE(cond)                            \
  do {                                              \
    if (!(cond)) {                                  \
      fprintf(stderr,                               \
              "  CHECK_TRUE(%s) failed at %s:%d\n", \
              #cond,                                \
              __FILE__,                             \
              __LINE__);                            \
      return 1;                                     \
    }                                               \
  } while (0)

#define CHECK_EQ_UINT64(actual, expected)                               \
  do {                                                                  \
    unsigned long long _a = (unsigned long long)(actual);               \
    unsigned long long _e = (unsigned long long)(expected);             \
    if (_a != _e) {                                                     \
      fprintf(stderr,                                                   \
              "  CHECK_EQ_UINT64(%s, %s) failed: got 0x%llx, expected " \
              "0x%llx at %s:%d\n",                                      \
              #actual,                                                  \
              #expected,                                                \
              _a,                                                       \
              _e,                                                       \
              __FILE__,                                                 \
              __LINE__);                                                \
      return 1;                                                         \
    }                                                                   \
  } while (0)
