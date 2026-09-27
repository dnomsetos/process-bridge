#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include <linux/maps_entry.h>

void parse_maps_str(const char *maps_str, maps_entry_t *entry);

typedef struct {
  maps_entry_t entry;
  FILE *file;
  char *line;
} maps_iter_t;

void create_maps_iter(maps_iter_t *iter, pid_t pid);

const maps_entry_t *next(maps_iter_t *iter);
