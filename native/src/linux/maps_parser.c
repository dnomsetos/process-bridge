#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <linux/maps_parser.h>
#include <log.h>

void parse_maps_str(const char *maps_str, maps_entry_t *entry) {
  char perms[5] = {0};

  entry->path[0] = '\0';

  int count = sscanf(
      maps_str, "%" SCNx64 "-%" SCNx64 " %4s %" SCNx64 " %*s %*s %4095[^\n]",
      &entry->start, &entry->end, perms, &entry->file_offset, entry->path);

  if (count < 4) {
    LOG_FATAL("failed to parse /proc/<pid>/maps line: \"%s\"", maps_str);
  }

  if (count == 4) {
    entry->path[0] = '\0';
  }

  entry->read = perms[0] == 'r';
  entry->write = perms[1] == 'w';
  entry->exec = perms[2] == 'x';
  entry->shared = perms[3] == 's';

  LOG_DEBUG("maps entry: 0x%" PRIx64 "-0x%" PRIx64 " %s offset=0x%" PRIx64
            " path=\"%s\"",
            entry->start, entry->end, perms, entry->file_offset, entry->path);
}

void create_maps_iter(maps_iter_t *iter, pid_t pid) {
  char buffer[PROC_BUF_SIZE];

  int written = snprintf(buffer, PROC_BUF_SIZE, "/proc/%d/maps", pid);
  if (written < 0 || (size_t)written >= PROC_BUF_SIZE) {
    LOG_FATAL("pid %d doesn't fit in the /proc/<pid>/maps path buffer", pid);
  }

  iter->file = fopen(buffer, "rb");

  if (iter->file == NULL) {
    LOG_FATAL_ERRNO("fopen('%s') failed", buffer);
  }

  iter->line = NULL;
}

const maps_entry_t *next(maps_iter_t *iter) {
  size_t line_size = 0;
  if (iter->line != NULL) {
    free(iter->line);
    iter->line = NULL;
  }

  if (getline(&iter->line, &line_size, iter->file) == -1) {
    if (ferror(iter->file)) {
      LOG_FATAL_ERRNO("getline() on /proc/<pid>/maps failed");
    }

    if (fclose(iter->file) != 0) {
      LOG_FATAL_ERRNO("fclose() on /proc/<pid>/maps failed");
    }

    free(iter->line);
    iter->line = NULL;

    return NULL;
  }

  parse_maps_str(iter->line, &iter->entry);

  return &iter->entry;
}
