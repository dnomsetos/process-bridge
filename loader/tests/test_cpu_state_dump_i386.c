#include <stdbool.h>
#include <string.h>

#include <gdt_utils.h>
#include <i386/cpu_state_dump.h>
#include <linux/i386/process_state.h>

#include <unicorn/unicorn.h>
#include <unicorn/x86.h>

#include <fatal_probe.h>
#include <test_util.h>

#define I386_COMPAT_USER_CS_INDEX 4
#define I386_TRUE_USER_CS_INDEX   14
#define I386_TRUE_USER_DS_INDEX   15

#define TARGET_EIP 0x08049000ULL

static linux_i386_process_state_t make_valid_state(uint32_t eip) {
  linux_i386_process_state_t state;
  memset(&state, 0, sizeof(state));

  state.regs.eip = eip;
  state.regs.eflags = 0x202;

  uint32_t data_selector = (I386_TRUE_USER_DS_INDEX << 3) | 3;
  state.regs.xcs = (I386_TRUE_USER_CS_INDEX << 3) | 3;
  state.regs.xss = data_selector;
  state.regs.xds = data_selector;
  state.regs.xes = data_selector;
  state.regs.xfs = data_selector;
  state.regs.xgs = data_selector;

  for (int i = 0; i < GDT_ENTRY_TLS_ENTRIES; ++i) {
    state.tls[i].entry_number = 20 + i;
    state.tls[i].base_addr = 0x1000 * (i + 1);
    state.tls[i].limit = 0xFFFFF;
    state.tls[i].seg_32bit = 1;
    state.tls[i].contents = 0;
    state.tls[i].read_exec_only = 1;
    state.tls[i].limit_in_pages = 1;
    state.tls[i].seg_not_present = 1;
    state.tls[i].useable = 1;
  }

  return state;
}

static void map_process_stub(uc_engine *uc, uint64_t addr) {
  ASSERT_TRUE(uc_mem_map(uc, addr, 0x1000, UC_PROT_READ | UC_PROT_EXEC) ==
              UC_ERR_OK);
  uint8_t int3 = 0xCC;
  ASSERT_TRUE(uc_mem_write(uc, addr, &int3, sizeof(int3)) == UC_ERR_OK);
}

static uint64_t read_gdt_qword(uc_engine *uc, int index) {
  uint8_t buf[8] = {0};
  uc_mem_read(uc, PB_GDT_ADDR + (uint64_t)index * 8, buf, sizeof(buf));

  uint64_t value;
  memcpy(&value, buf, sizeof(value));
  return value;
}

TEST(dump_i386_cpu_state_returns_eip_as_entry_point) {
  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_32, &uc) == UC_ERR_OK);
  map_process_stub(uc, TARGET_EIP);

  linux_i386_process_state_t state = make_valid_state(TARGET_EIP);

  uint64_t entry = dump_i386_cpu_state(uc, &state);

  ASSERT_EQ_UINT64(entry, TARGET_EIP);

  uc_close(uc);
}

TEST(dump_i386_cpu_state_does_not_execute_past_the_entry_point) {
  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_32, &uc) == UC_ERR_OK);
  map_process_stub(uc, TARGET_EIP);

  linux_i386_process_state_t state = make_valid_state(TARGET_EIP);
  dump_i386_cpu_state(uc, &state);

  uint32_t eip;
  ASSERT_TRUE(uc_reg_read(uc, UC_X86_REG_EIP, &eip) == UC_ERR_OK);
  ASSERT_EQ_UINT64(eip, TARGET_EIP);

  uc_close(uc);
}

TEST(dump_i386_cpu_state_restores_general_purpose_registers) {
  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_32, &uc) == UC_ERR_OK);
  map_process_stub(uc, TARGET_EIP);

  linux_i386_process_state_t state = make_valid_state(TARGET_EIP);
  state.regs.eax = 0xAAAAAAAA;
  state.regs.ebx = 0xBBBBBBBB;
  state.regs.ecx = 0xCCCCCCCC;
  state.regs.edx = 0xDDDDDDDD;
  state.regs.esi = 0x11111111;
  state.regs.edi = 0x22222222;
  state.regs.ebp = 0x33333333;
  state.regs.esp = 0xFFFFE000;

  dump_i386_cpu_state(uc, &state);

  uint32_t value;

