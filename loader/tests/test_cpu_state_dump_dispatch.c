#include <string.h>

#include <cpu_state_dump.h>

#include <unicorn/unicorn.h>

#include <test_util.h>

#define X8664_USER_DS_INDEX     5
#define X8664_USER_CS_INDEX     6
#define I386_TRUE_USER_CS_INDEX 14
#define I386_TRUE_USER_DS_INDEX 15

static void map_process_stub(uc_engine *uc, uint64_t addr) {
  ASSERT_TRUE(uc_mem_map(uc, addr, 0x1000, UC_PROT_READ | UC_PROT_EXEC) ==
              UC_ERR_OK);
  uint8_t int3 = 0xCC;
  ASSERT_TRUE(uc_mem_write(uc, addr, &int3, sizeof(int3)) == UC_ERR_OK);
}

TEST(dump_cpu_state_dispatches_to_i386_for_mode_32) {
  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_32, &uc) == UC_ERR_OK);
  map_process_stub(uc, 0x08049000);

  cpu_state_t state;
  memset(&state, 0, sizeof(state));
  state.arch = UC_ARCH_X86;
  state.mode = UC_MODE_32;
  state.i386.regs.eip = 0x08049000;
  uint32_t data_selector = (I386_TRUE_USER_DS_INDEX << 3) | 3;
  state.i386.regs.xcs = (I386_TRUE_USER_CS_INDEX << 3) | 3;
  state.i386.regs.xss = data_selector;
  state.i386.regs.xds = data_selector;
  state.i386.regs.xes = data_selector;
  state.i386.regs.xfs = data_selector;
  state.i386.regs.xgs = data_selector;
  for (int i = 0; i < GDT_ENTRY_TLS_ENTRIES; ++i) {
    state.i386.tls[i].entry_number = 20 + i;
    state.i386.tls[i].seg_not_present = 1;
    state.i386.tls[i].seg_32bit = 1;
    state.i386.tls[i].limit_in_pages = 1;
    state.i386.tls[i].read_exec_only = 1;
  }

  uint64_t entry = dump_cpu_state(uc, &state);

  ASSERT_EQ_UINT64(entry, 0x08049000ULL);

  uint32_t eip;
  ASSERT_TRUE(uc_reg_read(uc, UC_X86_REG_EIP, &eip) == UC_ERR_OK);
  ASSERT_EQ_UINT64(eip, 0x08049000ULL);

  uc_close(uc);
}

TEST(dump_cpu_state_dispatches_to_x86_64_for_mode_64) {
  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_64, &uc) == UC_ERR_OK);

  cpu_state_t state;
  memset(&state, 0, sizeof(state));
  state.arch = UC_ARCH_X86;
  state.mode = UC_MODE_64;
  state.x86_64.regs.rip = 0xDEADBEEF;
  state.x86_64.regs.cs = (X8664_USER_CS_INDEX << 3) | 3;
  state.x86_64.regs.ss = (X8664_USER_DS_INDEX << 3) | 3;

  uint64_t entry = dump_cpu_state(uc, &state);

  ASSERT_EQ_UINT64(entry, 0xDEADBEEFULL);

  uint64_t rip;
  ASSERT_TRUE(uc_reg_read(uc, UC_X86_REG_RIP, &rip) == UC_ERR_OK);
  ASSERT_EQ_UINT64(rip, 0xDEADBEEFULL);

  uc_close(uc);
}

TEST(dump_cpu_state_rejects_unsupported_mode_without_touching_engine) {
  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_64, &uc) == UC_ERR_OK);

  cpu_state_t state;
  memset(&state, 0, sizeof(state));
  state.arch = UC_ARCH_X86;
  state.mode = UC_MODE_16;

  uint64_t entry = dump_cpu_state(uc, &state);

  ASSERT_EQ_UINT64(entry, (uint64_t)-1);

  uc_close(uc);
}

TEST_MAIN()
