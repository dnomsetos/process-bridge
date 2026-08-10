#ifdef __i386__

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>

#include <linux/x86/snapshot.h>
#include <log.h>

void create_snapshot(snapshot_info_t *snapshot, breakpoint_t *breakpoint) {
  pid_t pid = breakpoint->pid;
  if (ptrace(PTRACE_GETREGS, pid, NULL, &snapshot->regs) == -1) {
    LOG_FATAL_ERRNO("ptrace(PTRACE_GETREGS, pid=%d) failed", pid);
  }

  unsigned long index = snapshot->regs.xgs >> 3;
  struct user_desc desc;

  if (ptrace(PTRACE_GET_THREAD_AREA, pid, index, &desc) == -1) {
    LOG_FATAL_ERRNO(
        "ptrace(PTRACE_GET_THREAD_AREA, pid=%d, index=%lu) "
        "failed",
        pid, index);
  }

  snapshot->seg.base_addr = desc.base_addr;
  snapshot->seg.limit = desc.limit;
  snapshot->seg.flags = (desc.seg_32bit << 0) | (desc.contents << 1) |
                        (desc.read_exec_only << 3) |
                        (desc.limit_in_pages << 4) |
                        (desc.seg_not_present << 5) | (desc.useable << 6);

  LOG_DEBUG("segment base: 0x%x", snapshot->seg.base_addr);
  LOG_DEBUG("limit: 0x%x", snapshot->seg.limit);
  LOG_DEBUG("flags: 0x%x", snapshot->seg.flags);

  LOG_DEBUG("ebp: 0x%lx", snapshot->regs.ebp);
  LOG_DEBUG("ebx: 0x%lx", snapshot->regs.ebx);
  LOG_DEBUG("eax: 0x%lx", snapshot->regs.eax);
  LOG_DEBUG("ecx: 0x%lx", snapshot->regs.ecx);
  LOG_DEBUG("edx: 0x%lx", snapshot->regs.edx);
  LOG_DEBUG("esi: 0x%lx", snapshot->regs.esi);
  LOG_DEBUG("edi: 0x%lx", snapshot->regs.edi);
  LOG_DEBUG("orig_eax: 0x%lx", snapshot->regs.orig_eax);
  LOG_DEBUG("eip: 0x%lx", snapshot->regs.eip);
  LOG_DEBUG("xcs: 0x%lx", snapshot->regs.xcs);
  LOG_DEBUG("eflags: 0x%lx", snapshot->regs.eflags);
  LOG_DEBUG("esp: 0x%lx", snapshot->regs.esp);
  LOG_DEBUG("xss: 0x%lx", snapshot->regs.xss);
  LOG_DEBUG("xds: 0x%lx", snapshot->regs.xds);
  LOG_DEBUG("xes: 0x%lx", snapshot->regs.xes);
  LOG_DEBUG("xfs: 0x%lx", snapshot->regs.xfs);
  LOG_DEBUG("xgs: 0x%lx", snapshot->regs.xgs);

  LOG_INFO("snapshot captured for pid %d at eip=0x%lx", pid,
           snapshot->regs.eip);
}

#endif
