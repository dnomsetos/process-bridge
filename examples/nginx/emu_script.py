import process_bridge

from qiling import Qiling
from qiling.const import QL_VERBOSE, QL_ARCH


def read_data(ql: Qiling) -> None:
    b_ptr = ql.arch.regs.rsi

    pos = ql.mem.read_ptr(b_ptr)
    last = ql.mem.read_ptr(b_ptr + 0x08)

    data = ql.mem.read(pos, last - pos)
    print(f"data:\n {data}")


if __name__ == "__main__":
    ql, entry = process_bridge.make_qiling_from_snapshot(
        "ql_snapshot", QL_ARCH.X8664, verbose=QL_VERBOSE.DEBUG
    )

    read_data(ql)

    ql.emu_start(begin=entry, end=0)
