#ifdef __x86_64__

#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>

#include <linux/x86_64/snapshot.h>

void create_snapshot(snapshot_info_t *snapshot, breakpoint_t *breakpoint) {
  pid_t pid = *(pid_t *)breakpoint;
  if (ptrace(PTRACE_GETREGS, pid, NULL, &snapshot->regs) == -1) {
    perror("ptrace");
    exit(EXIT_FAILURE);
  }

  printf("r15: 0x%llx\n", snapshot->regs.r15);
  printf("r14: 0x%llx\n", snapshot->regs.r14);
  printf("r13: 0x%llx\n", snapshot->regs.r13);
  printf("r12: 0x%llx\n", snapshot->regs.r12);
  printf("rbp: 0x%llx\n", snapshot->regs.rbp);
  printf("rbx: 0x%llx\n", snapshot->regs.rbx);
  printf("r11: 0x%llx\n", snapshot->regs.r11);
  printf("r10: 0x%llx\n", snapshot->regs.r10);
  printf("r9: 0x%llx\n", snapshot->regs.r9);
  printf("r8: 0x%llx\n", snapshot->regs.r8);
  printf("rax: 0x%llx\n", snapshot->regs.rax);
  printf("rcx: 0x%llx\n", snapshot->regs.rcx);
  printf("rdx: 0x%llx\n", snapshot->regs.rdx);
  printf("rsi: 0x%llx\n", snapshot->regs.rsi);
  printf("rdi: 0x%llx\n", snapshot->regs.rdi);
  printf("orig_rax: 0x%llx\n", snapshot->regs.orig_rax);
  printf("rip: 0x%llx\n", snapshot->regs.rip);
  printf("cs: 0x%llx\n", snapshot->regs.cs);
  printf("eflags: 0x%llx\n", snapshot->regs.eflags);
  printf("rsp: 0x%llx\n", snapshot->regs.rsp);
  printf("ss: 0x%llx\n", snapshot->regs.ss);
  printf("fs_base: 0x%llx\n", snapshot->regs.fs_base);
  printf("gs_base: 0x%llx\n", snapshot->regs.gs_base);
  printf("ds: 0x%llx\n", snapshot->regs.ds);
  printf("es: 0x%llx\n", snapshot->regs.es);
  printf("fs: 0x%llx\n", snapshot->regs.fs);
  printf("gs: 0x%llx\n", snapshot->regs.gs);

  fflush(stdout);
}

#endif
