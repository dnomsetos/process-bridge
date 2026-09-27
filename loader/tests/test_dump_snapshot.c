#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <dump.h>
#include <linux/i386/process_state.h>
#include <linux/maps_entry.h>
#include <linux/x86_64/process_state.h>

#include <unicorn/unicorn.h>

#include <fatal_probe.h>
#include <test_util.h>

#define X8664_USER_DS_INDEX     5
#define X8664_USER_CS_INDEX     6
#define I386_TRUE_USER_CS_INDEX 14
#define I386_TRUE_USER_DS_INDEX 15

#define PB_TEST_PAGE_SIZE 0x1000

static void write_all(FILE *f, const void *data, size_t size) {
  ASSERT_TRUE(fwrite(data, 1, size, f) == size);
}

static linux_x86_64_process_state_t make_valid_x8664_state(uint64_t rip) {
  linux_x86_64_process_state_t state;
  memset(&state, 0, sizeof(state));

  state.regs.rip = rip;
  state.regs.eflags = 0x202;
  state.regs.cs = (X8664_USER_CS_INDEX << 3) | 3;
  state.regs.ss = (X8664_USER_DS_INDEX << 3) | 3;

  return state;
}

static linux_i386_process_state_t make_valid_i386_state(uint32_t eip) {
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
    state.tls[i].seg_not_present = 1;
    state.tls[i].seg_32bit = 1;
    state.tls[i].limit_in_pages = 1;
    state.tls[i].read_exec_only = 1;
  }

  return state;
}

static void write_mapping_entry(FILE *f, uint64_t start, uint64_t end, int r,
                                int w, int x) {
  maps_entry_t entry;
  memset(&entry, 0, sizeof(entry));
  entry.start = start;
  entry.end = end;
  entry.read = r ? 1 : 0;
  entry.write = w ? 1 : 0;
  entry.exec = x ? 1 : 0;
  entry.shared = 0;
  entry.file_offset = 0;
  entry.path[0] = '\0';

  write_all(f, &entry, sizeof(entry));
}

static bool region_perms(uc_engine *uc, uint64_t addr, uint32_t *out_perms) {
  uc_mem_region *regions = NULL;
  uint32_t count = 0;
  if (uc_mem_regions(uc, &regions, &count) != UC_ERR_OK) {
    return false;
  }

  bool found = false;
  for (uint32_t i = 0; i < count; ++i) {
    if (regions[i].begin == addr) {
      *out_perms = regions[i].perms;
      found = true;
      break;
    }
  }

  uc_free(regions);
  return found;
}

TEST(dump_snapshot_loads_x8664_state_and_single_mapping) {
  const char *path = "/tmp/pb_test_dump_snapshot_x8664_single.bin";

  uint8_t content[PB_TEST_PAGE_SIZE];
  for (size_t i = 0; i < sizeof(content); ++i) {
    content[i] = (uint8_t)i;
  }

  FILE *f = fopen(path, "wb");
  ASSERT_TRUE(f != NULL);

  linux_x86_64_process_state_t state = make_valid_x8664_state(0x400000);
  write_all(f, &state, sizeof(state));
  write_mapping_entry(f, 0x400000, 0x400000 + PB_TEST_PAGE_SIZE, 1, 0, 1);
  write_all(f, content, sizeof(content));
  fclose(f);

  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_64, &uc) == UC_ERR_OK);

  uint64_t entry = dump_snapshot(uc, path);
  ASSERT_EQ_UINT64(entry, 0x400000ULL);

  uint8_t readback[PB_TEST_PAGE_SIZE];
  ASSERT_TRUE(uc_mem_read(uc, 0x400000, readback, sizeof(readback)) ==
              UC_ERR_OK);
  ASSERT_TRUE(memcmp(readback, content, sizeof(content)) == 0);

  uint32_t perms = 0;
  ASSERT_TRUE(region_perms(uc, 0x400000, &perms));
  ASSERT_EQ_UINT64(perms, (uint64_t)(UC_PROT_READ | UC_PROT_EXEC));

  uc_close(uc);
  unlink(path);
}

