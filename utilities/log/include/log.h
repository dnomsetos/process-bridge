#pragma once

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  LOG_LEVEL_DEBUG = 0,
  LOG_LEVEL_INFO = 1,
  LOG_LEVEL_WARN = 2,
  LOG_LEVEL_ERROR = 3,
} log_level_t;

void log_init(void);

void log_set_level(log_level_t level);

void log_log(log_level_t level, const char *file, int line, const char *fmt,
             ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 4, 5)))
#endif
    ;

#define LOG_DEBUG(...) log_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) log_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) log_log(LOG_LEVEL_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) log_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#define LOG_ERRNO(level, ...)                                      \
  do {                                                             \
    char _log_errno_msg[512];                                      \
    int _log_errno_saved = errno;                                  \
    snprintf(_log_errno_msg, sizeof(_log_errno_msg), __VA_ARGS__); \
    log_log((level), __FILE__, __LINE__, "%s: %s", _log_errno_msg, \
            strerror(_log_errno_saved));                           \
  } while (0)

#define LOG_FATAL(...)      \
  do {                      \
    LOG_ERROR(__VA_ARGS__); \
    exit(EXIT_FAILURE);     \
  } while (0)

#define LOG_FATAL_ERRNO(...)                 \
  do {                                       \
    LOG_ERRNO(LOG_LEVEL_ERROR, __VA_ARGS__); \
    exit(EXIT_FAILURE);                      \
  } while (0)