#define ASSERT_REG(reg, expected)                             \
  do {                                                        \
    ASSERT_TRUE(uc_reg_read(uc, (reg), &value) == UC_ERR_OK); \
    ASSERT_EQ_UINT64(value, (expected));                      \
  } while (0)

  ASSERT_REG(UC_X86_REG_EAX, state.regs.eax);
  ASSERT_REG(UC_X86_REG_EBX, state.regs.ebx);
  ASSERT_REG(UC_X86_REG_ECX, state.regs.ecx);
  ASSERT_REG(UC_X86_REG_EDX, state.regs.edx);
  ASSERT_REG(UC_X86_REG_ESI, state.regs.esi);
  ASSERT_REG(UC_X86_REG_EDI, state.regs.edi);
  ASSERT_REG(UC_X86_REG_EBP, state.regs.ebp);
  ASSERT_REG(UC_X86_REG_ESP, state.regs.esp);
  ASSERT_REG(UC_X86_REG_EIP, state.regs.eip);
  ASSERT_REG(UC_X86_REG_EFLAGS, state.regs.eflags);
  ASSERT_REG(UC_X86_REG_CS, state.regs.xcs);
  ASSERT_REG(UC_X86_REG_SS, state.regs.xss);
  ASSERT_REG(UC_X86_REG_DS, state.regs.xds);
  ASSERT_REG(UC_X86_REG_ES, state.regs.xes);
  ASSERT_REG(UC_X86_REG_FS, state.regs.xfs);
  ASSERT_REG(UC_X86_REG_GS, state.regs.xgs);

#undef ASSERT_REG

  uc_close(uc);
}

TEST(dump_i386_cpu_state_writes_default_code_and_data_descriptors) {
  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_32, &uc) == UC_ERR_OK);
  map_process_stub(uc, TARGET_EIP);

  linux_i386_process_state_t state = make_valid_state(TARGET_EIP);
  dump_i386_cpu_state(uc, &state);

  uint64_t expected_code_desc =
      GDT_ENTRY_INIT(DESC_CODE32 | DESC_USER, 0, 0xfffff);
  uint64_t expected_data_desc =
      GDT_ENTRY_INIT(DESC_DATA32 | DESC_USER, 0, 0xfffff);

  ASSERT_EQ_UINT64(read_gdt_qword(uc, I386_TRUE_USER_CS_INDEX),
                   expected_code_desc);
  ASSERT_EQ_UINT64(read_gdt_qword(uc, I386_TRUE_USER_DS_INDEX),
                   expected_data_desc);

  uc_close(uc);
}

TEST(dump_i386_cpu_state_uses_data64_descriptor_for_compatibility_cs) {
  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_32, &uc) == UC_ERR_OK);
  map_process_stub(uc, TARGET_EIP);

  linux_i386_process_state_t state = make_valid_state(TARGET_EIP);
  uint32_t data_selector = (I386_TRUE_USER_DS_INDEX << 3) | 3;
  state.regs.xcs = (I386_COMPAT_USER_CS_INDEX << 3) | 3;
  state.regs.xss = data_selector;

  dump_i386_cpu_state(uc, &state);

  uint64_t expected_data_desc =
      GDT_ENTRY_INIT(DESC_DATA64 | DESC_USER, 0, 0xfffff);
  ASSERT_EQ_UINT64(read_gdt_qword(uc, I386_TRUE_USER_DS_INDEX),
                   expected_data_desc);

  uc_close(uc);
}

