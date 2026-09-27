#include <elf.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/maps_parser.h>
#include <linux/process_info.h>
#include <log.h>

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
    LOG_FATAL_ERRNO("fseek to start of ELF file failed");
  }
  if (fread(e_ident, 1, EI_NIDENT, file) != EI_NIDENT) {
    if (ferror(file)) {
      LOG_FATAL_ERRNO("fread of ELF ident bytes failed");
    }
    LOG_FATAL(
        "file is too short to contain an ELF header (%d bytes "
        "expected)",
        EI_NIDENT);
  }

  if (memcmp(e_ident, ELFMAG, SELFMAG) != 0) {
    LOG_FATAL("not an ELF file (bad magic bytes)");
  }

  switch (e_ident[EI_CLASS]) {
    case ELFCLASS32:
      return ELF_CLASS_32;
    case ELFCLASS64:
      return ELF_CLASS_64;
    default:
      LOG_FATAL("unknown ELF class byte 0x%02x", e_ident[EI_CLASS]);
  }
}

void open_elf(const char *filename, elf_info_t *info) {
  info->file = fopen(filename, "rb");
  if (!info->file) {
    LOG_FATAL_ERRNO("fopen('%s') failed", filename);
  }

  info->class = detect_elf_class(info->file);
  rewind(info->file);

  size_t hdr_size =
      info->class == ELF_CLASS_32 ? sizeof(Elf32_Ehdr) : sizeof(Elf64_Ehdr);
  void *hdr_ptr = info->class == ELF_CLASS_32 ? (void *)&info->ehdr.e32
                                              : (void *)&info->ehdr.e64;

  if (fread(hdr_ptr, hdr_size, 1, info->file) != 1) {
    LOG_FATAL_ERRNO("fread of ELF header from '%s' failed", filename);
  }

  char link[32];

  int fd = fileno(info->file);
  if (fd == -1) {
    LOG_FATAL_ERRNO("fileno() on the freshly opened ELF file failed");
  }

  snprintf(link, sizeof(link), "/proc/self/fd/%d", fd);

  ssize_t len = readlink(link, info->path, sizeof(info->path) - 1);
  if (len == -1) {
    LOG_FATAL_ERRNO(
        "readlink('%s') failed while resolving the absolute "
        "path of '%s'",
        link, filename);
  }

  info->path[len] = '\0';

  LOG_INFO("elf absolute path: %s", info->path);
}

bool is_pie(const elf_info_t *info) {
  if (ehdr_type(info) != ET_DYN && ehdr_type(info) != ET_EXEC) {
    LOG_FATAL("unsupported ELF type %d (expected ET_EXEC or ET_DYN)",
              ehdr_type(info));
  }

  return ehdr_type(info) == ET_DYN;
}

uint64_t get_min_vaddr(const elf_info_t *info) {
  uint64_t min_vaddr = SIZE_MAX;

  Elf32_Phdr phdr32;
  Elf64_Phdr phdr64;

  for (size_t i = 0; i < ehdr_phnum(info); ++i) {
    off_t offset = (off_t)(ehdr_phoff(info) + (i * ehdr_phentsize(info)));

    if (fseek(info->file, offset, SEEK_SET) != 0) {
      LOG_FATAL_ERRNO("fseek to program header #%zu (offset %ld) failed", i,
                      (long)offset);
    }

    uint64_t p_type;
    uint64_t p_vaddr;

    if (info->class == ELF_CLASS_32) {
      if (fread(&phdr32, sizeof(Elf32_Phdr), 1, info->file) != 1) {
        LOG_FATAL_ERRNO("fread of 32-bit program header #%zu failed", i);
      }
      p_type = phdr32.p_type;
      p_vaddr = phdr32.p_vaddr;
    } else {
      if (fread(&phdr64, sizeof(Elf64_Phdr), 1, info->file) != 1) {
        LOG_FATAL_ERRNO("fread of 64-bit program header #%zu failed", i);
      }
      p_type = phdr64.p_type;
      p_vaddr = phdr64.p_vaddr;
    }

    if (p_type == PT_LOAD && p_vaddr < min_vaddr) {
      min_vaddr = p_vaddr;
    }
  }

  if (min_vaddr == SIZE_MAX) {
    LOG_WARN(
        "no PT_LOAD segment found; image base calculation will be "
        "wrong");
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

  if (min_maps_addr == SIZE_MAX) {
    LOG_FATAL(
        "no mapping of '%s' found in /proc/%d/maps; the process may "
        "not have loaded it yet, or the path doesn't match exactly",
        info->path, info->pid);
  }

  return min_maps_addr - min_vaddr;
}

uint64_t get_image_base(const char *filename, pid_t pid) {
  elf_info_t elf_info;
  elf_info.pid = pid;

  open_elf(filename, &elf_info);

  uint64_t image_base = get_image_base_via_elf_info(&elf_info);

  if (fclose(elf_info.file) != 0) {
    LOG_FATAL_ERRNO("fclose('%s') failed", filename);
  }

  LOG_INFO("image base for '%s' (pid %d): 0x%" PRIx64, filename, pid,
           image_base);

  return image_base;
}