TEST(dump_snapshot_loads_i386_state_and_returns_eip) {
  const char *path = "/tmp/pb_test_dump_snapshot_i386_single.bin";

  uint8_t content[PB_TEST_PAGE_SIZE] = {0};
  content[0] = 0xCC;

  FILE *f = fopen(path, "wb");
  ASSERT_TRUE(f != NULL);

  linux_i386_process_state_t state = make_valid_i386_state(0x08048000);
  write_all(f, &state, sizeof(state));
  write_mapping_entry(f, 0x08048000, 0x08048000 + PB_TEST_PAGE_SIZE, 1, 0, 1);
  write_all(f, content, sizeof(content));
  fclose(f);

  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_32, &uc) == UC_ERR_OK);

  uint64_t entry = dump_snapshot(uc, path);
  ASSERT_EQ_UINT64(entry, 0x08048000ULL);

  uint32_t eip;
  ASSERT_TRUE(uc_reg_read(uc, UC_X86_REG_EIP, &eip) == UC_ERR_OK);
  ASSERT_EQ_UINT64(eip, 0x08048000ULL);

  uint32_t perms = 0;
  ASSERT_TRUE(region_perms(uc, 0x08048000, &perms));
  ASSERT_EQ_UINT64(perms, (uint64_t)(UC_PROT_READ | UC_PROT_EXEC));

  uc_close(uc);
  unlink(path);
}

TEST(dump_snapshot_loads_multiple_mappings_with_distinct_perms) {
  const char *path = "/tmp/pb_test_dump_snapshot_x8664_multi.bin";

  uint8_t content_a[PB_TEST_PAGE_SIZE];
  uint8_t content_b[PB_TEST_PAGE_SIZE];
  memset(content_a, 0xAA, sizeof(content_a));
  memset(content_b, 0xBB, sizeof(content_b));

  FILE *f = fopen(path, "wb");
  ASSERT_TRUE(f != NULL);

  linux_x86_64_process_state_t state = make_valid_x8664_state(0x400000);
  write_all(f, &state, sizeof(state));

  write_mapping_entry(f, 0x400000, 0x400000 + PB_TEST_PAGE_SIZE, 1, 0, 1);
  write_all(f, content_a, sizeof(content_a));

  write_mapping_entry(f, 0x500000, 0x500000 + PB_TEST_PAGE_SIZE, 1, 1, 0);
  write_all(f, content_b, sizeof(content_b));

  fclose(f);

  uc_engine *uc = NULL;
  ASSERT_TRUE(uc_open(UC_ARCH_X86, UC_MODE_64, &uc) == UC_ERR_OK);

  dump_snapshot(uc, path);

  uint8_t readback[PB_TEST_PAGE_SIZE];

  ASSERT_TRUE(uc_mem_read(uc, 0x400000, readback, sizeof(readback)) ==
              UC_ERR_OK);
  ASSERT_TRUE(memcmp(readback, content_a, sizeof(content_a)) == 0);

  ASSERT_TRUE(uc_mem_read(uc, 0x500000, readback, sizeof(readback)) ==
              UC_ERR_OK);
  ASSERT_TRUE(memcmp(readback, content_b, sizeof(content_b)) == 0);

  uint32_t perms = 0;
  ASSERT_TRUE(region_perms(uc, 0x400000, &perms));
  ASSERT_EQ_UINT64(perms, (uint64_t)(UC_PROT_READ | UC_PROT_EXEC));

  ASSERT_TRUE(region_perms(uc, 0x500000, &perms));
  ASSERT_EQ_UINT64(perms, (uint64_t)(UC_PROT_READ | UC_PROT_WRITE));

  uc_close(uc);
  unlink(path);
}

static void probe_missing_file(void *arg) {
  (void)arg;
  uc_engine *uc = NULL;
  uc_open(UC_ARCH_X86, UC_MODE_64, &uc);
  dump_snapshot(uc, "/tmp/pb_test_dump_snapshot_does_not_exist.bin");
}

TEST(dump_snapshot_exits_fatal_when_file_is_missing) {
  ASSERT_TRUE(expect_fatal_exit(probe_missing_file, NULL));
}

static void probe_empty_file(void *arg) {
  (void)arg;
  const char *path = "/tmp/pb_test_dump_snapshot_empty.bin";
  FILE *f = fopen(path, "wb");
  if (f != NULL) {
    fclose(f);
  }

  uc_engine *uc = NULL;
  uc_open(UC_ARCH_X86, UC_MODE_64, &uc);
  dump_snapshot(uc, path);
}

