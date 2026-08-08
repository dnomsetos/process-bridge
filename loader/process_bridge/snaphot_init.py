from qiling import Qiling
from qiling.const import QL_ARCH, QL_OS, QL_VERBOSE
from unicorn import UcError

from .arch import get_arch_impl
from .read_snapshot import load_snapshot
from .log import get_logger

log = get_logger(__name__)

_SUPPORTED_ARCHES = {"x86_64": QL_ARCH.X8664, "x86": QL_ARCH.X86}


def from_snapshot(
    arch: str, rootfs_path: str, snapshot_path: str
) -> tuple[Qiling, int]:
    try:
        ql_arch = _SUPPORTED_ARCHES[arch]
    except KeyError:
        raise ValueError(
            f"unsupported arch {arch!r}, expected one of "
            f"{sorted(_SUPPORTED_ARCHES)}"
        ) from None

    log.info(
        "initializing Qiling for arch=%s rootfs=%r snapshot=%r",
        arch,
        rootfs_path,
        snapshot_path,
    )

    try:
        ql = Qiling(
            code=b"\x90",
            archtype=ql_arch,
            ostype=QL_OS.LINUX,
            rootfs=rootfs_path,
            verbose=QL_VERBOSE.DEFAULT,
        )
    except Exception:
        log.exception("failed to construct Qiling instance (rootfs=%r)", rootfs_path)
        raise

    get_arch_impl(ql_arch)

    gdt_base = ql.os.gdtm.array.base

    unmapped = 0
    for lbound, ubound, perms, label, *_ in list(ql.mem.map_info):
        if ql_arch == QL_ARCH.X86 and lbound == gdt_base:
            log.debug("keeping GDT mapping at %#x, not unmapping", lbound)
            continue

        try:
            ql.mem.unmap(lbound, ubound - lbound)
            unmapped += 1
        except UcError:
            log.exception(
                "failed to unmap qiling's default mapping [%#x-%#x] (%s) "
                "before loading the snapshot",
                lbound,
                ubound,
                label,
            )
            raise

    log.debug("unmapped %d default qiling mapping(s)", unmapped)

    load_snapshot(ql, snapshot_path)

    entry = ql.arch.regs.rip if ql_arch == QL_ARCH.X8664 else ql.arch.regs.eip

    log.info("snapshot loaded, entry point = %#x", entry)

    return ql, entry
