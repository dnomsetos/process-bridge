#include <stdio.h>
#include <sys/ptrace.h>

#include <platform/linux/breakpoint.h>
#include <platform/linux/maps_parser.h>
#include <platform/linux/process_info.h>

int main(int argc, char *argv[]) {
  char **child_argv = argc > 2 ? &argv[2] : NULL;

  pid_t pid = fork_child(argv[1], child_argv, NULL);

  elf_info_t elf_info;
  elf_info.pid = pid;
  open_elf(argv[1], &elf_info);

  size_t image_base = get_image_base(&elf_info);

  printf("image base: %lx\n", image_base);

  breakpoint_t breakpoint;
  create_breakpoint(&breakpoint, pid, (void *)(image_base + 0x1149));

  while (true) {
    sleep(2);

    execute_up_to_breakpoint(&breakpoint);
  }
  return 0;
}
