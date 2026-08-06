from qiling.const import QL_ARCH
import x86_64.user_regs_struct as x8664
import x86.user_regs_struct as x86


def get_arch_impl(arch: QL_ARCH):
    match arch:
        case QL_ARCH.X8664:
            globals()["SnapshotInfo"] = x8664.SnapshotInfo
            globals()["dump_regs"] = x8664.dump_regs
            globals()["finalize"] = lambda x, y: None
            globals()["SNAPSHOT_ARCH"] = QL_ARCH.X8664
        case QL_ARCH.X86:
            globals()["SnapshotInfo"] = x86.SnapshotInfo
            globals()["dump_regs"] = x86.dump_regs
            globals()["finalize"] = x86.restore_tls
            globals()["SNAPSHOT_ARCH"] = QL_ARCH.X86
        case _:
            print("Unsupported platform")
            exit(1)
