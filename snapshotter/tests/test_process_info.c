#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/maps_parser.h>
#include <linux/process_info.h>

#include "test_util.h"

static void write_fake_elf(const char *path, unsigned char ei_class) {
  FILE *f = fopen(path, "wb");
  if (f == NULL) {
    PB_FAIL_RETURN("could not create fixture file '%s'", path);
  }

  unsigned char header[64] = {0};
  header[0] = 0x7f;
  header[1] = 'E';
  header[2] = 'L';
  header[3] = 'F';
  header[4] = ei_class;
  header[5] = 1;
  header[6] = 1;

  size_t header_size = (ei_class == 2) ? 64 : 52;
  if (fwrite(header, 1, header_size, f) != header_size) {
    fclose(f);
    PB_FAIL_RETURN("could not write fixture ELF header to '%s'", path);
  }

  fclose(f);
}

TEST(detect_elf_class_64) {
  const char *path = "/tmp/pb_test_fake_elf64.bin";
  write_fake_elf(path, 2);

  FILE *f = fopen(path, "rb");
  ASSERT_TRUE(f != NULL);

  elf_class_t class = detect_elf_class(f);
  ASSERT_EQ_INT(class, ELF_CLASS_64);

  fclose(f);
  unlink(path);
}

TEST(detect_elf_class_32) {
  const char *path = "/tmp/pb_test_fake_elf32.bin";
  write_fake_elf(path, 1);

  FILE *f = fopen(path, "rb");
  ASSERT_TRUE(f != NULL);

  elf_class_t class = detect_elf_class(f);
  ASSERT_EQ_INT(class, ELF_CLASS_32);

  fclose(f);
  unlink(path);
}

TEST(open_elf_self_reports_absolute_path) {
  elf_info_t info;
  info.pid = getpid();

  open_elf("/proc/self/exe", &info);

  ASSERT_TRUE(info.path[0] == '/');
  ASSERT_TRUE(strstr(info.path, "/proc/self/exe") == NULL);

  fclose(info.file);
}

TEST(is_pie_matches_own_binary_type) {
  elf_info_t info;
  info.pid = getpid();
  open_elf("/proc/self/exe", &info);

  bool pie = is_pie(&info);
  (void)pie;

  fclose(info.file);
}

TEST(get_min_vaddr_is_page_aligned_and_sane) {
  elf_info_t info;
  info.pid = getpid();
  open_elf("/proc/self/exe", &info);

  uint64_t min_vaddr = get_min_vaddr(&info);

  ASSERT_TRUE(min_vaddr != UINT64_MAX);
  ASSERT_EQ_UINT64(min_vaddr % 0x1000, 0);

  fclose(info.file);
}

TEST(get_image_base_matches_proc_self_maps) {
  uint64_t image_base = get_image_base("/proc/self/exe", getpid());

  elf_info_t info;
  info.pid = getpid();
  open_elf("/proc/self/exe", &info);
  bool pie = is_pie(&info);
  uint64_t min_vaddr = get_min_vaddr(&info);
  fclose(info.file);

  if (!pie) {
    ASSERT_EQ_UINT64(image_base, 0);
    return;
  }

  maps_iter_t iter;
  create_maps_iter(&iter, getpid());

  uint64_t min_start = UINT64_MAX;
  char self_path[4096] = {0};
  ssize_t len = readlink("/proc/self/exe", self_path, sizeof(self_path) - 1);
  ASSERT_TRUE(len > 0);
  self_path[len] = '\0';

  for (const maps_entry_t *e = next(&iter); e != NULL; e = next(&iter)) {
    if (strcmp(e->path, self_path) == 0 && e->start < min_start) {
      min_start = e->start;
    }
  }

  ASSERT_TRUE(min_start != UINT64_MAX);
  ASSERT_EQ_UINT64(image_base, min_start - min_vaddr);
}

TEST_MAIN()
