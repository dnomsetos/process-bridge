#include <gdt_utils.h>
#include <linux/x86_64/process_state.h>
#include <log.h>
#include <uc_utils.h>
#include <x86_64/cpu_state_dump.h>

#include <unicorn/unicorn.h>
#include <unicorn/x86.h>

#define GDT_ENTRY_DEFAULT_USER_DS 5
#define GDT_ENTRY_DEFAULT_USER_CS 6

static uint64_t dump_gdt(uc_engine *uc, linux_x86_64_process_state_t *state) {
  PB_UC_CHECK(
      uc_mem_map(uc, PB_GDT_ADDR, PB_GDT_SIZE, UC_PROT_READ | UC_PROT_WRITE),
      "failed to map gdt: %s"
  );
  LOG_DEBUG("gdt mapped successfully at 0x%" PRIx64, PB_GDT_ADDR);

  uint8_t gdt[PB_GDT_SIZE] = {0};
  uint64_t *gdt_array = (uint64_t *)gdt;

  if ((state->regs.cs & 0x4) == 1 || (state->regs.ds & 0x4) == 1 ||
      (state->regs.es & 0x4) == 1 || (state->regs.fs & 0x4) == 1 ||
      (state->regs.gs & 0x4) == 1 || (state->regs.ss & 0x4) == 1) {
    LOG_FATAL("ldt not supported");
    return -1;
  }

  if ((state->regs.cs >> 3) != GDT_ENTRY_DEFAULT_USER_CS) {
    LOG_FATAL("cs is not the default user cs");
    return -1;
  }

  uint64_t code_desc = GDT_ENTRY_INIT(DESC_CODE64 | DESC_USER, 0, 0xfffff);
  gdt_array[GDT_ENTRY_DEFAULT_USER_CS] = code_desc;
  LOG_DEBUG("restored code descriptor: 0x%" PRIx64, code_desc);

  if ((state->regs.ss >> 3) != GDT_ENTRY_DEFAULT_USER_DS) {
    LOG_FATAL("ss is not the default user ds");
    return -1;
  }

  uint64_t data_desc = GDT_ENTRY_INIT(DESC_DATA64 | DESC_USER, 0, 0xfffff);
  gdt_array[GDT_ENTRY_DEFAULT_USER_DS] = data_desc;
  LOG_DEBUG("restored data descriptor: 0x%" PRIx64, data_desc);

  if ((state->regs.ds >> 3) != 0 &&
      (state->regs.ds >> 3) != GDT_ENTRY_DEFAULT_USER_DS) {
    LOG_FATAL("ds is not the default kernel ds");
    return -1;
  }

  if ((state->regs.es >> 3) != 0 &&
      (state->regs.es >> 3) != GDT_ENTRY_DEFAULT_USER_DS) {
    LOG_FATAL("es is not the default kernel ds");
    return -1;
  }

  PB_UC_CHECK(uc_mem_write(uc, PB_GDT_ADDR, gdt, PB_GDT_SIZE),
              "failed to write gdt: %s");
  LOG_DEBUG("gdt written successfully");

  PB_UC_CHECK(uc_mem_protect(uc, PB_GDT_ADDR, PB_GDT_SIZE, UC_PROT_READ),
              "failed to protect gdt: %s");
  LOG_DEBUG("gdt protected successfully");

  uc_x86_mmr gdtr = {
      .selector = 0,
      .base = PB_GDT_ADDR,
      .limit = PB_GDT_SIZE - 1,
      .flags = 0,
  };

  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_GDTR, &gdtr), "failed to write gdtr");
  LOG_DEBUG("gdtr written successfully");
  return 0;
}

