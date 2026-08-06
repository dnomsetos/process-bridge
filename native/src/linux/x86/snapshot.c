#ifdef __i386__

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>

#include <linux/x86/snapshot.h>

void create_snapshot(snapshot_info_t *snapshot, breakpoint_t *breakpoint) {
  pid_t pid = *(pid_t *)breakpoint;
  if (ptrace(PTRACE_GETREGS, pid, NULL, &snapshot->regs) == -1) {
    perror("ptrace");
    exit(EXIT_FAILURE);
  }

  unsigned long index = snapshot->regs.xgs >> 3;
  struct user_desc desc;

  if (ptrace(PTRACE_GET_THREAD_AREA, pid, index, &desc)) {
    perror("ptrace");
    exit(EXIT_FAILURE);
  }

  snapshot->seg.base_addr = desc.base_addr;
  snapshot->seg.limit = desc.limit;
  snapshot->seg.flags = (desc.seg_32bit << 0) | (desc.contents << 1) |
                        (desc.read_exec_only << 3) |
                        (desc.limit_in_pages << 4) |
                        (desc.seg_not_present << 5) | (desc.useable << 6);

  printf("segment base: %x\n", snapshot->seg.base_addr);
  printf("limit: %x\n", snapshot->seg.limit);
  printf("flags: %x\n", snapshot->seg.flags);

  printf("ebp: 0x%lx\n", snapshot->regs.ebp);
  printf("ebx: 0x%lx\n", snapshot->regs.ebx);
  printf("eax: 0x%lx\n", snapshot->regs.eax);
  printf("ecx: 0x%lx\n", snapshot->regs.ecx);
  printf("edx: 0x%lx\n", snapshot->regs.edx);
  printf("esi: 0x%lx\n", snapshot->regs.esi);
  printf("edi: 0x%lx\n", snapshot->regs.edi);
  printf("orig_eax: 0x%lx\n", snapshot->regs.orig_eax);
  printf("eip: 0x%lx\n", snapshot->regs.eip);
  printf("xcs: 0x%lx\n", snapshot->regs.xcs);
  printf("eflags: 0x%lx\n", snapshot->regs.eflags);
  printf("esp: 0x%lx\n", snapshot->regs.esp);
  printf("xss: 0x%lx\n", snapshot->regs.xss);
  printf("xds: 0x%lx\n", snapshot->regs.xds);
  printf("xes: 0x%lx\n", snapshot->regs.xes);
  printf("xfs: 0x%lx\n", snapshot->regs.xfs);
  printf("xgs: 0x%lx\n", snapshot->regs.xgs);

  fflush(stdout);
}

#endif
