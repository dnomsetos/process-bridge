#pragma once

#include <elf.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>

#define MAX_PATH 4096

typedef enum {
  ELF_CLASS_32,
  ELF_CLASS_64,
} elf_class_t;

typedef struct {
  elf_class_t class;
  union {
    Elf32_Ehdr e32;
    Elf64_Ehdr e64;
  } ehdr;

  char path[MAX_PATH];
  pid_t pid;
  FILE *file;
} elf_info_t;

elf_class_t detect_elf_class(FILE *file);

void open_elf(const char *filename, elf_info_t *info);

bool is_pie(const elf_info_t *info);

uint64_t get_min_vaddr(const elf_info_t *info);

uint64_t get_image_base_via_elf_info(const elf_info_t *info);

uint64_t get_image_base(const char *filename, pid_t pid);

#undef MAX_PATH
