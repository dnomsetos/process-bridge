from process_bridge import snapshot_init
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
from qiling.log import QL_VERBOSE
from unicorn.unicorn import UcError

if __name__ == "__main__":
    ql, entry = snapshot_init.from_snapshot(
        "x86_64", "dummy_rootfs", "ql_snapshot", QL_VERBOSE.DEBUG
    )

    ql.emu_start(begin=entry, end=0)
