from process_bridge import snaphot_init
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
    print(data)


if __name__ == "__main__":
    ql, entry = snaphot_init.from_snapshot(
        "x86_64", "dummy_rootfs", "ql_snapshot", QL_VERBOSE.DEBUG
    )
    try:
        ql.hook_address(callback=read_data, address=entry)
        ql.emu_start(begin=entry, end=0)
    except UcError as e:
        print(f"Emulation failed: {e}")
        pc = ql.arch.regs.arch_pc

        code = ql.mem.read(pc, 16)

        print(f"PC: 0x{pc:x}")
        print("Bytes:", code.hex(" "))

        md = Cs(CS_ARCH_X86, CS_MODE_64)

        for insn in md.disasm(code, pc):
            print(f"0x{insn.address:x}: {insn.mnemonic} {insn.op_str}")
