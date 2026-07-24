#include <stdio.h>

#include <platform/linux/maps_parser.h>

void parse_maps_str(const char *maps_str, maps_entry_t *entry) {
  entry->path[0] = '\0';
  sscanf(maps_str, "%lx-%*s %*s %*s %*s %*s %4095s", &entry->start, entry->path);
  // printf("start: %lx\npath: %s\n", entry->start, entry->path);
}
