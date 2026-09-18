from qiling import Qiling
from qiling.const import QL_ARCH, QL_OS, QL_VERBOSE
from unicorn import UcError

from .vas_cleaner import clean_vas
from .arch import get_arch_impl
from .read_snapshot import load_snapshot
from .log import get_logger

log = get_logger(__name__)

_SUPPORTED_ARCHES = {"x86_64": QL_ARCH.X8664, "i386": QL_ARCH.X86}


def from_snapshot(
    arch: str, rootfs_path: str, snapshot_path: str, ql_verbose: QL_VERBOSE
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
            verbose=ql_verbose,
        )
    except Exception:
        log.exception("failed to construct Qiling instance (rootfs=%r)", rootfs_path)
        raise

    get_arch_impl(ql_arch)

    unmapped = clean_vas(ql, ql_arch)
    log.debug("unmapped %d default qiling mapping(s)", unmapped)

    load_snapshot(ql, snapshot_path)

    entry = ql.arch.regs.rip if ql_arch == QL_ARCH.X8664 else ql.arch.regs.eip
    log.info("snapshot loaded, entry point = %#x", entry)

    return ql, entry
