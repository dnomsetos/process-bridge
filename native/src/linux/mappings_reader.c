#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>

#include <linux/mappings_reader.h>
#include <linux/maps_parser.h>

#define BUFFER_SIZE (1 << 20)

void dump_mappings_content(pid_t pid, FILE *file) {
  maps_iter_t iter;

  create_maps_iter(&iter, pid);

  char local_buffer[BUFFER_SIZE];

  struct iovec local = {
      .iov_base = local_buffer,
      .iov_len = sizeof(local_buffer),
  };

  struct iovec remote;

  for (const maps_entry_t *entry = next(&iter); entry != NULL;
       entry = next(&iter)) {
    if (strcmp(entry->path, "[vvar]") == 0 ||
        strcmp(entry->path, "[vsyscall]") == 0 ||
        strcmp(entry->path, "[vvar_vclock]") == 0) {
      continue;
    }

    if (fwrite(entry, sizeof(*entry), 1, file) != 1) {
      perror("fwrite");
      exit(EXIT_FAILURE);
    }

    size_t remaining = entry->end - entry->start;
    uintptr_t address = entry->start;

    while (remaining != 0) {
      size_t chunk = remaining > BUFFER_SIZE ? BUFFER_SIZE : remaining;

      remote.iov_base = (void *)address;
      remote.iov_len = chunk;
      local.iov_len = chunk;

      ssize_t result = process_vm_readv(pid, &local, 1, &remote, 1, 0);
      if (result == -1) {
        perror("process_vm_readv");
        exit(EXIT_FAILURE);
      }

      if ((size_t)result != chunk) {
        fprintf(stderr, "process_vm_readv: expected %zu bytes, got %zd\n",
                chunk, result);
        exit(EXIT_FAILURE);
      }

      if (fwrite(local_buffer, 1, chunk, file) != chunk) {
        perror("fwrite");
        exit(EXIT_FAILURE);
      }

      address += chunk;
      remaining -= chunk;
    }
  }

  if (fflush(file) == EOF) {
    perror("fflush");
    exit(EXIT_FAILURE);
  }
}

#undef BUFFER_SIZE
