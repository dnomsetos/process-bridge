from process_bridge import snapshot_init
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
from unicorn.unicorn import UcError
from qiling import Qiling
from qiling.const import QL_VERBOSE
from typing import Any


def read_data(ql: Qiling) -> None:
    b_ptr = ql.arch.regs.rsi

    pos = ql.mem.read_ptr(b_ptr)
    last = ql.mem.read_ptr(b_ptr + 0x08)

    data = ql.mem.read(pos, last - pos)
    print(f"data:\n {data}")


if __name__ == "__main__":
    ql, entry = snapshot_init.from_snapshot(
        "x86_64", "dummy_rootfs", "ql_snapshot", QL_VERBOSE.DEBUG
    )

    read_data(ql)

    ql.emu_start(begin=entry, end=0)
