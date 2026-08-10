#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PB_TEST_MAX_CASES 256

typedef void (*pb_test_fn)(void);

typedef struct {
  const char *name;
  pb_test_fn fn;
} pb_test_case_t;

static pb_test_case_t pb_test_cases[PB_TEST_MAX_CASES];
static int pb_test_case_count = 0;
static int pb_test_failures = 0;
static const char *pb_test_current_name = NULL;

typedef struct {
  const char *name;
  pb_test_fn fn;
} pb_test_registrar_t;

#define TEST(test_name)                                                    \
  static void test_name(void);                                             \
  __attribute__((constructor)) static void pb_register_##test_name(void) { \
    if (pb_test_case_count >= PB_TEST_MAX_CASES) {                         \
      fprintf(stderr, "too many tests, bump PB_TEST_MAX_CASES\n");         \
      exit(EXIT_FAILURE);                                                  \
    }                                                                      \
    pb_test_cases[pb_test_case_count].name = #test_name;                   \
    pb_test_cases[pb_test_case_count].fn = test_name;                      \
    pb_test_case_count++;                                                  \
  }                                                                        \
  static void test_name(void)

#define PB_FAIL_RETURN(...)                                \
  do {                                                     \
    fprintf(stderr, "  FAIL %s:%d: ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__);                          \
    fprintf(stderr, "\n");                                 \
    pb_test_failures++;                                    \
    return;                                                \
  } while (0)

#define ASSERT_TRUE(cond)                              \
  do {                                                 \
    if (!(cond)) {                                     \
      PB_FAIL_RETURN("ASSERT_TRUE(%s) failed", #cond); \
    }                                                  \
  } while (0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ_INT(actual, expected)                                       \
  do {                                                                        \
    long long _a = (long long)(actual);                                       \
    long long _e = (long long)(expected);                                     \
    if (_a != _e) {                                                           \
      PB_FAIL_RETURN("ASSERT_EQ_INT(%s, %s) failed: got %lld, expected %lld", \
                     #actual, #expected, _a, _e);                             \
    }                                                                         \
  } while (0)

#define ASSERT_EQ_UINT64(actual, expected)                                \
  do {                                                                    \
    unsigned long long _a = (unsigned long long)(actual);                 \
    unsigned long long _e = (unsigned long long)(expected);               \
    if (_a != _e) {                                                       \
      PB_FAIL_RETURN(                                                     \
          "ASSERT_EQ_UINT64(%s, %s) failed: got 0x%llx, expected 0x%llx", \
          #actual, #expected, _a, _e);                                    \
    }                                                                     \
  } while (0)

#define ASSERT_EQ_STR(actual, expected)                                \
  do {                                                                 \
    const char *_a = (actual);                                         \
    const char *_e = (expected);                                       \
    if (strcmp(_a, _e) != 0) {                                         \
      PB_FAIL_RETURN(                                                  \
          "ASSERT_EQ_STR(%s, %s) failed: got \"%s\", expected \"%s\"", \
          #actual, #expected, _a, _e);                                 \
    }                                                                  \
  } while (0)

#define SKIP_RETURN(...)                                  \
  do {                                                    \
    fprintf(stderr, "  SKIP %s: ", pb_test_current_name); \
    fprintf(stderr, __VA_ARGS__);                         \
    fprintf(stderr, "\n");                                \
    return;                                               \
  } while (0)

#define TEST_MAIN()                                                          \
  int main(void) {                                                           \
    int total = pb_test_case_count;                                          \
    for (int i = 0; i < total; ++i) {                                        \
      pb_test_current_name = pb_test_cases[i].name;                          \
      int failures_before = pb_test_failures;                                \
      fprintf(stderr, "RUN  %s\n", pb_test_cases[i].name);                   \
      pb_test_cases[i].fn();                                                 \
      if (pb_test_failures == failures_before) {                             \
        fprintf(stderr, "OK   %s\n", pb_test_cases[i].name);                 \
      }                                                                      \
    }                                                                        \
    fprintf(stderr, "---\n%d test case(s), %d failed assertion(s)\n", total, \
            pb_test_failures);                                               \
    return pb_test_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;              \
  }
