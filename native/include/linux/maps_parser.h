#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#define PROC_BUF_SIZE 22
#define MAX_PATH 4096

typedef struct {
  uint64_t start;
  uint64_t end;

  bool read;
  bool write;
  bool exec;
  bool shared;

  uint64_t file_offset;

  char path[MAX_PATH];
} maps_entry_t;

void parse_maps_str(const char *maps_str, maps_entry_t *entry);

typedef struct {
  maps_entry_t entry;
  FILE *file;
  char *line;
} maps_iter_t;

void create_maps_iter(maps_iter_t *iter, pid_t pid);

const maps_entry_t *next(maps_iter_t *iter);

#undef MAX_PATH
