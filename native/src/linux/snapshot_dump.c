#include <stdio.h>
#include <stdlib.h>

#include <linux/mappings_reader.h>
#include <linux/snapshot_dump.h>
#include <log.h>

void dump_snapshot(snapshot_info_t *snapshot, pid_t pid, const char *filename) {
  FILE *file = fopen(filename, "wb");
  if (file == NULL) {
    LOG_FATAL_ERRNO("fopen('%s') for writing failed", filename);
  }

  if (fwrite(snapshot, sizeof(*snapshot), 1, file) != 1) {
    LOG_FATAL_ERRNO("fwrite of the snapshot header to '%s' failed", filename);
  }

  dump_mappings_content(pid, file);

  if (fclose(file) != 0) {
    LOG_FATAL_ERRNO("fclose('%s') failed", filename);
  }

  LOG_INFO("snapshot for pid %d written to '%s'", pid, filename);
}