TEST(dump_snapshot_exits_fatal_when_file_is_empty) {
  ASSERT_TRUE(expect_fatal_exit(probe_empty_file, NULL));
  unlink("/tmp/pb_test_dump_snapshot_empty.bin");
}

static void probe_truncated_state(void *arg) {
  (void)arg;
  const char *path = "/tmp/pb_test_dump_snapshot_truncated_state.bin";
  FILE *f = fopen(path, "wb");
  if (f != NULL) {
    uint8_t partial[4] = {0};
    fwrite(partial, 1, sizeof(partial), f);
    fclose(f);
  }

  uc_engine *uc = NULL;
  uc_open(UC_ARCH_X86, UC_MODE_64, &uc);
  dump_snapshot(uc, path);
}

TEST(dump_snapshot_exits_fatal_when_process_state_is_truncated) {
  ASSERT_TRUE(expect_fatal_exit(probe_truncated_state, NULL));
  unlink("/tmp/pb_test_dump_snapshot_truncated_state.bin");
}

static void probe_truncated_maps_entry(void *arg) {
  (void)arg;
  const char *path = "/tmp/pb_test_dump_snapshot_truncated_entry.bin";
  FILE *f = fopen(path, "wb");
  if (f != NULL) {
    linux_x86_64_process_state_t state = make_valid_x8664_state(0x400000);
    fwrite(&state, 1, sizeof(state), f);

    uint8_t partial_entry[8] = {0};
    fwrite(partial_entry, 1, sizeof(partial_entry), f);
    fclose(f);
  }

  uc_engine *uc = NULL;
  uc_open(UC_ARCH_X86, UC_MODE_64, &uc);
  dump_snapshot(uc, path);
}

TEST(dump_snapshot_exits_fatal_when_maps_entry_is_truncated) {
  ASSERT_TRUE(expect_fatal_exit(probe_truncated_maps_entry, NULL));
  unlink("/tmp/pb_test_dump_snapshot_truncated_entry.bin");
}

static void probe_truncated_mapping_content(void *arg) {
  (void)arg;
  const char *path = "/tmp/pb_test_dump_snapshot_truncated_content.bin";
  FILE *f = fopen(path, "wb");
  if (f != NULL) {
    linux_x86_64_process_state_t state = make_valid_x8664_state(0x400000);
    fwrite(&state, 1, sizeof(state), f);

    write_mapping_entry(f, 0x400000, 0x400000 + PB_TEST_PAGE_SIZE, 1, 0, 1);

    uint8_t short_content[16] = {0};
    fwrite(short_content, 1, sizeof(short_content), f);
    fclose(f);
  }

  uc_engine *uc = NULL;
  uc_open(UC_ARCH_X86, UC_MODE_64, &uc);
  dump_snapshot(uc, path);
}

TEST(dump_snapshot_exits_fatal_when_mapping_content_is_truncated) {
  ASSERT_TRUE(expect_fatal_exit(probe_truncated_mapping_content, NULL));
  unlink("/tmp/pb_test_dump_snapshot_truncated_content.bin");
}

static void probe_misaligned_mapping(void *arg) {
  (void)arg;
  const char *path = "/tmp/pb_test_dump_snapshot_misaligned.bin";
  FILE *f = fopen(path, "wb");
  if (f != NULL) {
    linux_x86_64_process_state_t state = make_valid_x8664_state(0x400000);
    fwrite(&state, 1, sizeof(state), f);

    write_mapping_entry(f, 0x400001, 0x400001 + PB_TEST_PAGE_SIZE, 1, 0, 1);

    uint8_t content[PB_TEST_PAGE_SIZE] = {0};
    fwrite(content, 1, sizeof(content), f);
    fclose(f);
  }

  uc_engine *uc = NULL;
  uc_open(UC_ARCH_X86, UC_MODE_64, &uc);
  dump_snapshot(uc, path);
}

TEST(dump_snapshot_exits_fatal_when_mapping_is_not_page_aligned) {
  ASSERT_TRUE(expect_fatal_exit(probe_misaligned_mapping, NULL));
  unlink("/tmp/pb_test_dump_snapshot_misaligned.bin");
}

TEST_MAIN()
