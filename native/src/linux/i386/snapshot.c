#ifdef __i386__

#include <stddef.h>
#include <sys/ptrace.h>
#include <sys/types.h>

#include <linux/i386/snapshot.h>
#include <log.h>

#define GDT_ENTRY_TLS_MIN 12
#define GDT_ENTRY_TLS_MAX 14

void create_snapshot(snapshot_info_t *snapshot,
                     const breakpoint_t *breakpoint) {
  pid_t pid = breakpoint->pid;

  if (ptrace(PTRACE_GETREGS, pid, NULL, &snapshot->regs) == -1) {
    LOG_FATAL_ERRNO("ptrace(PTRACE_GETREGS, pid=%d) failed", pid);
  }

  for (int i = GDT_ENTRY_TLS_MIN; i <= GDT_ENTRY_TLS_MAX; ++i) {
    if (ptrace(PTRACE_GET_THREAD_AREA, pid, i,
               &snapshot->tls[i - GDT_ENTRY_TLS_MIN]) == -1) {
      LOG_FATAL_ERRNO("ptrace(PTRACE_GET_THREAD_AREA, pid=%d, index=%d) failed",
                      pid, i);
    }
  }

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
  LOG_DEBUG("xcs: 0x%lx", snapshot->regs.xcs);
  LOG_DEBUG("xss: 0x%lx", snapshot->regs.xss);
  LOG_DEBUG("xds: 0x%lx", snapshot->regs.xds);
  LOG_DEBUG("xes: 0x%lx", snapshot->regs.xes);
  LOG_DEBUG("xfs: 0x%lx", snapshot->regs.xfs);
  LOG_DEBUG("xgs: 0x%lx", snapshot->regs.xgs);

  for (int i = 0; i < GDT_ENTRY_TLS_MAX - GDT_ENTRY_TLS_MIN; ++i) {
    LOG_DEBUG(
        "gdt[%d]:\n"
        "  user_desc {\n"
        "    entry_number    = %u (0x%08x)\n"
        "    base_addr       = %u (0x%08x)\n"
        "    limit           = %u (0x%08x)\n"
        "    seg_32bit       = %u\n"
        "    contents        = %u\n"
        "    read_exec_only  = %u\n"
        "    limit_in_pages  = %u\n"
        "    seg_not_present = %u\n"
        "    useable         = %u\n"
        "}\n",
        i + GDT_ENTRY_TLS_MIN, snapshot->tls[i].entry_number,
        snapshot->tls[i].entry_number, snapshot->tls[i].base_addr,
        snapshot->tls[i].base_addr, snapshot->tls[i].limit,
        snapshot->tls[i].limit, snapshot->tls[i].seg_32bit,
        snapshot->tls[i].contents, snapshot->tls[i].read_exec_only,
        snapshot->tls[i].limit_in_pages, snapshot->tls[i].seg_not_present,
        snapshot->tls[i].useable);
  }

  LOG_INFO("snapshot captured for pid %d at eip=0x%lx gs=0x%lx", pid,
           snapshot->regs.eip, snapshot->regs.xgs);
}

#endif
