#include <stdio.h>
#include <stdlib.h>

#include <linux/mappings_reader.h>
#include <linux/snapshot_dump.h>

void dump_snapshot(snapshot_info_t *snapshot, pid_t pid, const char *filename) {
  FILE *file = fopen(filename, "wb");
  if (file == NULL) {
    printf("here1 \n");
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  if (fwrite(snapshot, sizeof(*snapshot), 1, file) != 1) {
    printf("here2\n");
    perror("fwrite");
    exit(EXIT_FAILURE);
  }

  dump_mappings_content(pid, file);

  if (fclose(file) != 0) {
    perror("fclose");
    exit(EXIT_FAILURE);
  }
}
