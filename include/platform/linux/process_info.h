#include <elf.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <sys/types.h>

typedef struct {
  int fd;
  pid_t pid;
  Elf64_Ehdr ehdr;
  char path[PATH_MAX];
} elf_info_t;

int open_elf(const char *filename, elf_info_t *info);

bool is_pie(const elf_info_t *info);

size_t get_min_vaddr(const elf_info_t *info);

size_t get_image_base(const elf_info_t *info);
