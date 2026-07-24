#include <linux/limits.h>
#include <stdint.h>

typedef struct {
  uint64_t start;
  char path[PATH_MAX];
} maps_entry_t;

void parse_maps_str(const char *maps_str, maps_entry_t *entry);
