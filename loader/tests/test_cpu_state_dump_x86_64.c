#include <stdbool.h>
#include <string.h>

#include <gdt_utils.h>
#include <linux/x86_64/process_state.h>
#include <x86_64/cpu_state_dump.h>

#include <unicorn/unicorn.h>
#include <unicorn/x86.h>

#include <fatal_probe.h>
#include <test_util.h>

#define X8664_USER_DS_INDEX 5
#define X8664_USER_CS_INDEX 6

static linux_x86_64_process_state_t make_valid_state(uint64_t rip) {
  linux_x86_64_process_state_t state;
  memset(&state, 0, sizeof(state));

  state.regs.rip = rip;
  state.regs.eflags = 0x202;
  state.regs.cs = (X8664_USER_CS_INDEX << 3) | 3;
  state.regs.ss = (X8664_USER_DS_INDEX << 3) | 3;
  state.regs.ds = 0;
  state.regs.es = 0;
  state.regs.fs = 0;
  state.regs.gs = 0;
  state.regs.fs_base = 0x7F0000000000ULL;
  state.regs.gs_base = 0x7F0000001000ULL;

  return state;
}

static uint64_t read_gdt_qword(uc_engine *uc, int index) {
  uint8_t buf[8] = {0};
  uc_mem_read(uc, PB_GDT_ADDR + (uint64_t)index * 8, buf, sizeof(buf));

  uint64_t value;
  memcpy(&value, buf, sizeof(value));
  return value;
}

TEST(dump_x86_64_cpu_state_returns_rip_as_entry_point) {
  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_64, &uc) == UC_ERR_OK);

  linux_x86_64_process_state_t state = make_valid_state(0xDEADBEEF);

  uint64_t entry = dump_x86_64_cpu_state(uc, &state);

  ASSERT_EQ_UINT64(entry, 0xDEADBEEFULL);

  uc_close(uc);
}

TEST(dump_x86_64_cpu_state_restores_general_purpose_registers) {
  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_64, &uc) == UC_ERR_OK);

  linux_x86_64_process_state_t state = make_valid_state(0xDEADBEEF);
  state.regs.rax = 0x1111;
  state.regs.rbx = 0x2222;
  state.regs.rcx = 0x3333;
  state.regs.rdx = 0x4444;
  state.regs.rsi = 0x5555;
  state.regs.rdi = 0x6666;
  state.regs.rbp = 0x7777;
  state.regs.rsp = 0x8888;
  state.regs.r8 = 0x9;
  state.regs.r9 = 0xA;
  state.regs.r10 = 0xB;
  state.regs.r11 = 0xC;
  state.regs.r12 = 0xD;
  state.regs.r13 = 0xE;
  state.regs.r14 = 0xF;
  state.regs.r15 = 0x10;

  dump_x86_64_cpu_state(uc, &state);

  uint64_t value;

#define ASSERT_REG(reg, expected)                             \
  do {                                                        \
    ASSERT_TRUE(uc_reg_read(uc, (reg), &value) == UC_ERR_OK); \
    ASSERT_EQ_UINT64(value, (expected));                      \
  } while (0)

  ASSERT_REG(UC_X86_REG_RAX, state.regs.rax);
  ASSERT_REG(UC_X86_REG_RBX, state.regs.rbx);
  ASSERT_REG(UC_X86_REG_RCX, state.regs.rcx);
  ASSERT_REG(UC_X86_REG_RDX, state.regs.rdx);
  ASSERT_REG(UC_X86_REG_RSI, state.regs.rsi);
  ASSERT_REG(UC_X86_REG_RDI, state.regs.rdi);
  ASSERT_REG(UC_X86_REG_RBP, state.regs.rbp);
  ASSERT_REG(UC_X86_REG_RSP, state.regs.rsp);
  ASSERT_REG(UC_X86_REG_R8, state.regs.r8);
  ASSERT_REG(UC_X86_REG_R9, state.regs.r9);
  ASSERT_REG(UC_X86_REG_R10, state.regs.r10);
  ASSERT_REG(UC_X86_REG_R11, state.regs.r11);
  ASSERT_REG(UC_X86_REG_R12, state.regs.r12);
  ASSERT_REG(UC_X86_REG_R13, state.regs.r13);
  ASSERT_REG(UC_X86_REG_R14, state.regs.r14);
  ASSERT_REG(UC_X86_REG_R15, state.regs.r15);
  ASSERT_REG(UC_X86_REG_RIP, state.regs.rip);
  ASSERT_REG(UC_X86_REG_EFLAGS, state.regs.eflags);
  ASSERT_REG(UC_X86_REG_CS, state.regs.cs);
  ASSERT_REG(UC_X86_REG_SS, state.regs.ss);
  ASSERT_REG(UC_X86_REG_DS, state.regs.ds);
  ASSERT_REG(UC_X86_REG_ES, state.regs.es);
  ASSERT_REG(UC_X86_REG_FS, state.regs.fs);
  ASSERT_REG(UC_X86_REG_GS, state.regs.gs);
  ASSERT_REG(UC_X86_REG_FS_BASE, state.regs.fs_base);
  ASSERT_REG(UC_X86_REG_GS_BASE, state.regs.gs_base);

