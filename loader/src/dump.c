#include <cpu_state_dump.h>
#include <dump.h>
#include <linux/maps_entry.h>
#include <log.h>
#include <snapshot_variant.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <unicorn/unicorn.h>

static int read_bytes(const uint8_t *data, size_t file_size, size_t *offset,
                      void *dst, size_t size) {
  if (*offset > file_size || size > file_size - *offset)
    return -1;

  memcpy(dst, data + *offset, size);
  *offset += size;

  return 0;
}

uint64_t dump_snapshot(uc_engine *uc, const char *snapshot_path) {
  int fd = open(snapshot_path, O_RDONLY);
  if (fd == -1) {
    LOG_FATAL_ERRNO("failed to open snapshot file '%s'", snapshot_path);
    goto cleanup;
  }

  struct stat st;
  if (fstat(fd, &st) == -1) {
    LOG_FATAL_ERRNO("failed to stat snapshot file '%s'", snapshot_path);
    goto cleanup;
  }

  if (st.st_size == 0) {
    LOG_FATAL("snapshot file '%s' is empty", snapshot_path);
    goto cleanup;
  }
  size_t file_size = st.st_size;

  void *mapping = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
  size_t offset = 0;

  if (mapping == MAP_FAILED) {
    LOG_FATAL_ERRNO("failed to mmap snapshot file '%s'", snapshot_path);
    goto cleanup;
  }

  close(fd);
  fd = -1;

  const uint8_t *data = mapping;

  uc_arch arch;
  uc_mode mode;
  size_t value;

  uc_query(uc, UC_QUERY_ARCH, &value);
  arch = (uc_arch)value;

  uc_query(uc, UC_QUERY_MODE, &value);
  mode = (uc_mode)value;

  cpu_state_t cpu_state = {.arch = arch, .mode = mode};

  switch (mode) {
    case UC_MODE_32:
      if (read_bytes(
              data, file_size, &offset, &cpu_state.i386, sizeof(cpu_state.i386)
          ) != 0) {
        LOG_FATAL("snapshot is truncated while reading i386 process state");
        goto cleanup;
      }
      break;
    case UC_MODE_64:
      if (read_bytes(data,
                     file_size,
                     &offset,
                     &cpu_state.x86_64,
                     sizeof(cpu_state.x86_64)) != 0) {
        LOG_FATAL("snapshot is truncated while reading x86_64 process state");
        goto cleanup;
      }
      break;
    default:
      LOG_FATAL("unsupported mode %d", mode);
  }

  size_t mappings_loaded = 0;
  while (offset < file_size) {
    maps_entry_t entry_data;

    if (read_bytes(data, file_size, &offset, &entry_data, sizeof(entry_data)) !=
        0) {
      LOG_FATAL("snapshot is truncated while reading maps entry");
      goto cleanup;
    }

    LOG_DEBUG(
        "mapping %#lx-%#lx r=%u w=%u x=%u shared=%u offset=%#lx path='%s'",
        (unsigned long)entry_data.start,
        (unsigned long)entry_data.end,
        entry_data.read,
        entry_data.write,
        entry_data.exec,
        entry_data.shared,
        (unsigned long)entry_data.file_offset,
        entry_data.path
    );

    uint64_t mapping_size = entry_data.end - entry_data.start;

    if (mapping_size > file_size - offset) {
      LOG_FATAL(
          "mapping[%#lx - %#lx] claims %lu bytes,"
          "but only %zu remain in file",
          (unsigned long)entry_data.start,
          (unsigned long)entry_data.end,
          (unsigned long)mapping_size,
          file_size - offset
      );
      goto cleanup;
    }

    const void *content = data + offset;

    uint32_t perms = 0;

    if (entry_data.read) {
      perms |= UC_PROT_READ;
    }

    if (entry_data.write) {
      perms |= UC_PROT_WRITE;
    }

    if (entry_data.exec) {
      perms |= UC_PROT_EXEC;
    }

    uc_err err = uc_mem_map(uc, entry_data.start, mapping_size, UC_PROT_WRITE);
    if (err != UC_ERR_OK) {
      LOG_FATAL("failed to map mapping [%#lx-%#lx]: %s",
                (unsigned long)entry_data.start,
                (unsigned long)entry_data.end,
                uc_strerror(err));
      goto cleanup;
    }

    err = uc_mem_write(uc, entry_data.start, content, mapping_size);
    if (err != UC_ERR_OK) {
      LOG_FATAL("failed to load mapping [%#lx-%#lx]: %s",
                (unsigned long)entry_data.start,
                (unsigned long)entry_data.end,
                uc_strerror(err));
      goto cleanup;
    }

    err = uc_mem_protect(uc, entry_data.start, mapping_size, perms);
    if (err != UC_ERR_OK) {
      LOG_FATAL("failed to restore permissions for mapping [%#lx-%#lx]: %s",
                (unsigned long)entry_data.start,
                (unsigned long)entry_data.end,
                uc_strerror(err));
      goto cleanup;
    }

    offset += mapping_size;
    mappings_loaded++;
  }

  uint64_t entry = dump_cpu_state(uc, &cpu_state);
  LOG_DEBUG("loaded snapshot '%s': %zu mapping(s), %zu bytes total",
            snapshot_path,
            mappings_loaded,
            file_size);

cleanup:
  if (mapping != MAP_FAILED) {
    munmap(mapping, file_size);
  }

  if (fd != -1) {
    close(fd);
  }

  return entry;
}
