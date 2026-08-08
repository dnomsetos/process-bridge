#include <log.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>

static log_level_t g_min_level = LOG_LEVEL_INFO;
static int g_initialized = 0;

static const char *level_name(log_level_t level) {
  switch (level) {
    case LOG_LEVEL_DEBUG:
      return "DEBUG";
    case LOG_LEVEL_INFO:
      return "INFO";
    case LOG_LEVEL_WARN:
      return "WARN";
    case LOG_LEVEL_ERROR:
      return "ERROR";
    default:
      return "?????";
  }
}

static int parse_level(const char *name, log_level_t *out) {
  if (name == NULL) {
    return -1;
  }

  if (strcasecmp(name, "DEBUG") == 0) {
    *out = LOG_LEVEL_DEBUG;
  } else if (strcasecmp(name, "INFO") == 0) {
    *out = LOG_LEVEL_INFO;
  } else if (strcasecmp(name, "WARN") == 0 ||
             strcasecmp(name, "WARNING") == 0) {
    *out = LOG_LEVEL_WARN;
  } else if (strcasecmp(name, "ERROR") == 0) {
    *out = LOG_LEVEL_ERROR;
  } else {
    return -1;
  }

  return 0;
}

void log_init(void) {
  if (g_initialized) {
    return;
  }

  g_initialized = 1;

  const char *env = getenv("PROCESS_BRIDGE_LOG_LEVEL");
  log_level_t level;

  if (env != NULL && parse_level(env, &level) == 0) {
    g_min_level = level;
  } else if (env != NULL) {
    fprintf(stderr,
            "log: unknown PROCESS_BRIDGE_LOG_LEVEL=\"%s\", expected "
            "DEBUG/INFO/WARN/ERROR, defaulting to INFO\n",
            env);
  }
}

void log_set_level(log_level_t level) {
  g_initialized = 1;
  g_min_level = level;
}

void log_log(log_level_t level, const char *file, int line, const char *fmt,
             ...) {
  if (!g_initialized) {
    log_init();
  }

  if (level < g_min_level) {
    return;
  }

  time_t now = time(NULL);
  struct tm tm_buf;
  struct tm *tm_info = localtime_r(&now, &tm_buf);

  char time_buf[16] = "??:??:??";
  if (tm_info != NULL) {
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);
  }

  const char *basename = strrchr(file, '/');
  basename = (basename != NULL) ? basename + 1 : file;

  fprintf(stderr, "%s [%-5s] %s:%d: ", time_buf, level_name(level), basename,
          line);

  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  fprintf(stderr, "\n");
}