#undef ASSERT_REG

  uc_close(uc);
}

TEST(dump_x86_64_cpu_state_writes_default_code_and_data_descriptors) {
  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_64, &uc) == UC_ERR_OK);

  linux_x86_64_process_state_t state = make_valid_state(0xDEADBEEF);
  dump_x86_64_cpu_state(uc, &state);

  uint64_t expected_code_desc =
      GDT_ENTRY_INIT(DESC_CODE64 | DESC_USER, 0, 0xfffff);
  uint64_t expected_data_desc =
      GDT_ENTRY_INIT(DESC_DATA64 | DESC_USER, 0, 0xfffff);

  ASSERT_EQ_UINT64(read_gdt_qword(uc, X8664_USER_CS_INDEX), expected_code_desc);
  ASSERT_EQ_UINT64(read_gdt_qword(uc, X8664_USER_DS_INDEX), expected_data_desc);

  uc_close(uc);
}

TEST(dump_x86_64_cpu_state_accepts_ds_es_pointing_at_default_user_ds) {
  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_64, &uc) == UC_ERR_OK);

  linux_x86_64_process_state_t state = make_valid_state(0xDEADBEEF);
  state.regs.ds = (X8664_USER_DS_INDEX << 3) | 3;
  state.regs.es = (X8664_USER_DS_INDEX << 3) | 3;

  uint64_t entry = dump_x86_64_cpu_state(uc, &state);

  ASSERT_EQ_UINT64(entry, 0xDEADBEEFULL);

  uc_close(uc);
}

TEST(dump_x86_64_cpu_state_protects_gdt_page_read_only) {
  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_64, &uc) == UC_ERR_OK);

  linux_x86_64_process_state_t state = make_valid_state(0xDEADBEEF);
  dump_x86_64_cpu_state(uc, &state);

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

static void probe_rejects_ldt_selector(void *arg) {
  (void)arg;
  uc_engine *uc = NULL;
  uc_open(UC_ARCH_X86, UC_MODE_64, &uc);

  linux_x86_64_process_state_t state = make_valid_state(0xDEADBEEF);
  state.regs.ds |= 0x4;

  dump_x86_64_cpu_state(uc, &state);
}

TEST(dump_x86_64_cpu_state_rejects_ldt_selectors) {
  ASSERT_TRUE(expect_fatal_exit(probe_rejects_ldt_selector, NULL));
}

static void probe_rejects_non_default_cs(void *arg) {
  (void)arg;
  uc_engine *uc = NULL;
  uc_open(UC_ARCH_X86, UC_MODE_64, &uc);

  linux_x86_64_process_state_t state = make_valid_state(0xDEADBEEF);
  state.regs.cs = (2 << 3) | 3;

  dump_x86_64_cpu_state(uc, &state);
}

TEST(dump_x86_64_cpu_state_rejects_non_default_user_cs) {
  ASSERT_TRUE(expect_fatal_exit(probe_rejects_non_default_cs, NULL));
}

static void probe_rejects_non_default_ss(void *arg) {
  (void)arg;
  uc_engine *uc = NULL;
  uc_open(UC_ARCH_X86, UC_MODE_64, &uc);

  linux_x86_64_process_state_t state = make_valid_state(0xDEADBEEF);
  state.regs.ss = (2 << 3) | 3;

  dump_x86_64_cpu_state(uc, &state);
}

TEST(dump_x86_64_cpu_state_rejects_non_default_user_ss) {
  ASSERT_TRUE(expect_fatal_exit(probe_rejects_non_default_ss, NULL));
}

static void probe_rejects_bad_ds(void *arg) {
  (void)arg;
  uc_engine *uc = NULL;
  uc_open(UC_ARCH_X86, UC_MODE_64, &uc);

  linux_x86_64_process_state_t state = make_valid_state(0xDEADBEEF);
  state.regs.ds = (2 << 3) | 3;

  dump_x86_64_cpu_state(uc, &state);
}

TEST(dump_x86_64_cpu_state_rejects_ds_outside_null_or_default_user_ds) {
  ASSERT_TRUE(expect_fatal_exit(probe_rejects_bad_ds, NULL));
}

static void probe_rejects_bad_es(void *arg) {
  (void)arg;
  uc_engine *uc = NULL;
  uc_open(UC_ARCH_X86, UC_MODE_64, &uc);

  linux_x86_64_process_state_t state = make_valid_state(0xDEADBEEF);
  state.regs.es = (2 << 3) | 3;

  dump_x86_64_cpu_state(uc, &state);
}

TEST(dump_x86_64_cpu_state_rejects_es_outside_null_or_default_user_ds) {
  ASSERT_TRUE(expect_fatal_exit(probe_rejects_bad_es, NULL));
}

TEST_MAIN()
