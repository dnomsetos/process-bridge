#ifdef __x86_64__

#include <sys/ptrace.h>
#include <sys/types.h>

#include <linux/snapshot.h>
#include <log.h>

void create_snapshot(process_state_t *snapshot,
                     const breakpoint_t *breakpoint) {
  pid_t pid = breakpoint->pid;
  if (ptrace(PTRACE_GETREGS, pid, NULL, &snapshot->regs) == -1) {
    LOG_FATAL_ERRNO("ptrace(PTRACE_GETREGS, pid=%d) failed", pid);
  }

  LOG_DEBUG("r15: 0x%lx", snapshot->regs.r15);
  LOG_DEBUG("r14: 0x%lx", snapshot->regs.r14);
  LOG_DEBUG("r13: 0x%lx", snapshot->regs.r13);
  LOG_DEBUG("r12: 0x%lx", snapshot->regs.r12);
  LOG_DEBUG("rbp: 0x%lx", snapshot->regs.rbp);
  LOG_DEBUG("rbx: 0x%lx", snapshot->regs.rbx);
  LOG_DEBUG("r11: 0x%lx", snapshot->regs.r11);
  LOG_DEBUG("r10: 0x%lx", snapshot->regs.r10);
  LOG_DEBUG("r9: 0x%lx", snapshot->regs.r9);
  LOG_DEBUG("r8: 0x%lx", snapshot->regs.r8);
  LOG_DEBUG("rax: 0x%lx", snapshot->regs.rax);
  LOG_DEBUG("rcx: 0x%lx", snapshot->regs.rcx);
  LOG_DEBUG("rdx: 0x%lx", snapshot->regs.rdx);
  LOG_DEBUG("rsi: 0x%lx", snapshot->regs.rsi);
  LOG_DEBUG("rdi: 0x%lx", snapshot->regs.rdi);
  LOG_DEBUG("orig_rax: 0x%lx", snapshot->regs.orig_rax);
  LOG_DEBUG("rip: 0x%lx", snapshot->regs.rip);
  LOG_DEBUG("cs: 0x%lx", snapshot->regs.cs);
  LOG_DEBUG("eflags: 0x%lx", snapshot->regs.eflags);
  LOG_DEBUG("rsp: 0x%lx", snapshot->regs.rsp);
  LOG_DEBUG("ss: 0x%lx", snapshot->regs.ss);
  LOG_DEBUG("fs_base: 0x%lx", snapshot->regs.fs_base);
  LOG_DEBUG("gs_base: 0x%lx", snapshot->regs.gs_base);
  LOG_DEBUG("ds: 0x%lx", snapshot->regs.ds);
  LOG_DEBUG("es: 0x%lx", snapshot->regs.es);
  LOG_DEBUG("fs: 0x%lx", snapshot->regs.fs);
  LOG_DEBUG("gs: 0x%lx", snapshot->regs.gs);

  LOG_INFO("snapshot captured for pid %d at rip=0x%lx",
           pid,
           snapshot->regs.rip);
}

#endif
