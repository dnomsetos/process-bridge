from ctypes import Structure, sizeof
from typing import TypeVar
from qiling import Qiling
import mmap

from . import arch
from . import maps_entry as maps_entry_mod
from .maps_entry import dump_mapping
from .log import get_logger

log = get_logger(__name__)

T = TypeVar("T", bound=Structure)


def read_struct(cls: type[T], view: memoryview, offset: int) -> tuple[T, int]:
    size = sizeof(cls)
    if offset + size > len(view):
        raise EOFError(
            f"Expected {size} bytes for {cls.__name__} at offset {offset}, "
            f"but only {len(view) - offset} remain"
        )
    obj = cls.from_buffer_copy(view[offset : offset + size])
    log.debug("read %s (%d bytes) at offset %#x", cls.__name__, size, offset)
    return obj, offset + size


def load_snapshot(ql: Qiling, snapshot_path: str) -> None:
    try:
        file = open(snapshot_path, "rb")
    except OSError as err:
        raise OSError(f"failed to open snapshot file '{snapshot_path}': {err}") from err

    with file:
        try:
            mm = mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ)
        except (OSError, ValueError) as err:
            raise OSError(
                f"failed to mmap snapshot file '{snapshot_path}' "
                f"(is it empty or corrupt?): {err}"
            ) from err

        view = memoryview(mm)

        try:
            snapshot, offset = read_struct(arch.SnapshotInfo, view, 0)

            arch.dump_regs(ql, snapshot)

            maps_entry_cls = maps_entry_mod.get_maps_entry_cls()

            mappings_loaded = 0

            while offset < len(view):
                entry, offset = read_struct(maps_entry_cls, view, offset)

                log.debug(
                    "mapping %#x-%#x r=%s w=%s x=%s shared=%s offset=%#x path=%r",
                    entry.start,
                    entry.end,
                    entry.read,
                    entry.write,
                    entry.exec,
                    entry.shared,
                    entry.file_offset,
                    entry.path,
                )

                size = entry.end - entry.start
                if offset + size > len(view):
                    raise EOFError(
                        f"Mapping [{entry.start:#x}-{entry.end:#x}] claims {size} "
                        f"bytes of content, but only {len(view) - offset} remain "
                        "in file (snapshot file looks truncated or corrupt)"
                    )

                content = view[offset : offset + size]
                try:
                    dump_mapping(ql, entry, content.tobytes())
                finally:
                    content.release()

                offset += size
                mappings_loaded += 1

            arch.finalize(ql, snapshot)

            log.info(
                "loaded snapshot '%s': %d mapping(s), %d bytes total",
                snapshot_path,
                mappings_loaded,
                len(view),
            )
        except Exception:
            log.exception("failed to load snapshot '%s'", snapshot_path)
            raise
        finally:
            view.release()
            mm.close()
