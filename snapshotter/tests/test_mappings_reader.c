#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <linux/breakpoint.h>
#include <linux/mappings_reader.h>
#include <linux/maps_parser.h>

#include "ptrace_probe.h"
#include "test_util.h"

TEST(dump_child_mappings_round_trips) {
  if (!ptrace_traceme_is_usable()) {
    SKIP_RETURN("ptrace() is not usable in this environment");
  }

  const char *child_path = "/bin/true";
  char *argv[] = {(char *)child_path, NULL};
  extern char **environ;

  pid_t pid = fork_child(child_path, argv, environ);
  ASSERT_TRUE(pid > 0);

  const char *path = "/tmp/pb_test_mappings_dump.bin";

  FILE *out = fopen(path, "wb");
  ASSERT_TRUE(out != NULL);

  dump_mappings_content(pid, out);
  fclose(out);

  kill(pid, SIGKILL);
  waitpid(pid, NULL, 0);

  FILE *in = fopen(path, "rb");
  ASSERT_TRUE(in != NULL);

  size_t entries_read = 0;
  size_t bytes_content = 0;

  maps_entry_t entry;
  while (fread(&entry, sizeof(entry), 1, in) == 1) {
    ASSERT_TRUE(entry.start < entry.end);

    ASSERT_FALSE(strcmp(entry.path, "[vvar]") == 0);
    ASSERT_FALSE(strcmp(entry.path, "[vsyscall]") == 0);
    ASSERT_FALSE(strcmp(entry.path, "[vvar_vclock]") == 0);

    size_t size = entry.end - entry.start;

    ASSERT_TRUE(size < (100u * 1024 * 1024));

    char *content = malloc(size);
    ASSERT_TRUE(content != NULL);

    size_t got = fread(content, 1, size, in);
    ASSERT_EQ_INT(got, size);

    free(content);
    entries_read++;
    bytes_content += size;
  }

  ASSERT_FALSE(ferror(in));
  ASSERT_TRUE(entries_read > 2);
  ASSERT_TRUE(bytes_content > 0);

  fclose(in);
  unlink(path);
}

TEST_MAIN()
