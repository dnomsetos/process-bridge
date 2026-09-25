#include <unistd.h>

#include <linux/maps_parser.h>

#include "test_util.h"

TEST(parse_line_with_path_and_all_perms) {
  maps_entry_t entry;
  parse_maps_str(
      "55d1a2c00000-55d1a2c05000 r-xp 00001000 08:01 1234567 "
      "/usr/bin/process-bridge-x64\n",
      &entry);

  ASSERT_EQ_UINT64(entry.start, 0x55d1a2c00000ULL);
  ASSERT_EQ_UINT64(entry.end, 0x55d1a2c05000ULL);
  ASSERT_TRUE(entry.read);
  ASSERT_FALSE(entry.write);
  ASSERT_TRUE(entry.exec);
  ASSERT_FALSE(entry.shared);
  ASSERT_EQ_UINT64(entry.file_offset, 0x1000ULL);
  ASSERT_EQ_STR(entry.path, "/usr/bin/process-bridge-x64");
}

TEST(parse_line_rw_shared_no_path) {
  maps_entry_t entry;
  parse_maps_str("7f0000000000-7f0000021000 rw-s 00000000 00:00 0 \n", &entry);

  ASSERT_TRUE(entry.read);
  ASSERT_TRUE(entry.write);
  ASSERT_FALSE(entry.exec);
  ASSERT_TRUE(entry.shared);
  ASSERT_EQ_STR(entry.path, "");
}

TEST(parse_line_without_path_at_all) {
  maps_entry_t entry;
  parse_maps_str("00400000-00401000 r--p 00000000 08:01 999\n", &entry);

  ASSERT_TRUE(entry.read);
  ASSERT_FALSE(entry.write);
  ASSERT_FALSE(entry.exec);
  ASSERT_FALSE(entry.shared);
  ASSERT_EQ_STR(entry.path, "");
}

TEST(parse_line_pseudo_path_heap) {
  maps_entry_t entry;
  parse_maps_str("00602000-00623000 rw-p 00000000 00:00 0 [heap]\n", &entry);

  ASSERT_EQ_STR(entry.path, "[heap]");
  ASSERT_TRUE(entry.read);
  ASSERT_TRUE(entry.write);
}

TEST(parse_line_large_addresses_no_overflow) {
  maps_entry_t entry;
  parse_maps_str(
      "ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0 "
      "[vsyscall]\n",
      &entry);

  ASSERT_EQ_UINT64(entry.start, 0xffffffffff600000ULL);
  ASSERT_EQ_UINT64(entry.end, 0xffffffffff601000ULL);
  ASSERT_FALSE(entry.read);
  ASSERT_FALSE(entry.write);
  ASSERT_TRUE(entry.exec);
  ASSERT_EQ_STR(entry.path, "[vsyscall]");
}

TEST(parse_stale_path_is_cleared_between_calls) {
  maps_entry_t entry;
  parse_maps_str("10000-11000 r-xp 00000000 08:01 1 /usr/bin/something\n",
                 &entry);
  ASSERT_EQ_STR(entry.path, "/usr/bin/something");

  parse_maps_str("20000-21000 rw-p 00000000 00:00 0 \n", &entry);
  ASSERT_EQ_STR(entry.path, "");
}

TEST(iterate_own_process_maps_is_well_formed) {
  maps_iter_t iter;
  create_maps_iter(&iter, getpid());

  size_t count = 0;
  uint64_t prev_end = 0;

  for (const maps_entry_t *e = next(&iter); e != NULL; e = next(&iter)) {
    ASSERT_TRUE(e->start < e->end);
    ASSERT_TRUE(e->start >= prev_end);
    prev_end = e->end;
    count++;
  }

  ASSERT_TRUE(count > 3);
}

TEST_MAIN()
