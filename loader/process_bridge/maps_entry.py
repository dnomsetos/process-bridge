from ctypes import Structure, c_uint64, c_bool, c_char
from qiling import Qiling
from qiling.const import QL_ARCH
from unicorn import UC_PROT_READ, UC_PROT_WRITE, UC_PROT_EXEC
import sys

import arch

MAX_PATH = 4096


class MapsEntry(Structure):
    start: int = 0
    end: int = 0
    read: bool = False
    write: bool = False
    exec: bool = False
    shared: bool = False
    file_offset: int = 0
    path: bytes = b"\x00" * 4096

    _pack_ = 8 if sys.argv[1] == "X8664" else 4
    _fields_ = [
        ("start", c_uint64),
        ("end", c_uint64),
        ("read", c_bool),
        ("write", c_bool),
        ("exec", c_bool),
        ("shared", c_bool),
        ("file_offset", c_uint64),
        ("path", c_char * MAX_PATH),
    ]


def to_uc_perms(entry: MapsEntry) -> int:
    p = 0
    if entry.read:
        p |= UC_PROT_READ
    if entry.write:
        p |= UC_PROT_WRITE
    if entry.exec:
        p |= UC_PROT_EXEC
    return p


def dump_mapping(ql: Qiling, entry: MapsEntry, content: bytes) -> None:
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

    perms = to_uc_perms(entry)
    size = entry.end - entry.start

    print(
        f"dump mapping: start={entry.start:#x} end={entry.end:#x} "
        f"size={size:#x} perms={perms} path={entry.path}"
    )

    try:
        ql.mem.map(entry.start, size, perms=perms)
    except:
        print(
            f"mem.map failed: start={entry.start:#x} end={entry.end:#x} "
            f"size={size:#x} perms={perms} path={entry.path} -> {e!r}"
        )
        raise

    try:
        ql.mem.write(entry.start, content)
    except Exception:
        print(f"mem.write failed: start={entry.start:#x} size={size:#x} -> {e!r}")
        raise
