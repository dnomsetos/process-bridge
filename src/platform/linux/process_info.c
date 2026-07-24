#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <platform/linux/maps_parser.h>
#include <platform/linux/process_info.h>

#define PROC_BUF_SIZE 64

static void read_full(int fd, char *buffer, size_t size) {
  size_t total = 0;

  while (total < size) {
    ssize_t bytes_read = read(fd, buffer + total, size - total);

    if (bytes_read > 0) {
      total += (size_t)bytes_read;
      continue;
    }

    if (bytes_read == EINTR) {
      continue;
    }

    perror("read");
    exit(EXIT_FAILURE);
  }
}

int open_elf(const char *filename, elf_info_t *info) {
  int fd = open(filename, O_RDONLY);
  info->fd = fd;

  if (fd < 0) {
    perror("open");
    exit(EXIT_FAILURE);
  }

  read_full(fd, (char *)&info->ehdr, sizeof(Elf64_Ehdr));

  char link[PROC_BUF_SIZE];

  snprintf(link, sizeof(link), "/proc/self/fd/%d", fd);

  ssize_t len = readlink(link, info->path, sizeof(info->path) - 1);
  if (len == -1) {
    perror("readlink");
    exit(EXIT_FAILURE);
  }
  info->path[len] = '\0';

  printf("elf absolute path: %s\n", info->path);

  return fd;
}

bool is_pie(const elf_info_t *info) {
  if (info->ehdr.e_type != ET_DYN && info->ehdr.e_type != ET_EXEC) {
    printf("Unsupported ELF type: %d\n", info->ehdr.e_type);
    exit(EXIT_FAILURE);
  }

  return info->ehdr.e_type == ET_DYN;
}

size_t get_min_vaddr(const elf_info_t *info) {
  size_t min_vaddr = SIZE_MAX;

  Elf64_Phdr phdr;

  for (size_t i = 0; i < info->ehdr.e_phnum; ++i) {
    off_t offset = (off_t)(info->ehdr.e_phoff + (i * info->ehdr.e_phentsize));

    lseek(info->fd, offset, SEEK_SET);
    read(info->fd, &phdr, sizeof(Elf64_Phdr));

    if (phdr.p_type == PT_LOAD && phdr.p_vaddr < min_vaddr) {
      min_vaddr = phdr.p_vaddr;
    }
  }

  return min_vaddr;
}

size_t get_image_base(const elf_info_t *info) {
  if (!is_pie(info)) {
    return 0;
  }

  size_t min_maps_addr = SIZE_MAX;
  size_t min_vaddr = get_min_vaddr(info);

  char buf[PROC_BUF_SIZE];
  snprintf(buf, sizeof(buf), "/proc/%d/maps", info->pid);

  FILE *maps = fopen(buf, "r");

  if (maps == NULL) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  char *line = NULL;
  size_t line_size = 0;
  maps_entry_t entry;

  while (getline(&line, &line_size, maps) != -1) {
    parse_maps_str(line, &entry);

    if (strcmp(entry.path, info->path) == 0 && min_maps_addr > entry.start) {
      min_maps_addr = entry.start;
    }
  }

  return min_maps_addr - min_vaddr;
}
