from qiling import Qiling
from qiling.const import QL_ARCH, QL_OS, QL_VERBOSE
from unicorn import UcError

from arch import get_arch_impl
from read_snapshot import load_snapshot


def from_snapshot(
    arch: str, rootfs_path: str, snapshot_path: str
) -> tuple[Qiling, int]:

    ql_arch = QL_ARCH.X8664 if arch == "x86_64" else QL_ARCH.X86

    ql = Qiling(
        code=b"\x90",
        archtype=ql_arch,
        ostype=QL_OS.LINUX,
        rootfs=rootfs_path,
        verbose=QL_VERBOSE.DEFAULT,
    )

    get_arch_impl(ql_arch)

    gdt_base = ql.os.gdtm.array.base

    for lbound, ubound, perms, label, *_ in list(ql.mem.map_info):
        if ql_arch == QL_ARCH.X86 and lbound == gdt_base:
            continue

        ql.mem.unmap(lbound, ubound - lbound)

    load_snapshot(ql, snapshot_path)

    entry = ql.arch.regs.rip if ql_arch == QL_ARCH.X8664 else ql.arch.regs.eip

    return ql, entry
