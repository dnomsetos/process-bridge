from typing import Callable, Optional

from qiling import Qiling
from qiling.const import QL_ARCH

from .x86_64 import user_regs_struct as x8664
from .x86 import user_regs_struct as x86
from .log import get_logger

log = get_logger(__name__)

SnapshotInfo: type | None = None
dump_regs: Callable[[Qiling, object], None] | None = None
finalize: Callable[[Qiling, object], None] | None = None
SNAPSHOT_ARCH: QL_ARCH | None = None


def get_arch_impl(target_arch: QL_ARCH) -> None:
    match target_arch:
        case QL_ARCH.X8664:
            globals()["SnapshotInfo"] = x8664.SnapshotInfo
            globals()["dump_regs"] = x8664.dump_regs
            globals()["finalize"] = lambda ql, snapshot: None
            globals()["SNAPSHOT_ARCH"] = QL_ARCH.X8664
        case QL_ARCH.X86:
            globals()["SnapshotInfo"] = x86.SnapshotInfo
            globals()["dump_regs"] = x86.dump_regs
            globals()["finalize"] = x86.restore_tls
            globals()["SNAPSHOT_ARCH"] = QL_ARCH.X86
        case _:
            raise ValueError(
                f"unsupported architecture: {target_arch!r} "
                "(only QL_ARCH.X8664 and QL_ARCH.X86 are supported)"
            )

    log.info("selected architecture implementation: %s", target_arch)
