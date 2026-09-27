#include <inttypes.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>

#include <linux/mappings_reader.h>
#include <linux/maps_parser.h>
#include <log.h>

#define BUFFER_SIZE (1 << 20)

void dump_mappings_content(pid_t pid, FILE *file) {
  maps_iter_t iter;

  create_maps_iter(&iter, pid);

  char *local_buffer = malloc(BUFFER_SIZE);
  if (local_buffer == NULL) {
    LOG_FATAL_ERRNO("malloc(%d) for the mapping read buffer failed",
                    BUFFER_SIZE);
  }

  struct iovec local = {
      .iov_base = local_buffer,
      .iov_len = BUFFER_SIZE,
  };

  struct iovec remote;
  size_t mappings_dumped = 0;
  size_t mappings_skipped = 0;

  for (const maps_entry_t *entry = next(&iter); entry != NULL;
       entry = next(&iter)) {
    if (strcmp(entry->path, "[vvar]") == 0 ||
        strcmp(entry->path, "[vsyscall]") == 0 ||
        strcmp(entry->path, "[vvar_vclock]") == 0) {
      LOG_DEBUG("skipping special mapping '%s' (0x%" PRIx64 "-0x%" PRIx64 ")",
                entry->path, entry->start, entry->end);
      continue;
    }

    if (fwrite(entry, sizeof(*entry), 1, file) != 1) {
      LOG_FATAL_ERRNO("fwrite of maps_entry_t header failed");
    }

    size_t remaining = entry->end - entry->start;
    uintptr_t address = entry->start;

    if (!entry->read) {
      LOG_DEBUG("mapping 0x%" PRIx64 "-0x%" PRIx64
                " (%s) has no read permission, writing zeros",
                entry->start, entry->end, entry->path);
      memset(local_buffer, 0,
             BUFFER_SIZE > remaining ? remaining : BUFFER_SIZE);
    }

    while (remaining != 0) {
      size_t chunk = remaining > BUFFER_SIZE ? BUFFER_SIZE : remaining;

      if (entry->read) {
        remote.iov_base = (void *)address;
        remote.iov_len = chunk;
        local.iov_len = chunk;

        ssize_t result = process_vm_readv(pid, &local, 1, &remote, 1, 0);

        if (result == -1 || (size_t)result != chunk) {
          if (result == -1) {
            LOG_ERRNO(LOG_LEVEL_WARN,
                      "process_vm_readv(pid=%d, addr=%p, len=%zu) failed", pid,
                      (void *)address, chunk);
          } else {
            LOG_WARN(
                "process_vm_readv(pid=%d, addr=%p) returned %zd bytes, "
                "expected %zu",
                pid, (void *)address, result, chunk);
          }

          memset(local_buffer, 0, chunk);
          mappings_skipped++;
        }
      }

      if (fwrite(local_buffer, 1, chunk, file) != chunk) {
        LOG_FATAL_ERRNO("fwrite of %zu bytes of mapping content failed", chunk);
      }

      address += chunk;
      remaining -= chunk;
    }

    mappings_dumped++;
  }

  if (fflush(file) == EOF) {
    LOG_ERRNO(LOG_LEVEL_ERROR, "fflush of the snapshot file failed");
    free(local_buffer);
    exit(EXIT_FAILURE);
  }

  LOG_INFO("dumped %zu mapping(s) (%zu chunk(s) fell back to zeros)",
           mappings_dumped, mappings_skipped);

  free(local_buffer);
}

#undef BUFFER_SIZE
