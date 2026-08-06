#include <inttypes.h>
#include <stdlib.h>

#include <linux/maps_parser.h>

void parse_maps_str(const char *maps_str, maps_entry_t *entry) {
  char perms[5] = {0};

  entry->path[0] = '\0';

  int count = sscanf(
      maps_str, "%" SCNx64 "-%" SCNx64 " %4s %" SCNx64 " %*s %*s %4095[^\n]",
      &entry->start, &entry->end, perms, &entry->file_offset, entry->path);

  if (count < 4) {
    fprintf(stderr, "Failed to parse maps line:\n%s\n", maps_str);
    abort();
  }

  if (count == 4) {
    entry->path[0] = '\0';
  }

  entry->read = perms[0] == 'r';
  entry->write = perms[1] == 'w';
  entry->exec = perms[2] == 'x';
  entry->shared = perms[3] == 's';

  printf(
      "-----------------------------\n"
      "start: 0x%" PRIx64
      "\n"
      "end:   0x%" PRIx64
      "\n"
      "offset: 0x%" PRIx64
      "\n"
      "perms: %s\n"
      "path: %s\n",
      entry->start, entry->end, entry->file_offset, perms, entry->path);
}

void create_maps_iter(maps_iter_t *iter, pid_t pid) {
  char buffer[PROC_BUF_SIZE];

  snprintf(buffer, PROC_BUF_SIZE, "/proc/%d/maps", pid);
  iter->file = fopen(buffer, "rb");

  if (iter->file == NULL) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  // char buf[4096];
  // size_t n = fread(buf, 4095, 1, iter->file);
  // buf[n] = '\0';
  // printf("%s\n", buf);
  // fflush(stdout);

  iter->line = NULL;
}

const maps_entry_t *next(maps_iter_t *iter) {
  size_t line_size;
  if (iter->line != NULL) {
    free(iter->line);
    iter->line = NULL;
  }

  if (getline(&iter->line, &line_size, iter->file) == -1) {
    if (fclose(iter->file) != 0) {
      perror("fclose");
      exit(EXIT_FAILURE);
    }

    free(iter->line);

    return NULL;
  }

  parse_maps_str(iter->line, &iter->entry);

  return &iter->entry;
}
