from qiling import Qiling
from qiling.const import QL_VERBOSE, QL_OS

from ._native import restore_snapshot


def make_qiling_from_snapshot(
    snapshot_path: str,
    archtype,
    rootfs: str = ".",
    verbose: QL_VERBOSE = QL_VERBOSE.DEFAULT,
) -> tuple[Qiling, int]:
    ql = Qiling(
        code=b"\x90",
        rootfs=rootfs,
        ostype=QL_OS.LINUX,
        archtype=archtype,
        verbose=verbose,
    )

    for lbound, ubound, _, label, *_ in list(ql.mem.map_info):
        ql.mem.unmap(lbound, ubound - lbound)

    entry = restore_snapshot(ql.arch.uc, snapshot_path)
    return ql, entry