static uint64_t dump_regs(uc_engine *uc, linux_x86_64_process_state_t *state) {
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_RAX, &state->regs.rax),
              "failed to write rax");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_RBX, &state->regs.rbx),
              "failed to write rbx");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_RCX, &state->regs.rcx),
              "failed to write rcx");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_RDX, &state->regs.rdx),
              "failed to write rdx");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_RSI, &state->regs.rsi),
              "failed to write rsi");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_RDI, &state->regs.rdi),
              "failed to write rdi");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_RBP, &state->regs.rbp),
              "failed to write rbp");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_RSP, &state->regs.rsp),
              "failed to write rsp");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_R8, &state->regs.r8),
              "failed to write r8");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_R9, &state->regs.r9),
              "failed to write r9");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_R10, &state->regs.r10),
              "failed to write r10");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_R11, &state->regs.r11),
              "failed to write r11");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_R12, &state->regs.r12),
              "failed to write r12");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_R13, &state->regs.r13),
              "failed to write r13");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_R14, &state->regs.r14),
              "failed to write r14");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_R15, &state->regs.r15),
              "failed to write r15");

  LOG_DEBUG("general-purpose registers written successfully");
  LOG_DEBUG("rax: 0x%" PRIx64, (uint64_t)state->regs.rax);
  LOG_DEBUG("rbx: 0x%" PRIx64, (uint64_t)state->regs.rbx);
  LOG_DEBUG("rcx: 0x%" PRIx64, (uint64_t)state->regs.rcx);
  LOG_DEBUG("rdx: 0x%" PRIx64, (uint64_t)state->regs.rdx);
  LOG_DEBUG("rsi: 0x%" PRIx64, (uint64_t)state->regs.rsi);
  LOG_DEBUG("rdi: 0x%" PRIx64, (uint64_t)state->regs.rdi);
  LOG_DEBUG("rbp: 0x%" PRIx64, (uint64_t)state->regs.rbp);
  LOG_DEBUG("rsp: 0x%" PRIx64, (uint64_t)state->regs.rsp);
  LOG_DEBUG("r8: 0x%" PRIx64, (uint64_t)state->regs.r8);
  LOG_DEBUG("r9: 0x%" PRIx64, (uint64_t)state->regs.r9);
  LOG_DEBUG("r10: 0x%" PRIx64, (uint64_t)state->regs.r10);
  LOG_DEBUG("r11: 0x%" PRIx64, (uint64_t)state->regs.r11);
  LOG_DEBUG("r12: 0x%" PRIx64, (uint64_t)state->regs.r12);
  LOG_DEBUG("r13: 0x%" PRIx64, (uint64_t)state->regs.r13);
  LOG_DEBUG("r14: 0x%" PRIx64, (uint64_t)state->regs.r14);
  LOG_DEBUG("r15: 0x%" PRIx64, (uint64_t)state->regs.r15);

  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_RIP, &state->regs.rip),
              "failed to write rip");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_EFLAGS, &state->regs.eflags),
              "failed to write eflags");

  LOG_DEBUG("rip and eflags written successfully");
  LOG_DEBUG("rip: 0x%" PRIx64, (uint64_t)state->regs.rip);
  LOG_DEBUG("eflags: 0x%" PRIx64, (uint64_t)state->regs.eflags);

  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_CS, &state->regs.cs),
              "failed to write cs");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_SS, &state->regs.ss),
              "failed to write ss");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_DS, &state->regs.ds),
              "failed to write ds");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_ES, &state->regs.es),
              "failed to write es");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_FS, &state->regs.fs),
              "failed to write fs");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_GS, &state->regs.gs),
              "failed to write gs");

  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_FS_BASE, &state->regs.fs_base),
              "failed to write fs base");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_GS_BASE, &state->regs.gs_base),
              "failed to write gs base");

  LOG_DEBUG("segment registers restored successfully");
  LOG_DEBUG("cs: 0x%" PRIx64, (uint64_t)state->regs.cs);
  LOG_DEBUG("ss: 0x%" PRIx64, (uint64_t)state->regs.ss);
  LOG_DEBUG("ds: 0x%" PRIx64, (uint64_t)state->regs.ds);
  LOG_DEBUG("es: 0x%" PRIx64, (uint64_t)state->regs.es);
  LOG_DEBUG("fs: 0x%" PRIx64, (uint64_t)state->regs.fs);
  LOG_DEBUG("gs: 0x%" PRIx64, (uint64_t)state->regs.gs);
  LOG_DEBUG("fs base: 0x%" PRIx64, (uint64_t)state->regs.fs_base);
  LOG_DEBUG("gs base: 0x%" PRIx64, (uint64_t)state->regs.gs_base);

  return 0;
}

uint64_t dump_x86_64_cpu_state(uc_engine *uc,
                               linux_x86_64_process_state_t *state) {
  if (dump_gdt(uc, state) == -1) {
    return -1;
  }

  if (dump_regs(uc, state) == -1) {
    return -1;
  }

  uint64_t entry = -1;
  PB_UC_CHECK(uc_reg_read(uc, UC_X86_REG_RIP, &entry), "failed to read rip");

  return entry;
}
