#ifdef __x86_64__

#include <sys/ptrace.h>
#include <sys/types.h>

#include <linux/x86_64/snapshot.h>
#include <log.h>

void create_snapshot(snapshot_info_t *snapshot, breakpoint_t *breakpoint) {
  pid_t pid = breakpoint->pid;
  if (ptrace(PTRACE_GETREGS, pid, NULL, &snapshot->regs) == -1) {
    LOG_FATAL_ERRNO("ptrace(PTRACE_GETREGS, pid=%d) failed", pid);
  }

  LOG_DEBUG("r15: 0x%llx", snapshot->regs.r15);
  LOG_DEBUG("r14: 0x%llx", snapshot->regs.r14);
  LOG_DEBUG("r13: 0x%llx", snapshot->regs.r13);
  LOG_DEBUG("r12: 0x%llx", snapshot->regs.r12);
  LOG_DEBUG("rbp: 0x%llx", snapshot->regs.rbp);
  LOG_DEBUG("rbx: 0x%llx", snapshot->regs.rbx);
  LOG_DEBUG("r11: 0x%llx", snapshot->regs.r11);
  LOG_DEBUG("r10: 0x%llx", snapshot->regs.r10);
  LOG_DEBUG("r9: 0x%llx", snapshot->regs.r9);
  LOG_DEBUG("r8: 0x%llx", snapshot->regs.r8);
  LOG_DEBUG("rax: 0x%llx", snapshot->regs.rax);
  LOG_DEBUG("rcx: 0x%llx", snapshot->regs.rcx);
  LOG_DEBUG("rdx: 0x%llx", snapshot->regs.rdx);
  LOG_DEBUG("rsi: 0x%llx", snapshot->regs.rsi);
  LOG_DEBUG("rdi: 0x%llx", snapshot->regs.rdi);
  LOG_DEBUG("orig_rax: 0x%llx", snapshot->regs.orig_rax);
  LOG_DEBUG("rip: 0x%llx", snapshot->regs.rip);
  LOG_DEBUG("cs: 0x%llx", snapshot->regs.cs);
  LOG_DEBUG("eflags: 0x%llx", snapshot->regs.eflags);
  LOG_DEBUG("rsp: 0x%llx", snapshot->regs.rsp);
  LOG_DEBUG("ss: 0x%llx", snapshot->regs.ss);
  LOG_DEBUG("fs_base: 0x%llx", snapshot->regs.fs_base);
  LOG_DEBUG("gs_base: 0x%llx", snapshot->regs.gs_base);
  LOG_DEBUG("ds: 0x%llx", snapshot->regs.ds);
  LOG_DEBUG("es: 0x%llx", snapshot->regs.es);
  LOG_DEBUG("fs: 0x%llx", snapshot->regs.fs);
  LOG_DEBUG("gs: 0x%llx", snapshot->regs.gs);

  LOG_INFO("snapshot captured for pid %d at rip=0x%llx", pid,
           snapshot->regs.rip);
}

#endif
