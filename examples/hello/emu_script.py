import process_bridge

from qiling.const import QL_ARCH, QL_VERBOSE

if __name__ == "__main__":
    ql, entry = process_bridge.make_qiling_from_snapshot(
        "ql_snapshot", QL_ARCH.X8664, verbose=QL_VERBOSE.DEBUG
    )
    ql.emu_start(begin=entry, end=0)