TEST(dump_i386_cpu_state_writes_all_tls_descriptors) {
  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_32, &uc) == UC_ERR_OK);
  map_process_stub(uc, TARGET_EIP);

  linux_i386_process_state_t state = make_valid_state(TARGET_EIP);
  dump_i386_cpu_state(uc, &state);

  for (int i = 0; i < GDT_ENTRY_TLS_ENTRIES; ++i) {
    uint64_t expected = USER_DESC_TO_REAL_DESC(state.tls[i]);
    ASSERT_EQ_UINT64(read_gdt_qword(uc, state.tls[i].entry_number), expected);
  }

  uc_close(uc);
}

TEST(dump_i386_cpu_state_protects_gdt_page_read_only) {
  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_32, &uc) == UC_ERR_OK);
  map_process_stub(uc, TARGET_EIP);

  linux_i386_process_state_t state = make_valid_state(TARGET_EIP);
  dump_i386_cpu_state(uc, &state);

  uc_mem_region *regions = NULL;
  uint32_t count = 0;
  ASSERT_TRUE(uc_mem_regions(uc, &regions, &count) == UC_ERR_OK);

  bool found = false;
  for (uint32_t i = 0; i < count; ++i) {
    if (regions[i].begin == PB_GDT_ADDR) {
      found = true;
      ASSERT_EQ_UINT64(regions[i].perms, (uint64_t)UC_PROT_READ);
    }
  }
  ASSERT_TRUE(found);

  uc_free(regions);
  uc_close(uc);
}

TEST(dump_i386_cpu_state_unmaps_the_ring3_trampoline_scratch_page) {
  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_32, &uc) == UC_ERR_OK);
  map_process_stub(uc, TARGET_EIP);

  linux_i386_process_state_t state = make_valid_state(TARGET_EIP);
  dump_i386_cpu_state(uc, &state);

  uc_mem_region *regions = NULL;
  uint32_t count = 0;
  ASSERT_TRUE(uc_mem_regions(uc, &regions, &count) == UC_ERR_OK);

  bool leaked = false;
  for (uint32_t i = 0; i < count; ++i) {
    if (regions[i].begin < PB_GDT_ADDR) {
      leaked = true;
    }
  }
  uc_free(regions);

  ASSERT_TRUE(!leaked);

  uc_close(uc);
}

TEST(dump_i386_cpu_state_sets_gdtr_base_and_limit) {
  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_32, &uc) == UC_ERR_OK);
  map_process_stub(uc, TARGET_EIP);

  linux_i386_process_state_t state = make_valid_state(TARGET_EIP);
  dump_i386_cpu_state(uc, &state);

  uc_x86_mmr gdtr;
  ASSERT_TRUE(uc_reg_read(uc, UC_X86_REG_GDTR, &gdtr) == UC_ERR_OK);
  ASSERT_EQ_UINT64(gdtr.base, PB_GDT_ADDR);
  ASSERT_EQ_UINT64(gdtr.limit, PB_GDT_SIZE - 1);

  uc_close(uc);
}

static void probe_rejects_ldt_cs(void *arg) {
  (void)arg;
  uc_engine *uc = NULL;
  uc_open(UC_ARCH_X86, UC_MODE_32, &uc);

  linux_i386_process_state_t state = make_valid_state(TARGET_EIP);
  state.regs.xcs |= 0x4;

  dump_i386_cpu_state(uc, &state);
}

TEST(dump_i386_cpu_state_rejects_ldt_selectors) {
  ASSERT_TRUE(expect_fatal_exit(probe_rejects_ldt_cs, NULL));
}

static void probe_rejects_non_default_cs(void *arg) {
  (void)arg;
  uc_engine *uc = NULL;
  uc_open(UC_ARCH_X86, UC_MODE_32, &uc);

  linux_i386_process_state_t state = make_valid_state(TARGET_EIP);
  state.regs.xcs = (2 << 3) | 3;

  dump_i386_cpu_state(uc, &state);
}

TEST(dump_i386_cpu_state_rejects_non_default_user_cs) {
  ASSERT_TRUE(expect_fatal_exit(probe_rejects_non_default_cs, NULL));
}

TEST_MAIN()
