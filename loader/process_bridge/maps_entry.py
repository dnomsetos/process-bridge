from __future__ import annotations

from ctypes import Structure, c_uint64, c_bool, c_char
from qiling import Qiling
from qiling.const import QL_ARCH
from unicorn import UC_PROT_READ, UC_PROT_WRITE, UC_PROT_EXEC

from . import arch
from .log import get_logger

log = get_logger(__name__)

MAX_PATH = 4096

_CLASS_CACHE: dict[int, type[Structure]] = {}


def _build_maps_entry_cls(pack: int) -> type[Structure]:
    class MapsEntry(Structure):
        _pack_ = pack
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

    return MapsEntry


def get_maps_entry_cls() -> type[Structure]:
    if arch.SNAPSHOT_ARCH is None:
        raise RuntimeError(
            "get_maps_entry_cls() called before arch.get_arch_impl() selected "
            "a target architecture"
        )

    pack = 8 if arch.SNAPSHOT_ARCH == QL_ARCH.X8664 else 4

    if pack not in _CLASS_CACHE:
        log.debug("building MapsEntry ctypes class with pack=%d", pack)
        _CLASS_CACHE[pack] = _build_maps_entry_cls(pack)

    return _CLASS_CACHE[pack]


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
    perms = to_uc_perms(entry)
    size = entry.end - entry.start

    log.debug(
        "mapping start=%#x end=%#x size=%#x perms=%d read=%s write=%s "
        "exec=%s shared=%s path=%r",
        entry.start,
        entry.end,
        size,
        perms,
        entry.read,
        entry.write,
        entry.exec,
        entry.shared,
        entry.path,
    )

    try:
        ql.mem.map(entry.start, size, perms=perms)
    except Exception:
        log.error(
            "mem.map failed: start=%#x end=%#x size=%#x perms=%d path=%r",
            entry.start,
            entry.end,
            size,
            perms,
            entry.path,
        )
        raise

    try:
        ql.mem.write(entry.start, content)
    except Exception:
        log.error(
            "mem.write failed: start=%#x size=%#x path=%r",
            entry.start,
            size,
            entry.path,
        )
        raise
