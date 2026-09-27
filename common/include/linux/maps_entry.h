#pragma once

#include <stdbool.h>
#include <stdint.h>

#define MAX_PATH 4096

typedef struct __attribute__((packed)) {
  uint64_t start;
  uint64_t end;

  uint8_t read;
  uint8_t write;
  uint8_t exec;
  uint8_t shared;

  uint64_t file_offset;

  char path[MAX_PATH];
} maps_entry_t;

_Static_assert(sizeof(maps_entry_t) == 28 + MAX_PATH,
               "maps_entry_t has unexpected layout");

#undef MAX_PATH
