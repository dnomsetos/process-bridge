#include <gdt_utils.h>
#include <i386/cpu_state_dump.h>
#include <linux/i386/process_state.h>
#include <log.h>
#include <uc_utils.h>

#include <unicorn/unicorn.h>
#include <unicorn/x86.h>

#define COMPATIBILITY_GDT_ENTRY_DEFAULT_USER32_CS 4
#define COMPATIBILITY_GDT_ENTRY_DEFAULT_USER_DS   5

#define TRUE_GDT_ENTRY_DEFAULT_USER_CS 14
#define TRUE_GDT_ENTRY_DEFAULT_USER_DS 15

static uint64_t dump_gdt(uc_engine *uc, linux_i386_process_state_t *state) {
  PB_UC_CHECK(
      uc_mem_map(uc, PB_GDT_ADDR, PB_GDT_SIZE, UC_PROT_READ | UC_PROT_WRITE),
      "failed to map gdt: %s"
  );
  LOG_DEBUG("gdt mapped successfully at 0x%" PRIx64, PB_GDT_ADDR);

  uint8_t gdt[PB_GDT_SIZE] = {0};
  uint64_t *gdt_array = (uint64_t *)gdt;

  if ((state->regs.xcs & 0x4) == 1 || (state->regs.xds & 0x4) == 1 ||
      (state->regs.xes & 0x4) == 1 || (state->regs.xfs & 0x4) == 1 ||
      (state->regs.xgs & 0x4) == 1 || (state->regs.xss & 0x4) == 1) {
    LOG_FATAL("ldt not supported");
    return -1;
  }

  uint64_t code_desc = GDT_ENTRY_INIT(DESC_CODE32 | DESC_USER, 0, 0xfffff);
  uint64_t data_desc = -1;

  if ((state->regs.xcs >> 3) == COMPATIBILITY_GDT_ENTRY_DEFAULT_USER32_CS) {
    data_desc = GDT_ENTRY_INIT(DESC_DATA64 | DESC_USER, 0, 0xfffff);
  } else if (state->regs.xcs >> 3 == TRUE_GDT_ENTRY_DEFAULT_USER_CS) {
    data_desc = GDT_ENTRY_INIT(DESC_DATA32 | DESC_USER, 0, 0xfffff);
  } else {
    LOG_FATAL("cs is not the default user cs");
    return -1;
  }

  gdt_array[state->regs.xcs >> 3] = code_desc;
  LOG_DEBUG("restored code descriptor: 0x%" PRIx64, code_desc);

  gdt_array[state->regs.xss >> 3] = data_desc;
  LOG_DEBUG("restored data descriptor: 0x%" PRIx64, data_desc);

  for (int i = 0; i < GDT_ENTRY_TLS_ENTRIES; ++i) {
    gdt_array[state->tls[i].entry_number] =
        USER_DESC_TO_REAL_DESC(state->tls[i]);
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

static uint64_t dump_regs(uc_engine *uc, linux_i386_process_state_t *state) {
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_EBX, &state->regs.ebx),
              "failed to write ebx");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_ECX, &state->regs.ecx),
              "failed to write ecx");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_EDX, &state->regs.edx),
              "failed to write edx");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_ESI, &state->regs.esi),
              "failed to write esi");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_EDI, &state->regs.edi),
              "failed to write edi");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_EBP, &state->regs.ebp),
              "failed to write ebp");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_EAX, &state->regs.eax),
              "failed to write eax");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_ESP, &state->regs.esp),
              "failed to write esp");

  LOG_DEBUG("general-purpose registers written successfully");
  LOG_DEBUG("eax: 0x%d" PRIx64, state->regs.eax);
  LOG_DEBUG("ebx: 0x%d" PRIx64, state->regs.ebx);
  LOG_DEBUG("ecx: 0x%d" PRIx64, state->regs.ecx);
  LOG_DEBUG("edx: 0x%d" PRIx64, state->regs.edx);
  LOG_DEBUG("esi: 0x%d" PRIx64, state->regs.esi);
  LOG_DEBUG("edi: 0x%d" PRIx64, state->regs.edi);
  LOG_DEBUG("ebp: 0x%d" PRIx64, state->regs.ebp);
  LOG_DEBUG("esp: 0x%d" PRIx64, state->regs.esp);

  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_EIP, &state->regs.eip),
              "failed to write eip");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_EFLAGS, &state->regs.eflags),
              "failed to write eflags");

  LOG_DEBUG("eip and eflags written successfully");
  LOG_DEBUG("eip: 0x%d" PRIx64, state->regs.eip);
  LOG_DEBUG("eflags: 0x%d" PRIx64, state->regs.eflags);

  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_DS, &state->regs.xds),
              "failed to write xds");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_ES, &state->regs.xes),
              "failed to write xes");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_FS, &state->regs.xfs),
              "failed to write xfs");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_GS, &state->regs.xgs),
              "failed to write xgs");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_CS, &state->regs.xcs),
              "failed to write xcs");
  PB_UC_CHECK(uc_reg_write(uc, UC_X86_REG_SS, &state->regs.xss),
              "failed to write xss");

  LOG_DEBUG("segment registers written successfully");
  LOG_DEBUG("cs: 0x%d" PRIx64, state->regs.xcs);
  LOG_DEBUG("ds: 0x%d" PRIx64, state->regs.xds);
  LOG_DEBUG("es: 0x%d" PRIx64, state->regs.xes);
  LOG_DEBUG("fs: 0x%d" PRIx64, state->regs.xfs);
  LOG_DEBUG("gs: 0x%d" PRIx64, state->regs.xgs);
  LOG_DEBUG("ss: 0x%d" PRIx64, state->regs.xss);

  return 0;
}

uint64_t dump_i386_cpu_state(uc_engine *uc, linux_i386_process_state_t *state) {
  if (dump_gdt(uc, state) == -1) {
    return -1;
  }

  if (dump_regs(uc, state) == -1) {
    return -1;
  }

  uint64_t entry = -1;
  PB_UC_CHECK(uc_reg_read(uc, UC_X86_REG_EIP, &entry), "failed to read eip");
  return entry;
}
