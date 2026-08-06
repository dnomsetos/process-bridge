from ctypes import Structure, sizeof
from typing import TypeVar
from qiling import Qiling
import mmap

import arch
from maps_entry import MapsEntry, dump_mapping

T = TypeVar("T", bound=Structure)


def read_struct(cls: type[T], view: memoryview, offset: int) -> tuple[T, int]:
    size = sizeof(cls)
    print(f"{cls.__name__} {size}")
    if offset + size > len(view):
        raise EOFError(
            f"Expected {size} bytes for {cls.__name__} at offset {offset}, "
            + f"but only {len(view) - offset} remain"
        )
    obj = cls.from_buffer_copy(view[offset : offset + size])
    if hasattr(cls, "path"):
        print(obj.path)
    return obj, offset + size


def load_snapshot(ql: Qiling, snapshot_path: str):

    with open(snapshot_path, "rb") as file:
        mm = mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ)
        view = memoryview(mm)

        try:
            snapshot, offset = read_struct(arch.SnapshotInfo, view, 0)

            arch.dump_regs(ql, snapshot)

            while offset < len(view):
                entry, offset = read_struct(MapsEntry, view, offset)

                print(
                    hex(entry.start),
                    hex(entry.end),
                    entry.read,
                    entry.write,
                    entry.exec,
                    entry.shared,
                    hex(entry.file_offset),
                    str(entry.path),
                )

                size = entry.end - entry.start
                if offset + size > len(view):
                    raise EOFError(
                        f"Mapping [{entry.start:#x}-{entry.end:#x}] claims {size} bytes "
                        + f"of content, but only {len(view) - offset} remain in file"
                    )

                content = view[offset : offset + size]
                dump_mapping(ql, entry, content.tobytes())
                content.release()

                offset += size

            arch.finalize(ql, snapshot)
        except Exception as err:
            print(f"Error while loading snapshot:\n{err}")
            raise err
        finally:
            view.release()
            mm.close()
