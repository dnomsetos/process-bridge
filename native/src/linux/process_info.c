#include <elf.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/maps_parser.h>
#include <linux/process_info.h>

static inline uint16_t ehdr_type(const elf_info_t *info) {
  return info->class == ELF_CLASS_32 ? info->ehdr.e32.e_type
                                     : info->ehdr.e64.e_type;
}
static inline uint16_t ehdr_phnum(const elf_info_t *info) {
  return info->class == ELF_CLASS_32 ? info->ehdr.e32.e_phnum
                                     : info->ehdr.e64.e_phnum;
}
static inline uint64_t ehdr_phoff(const elf_info_t *info) {
  return info->class == ELF_CLASS_32 ? info->ehdr.e32.e_phoff
                                     : info->ehdr.e64.e_phoff;
}
static inline uint16_t ehdr_phentsize(const elf_info_t *info) {
  return info->class == ELF_CLASS_32 ? info->ehdr.e32.e_phentsize
                                     : info->ehdr.e64.e_phentsize;
}

elf_class_t detect_elf_class(FILE *file) {
  unsigned char e_ident[EI_NIDENT];

  if (fseek(file, 0, SEEK_SET) != 0) {
    perror("fseek");
    exit(EXIT_FAILURE);
  }
  if (fread(e_ident, 1, EI_NIDENT, file) != EI_NIDENT) {
    perror("fread");
    exit(EXIT_FAILURE);
  }

  if (memcmp(e_ident, ELFMAG, SELFMAG) != 0) {
    fprintf(stderr, "Not an ELF file\n");
    exit(EXIT_FAILURE);
  }

  switch (e_ident[EI_CLASS]) {
    case ELFCLASS32:
      return ELF_CLASS_32;
    case ELFCLASS64:
      return ELF_CLASS_64;
    default:
      fprintf(stderr, "Unknown ELF class\n");
      exit(EXIT_FAILURE);
  }
}

void open_elf(const char *filename, elf_info_t *info) {
  info->file = fopen(filename, "rb");
  if (!info->file) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  info->class = detect_elf_class(info->file);
  rewind(info->file);

  size_t hdr_size =
      info->class == ELF_CLASS_32 ? sizeof(Elf32_Ehdr) : sizeof(Elf64_Ehdr);
  void *hdr_ptr = info->class == ELF_CLASS_32 ? (void *)&info->ehdr.e32
                                              : (void *)&info->ehdr.e64;

  if (fread(hdr_ptr, hdr_size, 1, info->file) != 1) {
    perror("fread");
    exit(EXIT_FAILURE);
  }

  char link[PROC_BUF_SIZE];

  int fd = fileno(info->file);
  if (fd == -1) {
    perror("fileno");
    exit(EXIT_FAILURE);
  }

  snprintf(link, sizeof(link), "/proc/self/fd/%d", fd);

  ssize_t len = readlink(link, info->path, sizeof(info->path) - 1);
  if (len == -1) {
    perror("readlink");
    exit(EXIT_FAILURE);
  }

  info->path[len] = '\0';

  printf("elf absolute path: %s\n", info->path);
}

bool is_pie(const elf_info_t *info) {
  if (ehdr_type(info) != ET_DYN && ehdr_type(info) != ET_EXEC) {
    printf("Unsupported ELF type: %d\n", ehdr_type(info));
    exit(EXIT_FAILURE);
  }

  return ehdr_type(info) == ET_DYN;
}

uint64_t get_min_vaddr(const elf_info_t *info) {
  uint64_t min_vaddr = SIZE_MAX;

  Elf64_Phdr phdr;

  for (size_t i = 0; i < ehdr_phnum(info); ++i) {
    off_t offset = (off_t)(ehdr_phoff(info) + (i * ehdr_phentsize(info)));

    if (fseek(info->file, offset, SEEK_SET) != 0) {
      perror("fseek");
      exit(EXIT_FAILURE);
    }

    if (fread(&phdr, sizeof(Elf64_Phdr), 1, info->file) != 1) {
      perror("fread");
      exit(EXIT_FAILURE);
    }

    if (phdr.p_type == PT_LOAD && phdr.p_vaddr < min_vaddr) {
      min_vaddr = phdr.p_vaddr;
    }
  }

  return min_vaddr;
}

uint64_t get_image_base_via_elf_info(const elf_info_t *info) {
  if (!is_pie(info)) {
    return 0;
  }

  uint64_t min_maps_addr = SIZE_MAX;
  uint64_t min_vaddr = get_min_vaddr(info);

  maps_iter_t iter;
  create_maps_iter(&iter, info->pid);

  for (const maps_entry_t *cur_entry = next(&iter); cur_entry != NULL;
       cur_entry = next(&iter)) {
    if (strcmp(cur_entry->path, info->path) == 0 &&
        min_maps_addr > cur_entry->start) {
      min_maps_addr = cur_entry->start;
    }
  }

  return min_maps_addr - min_vaddr;
}

uint64_t get_image_base(const char *filename, pid_t pid) {
  elf_info_t elf_info;
  elf_info.pid = pid;

  open_elf(filename, &elf_info);

  uint64_t image_base = get_image_base_via_elf_info(&elf_info);

  if (fclose(elf_info.file) != 0) {
    perror("fclose");
    exit(EXIT_FAILURE);
  }

  return image_base;
}
