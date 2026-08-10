from process_bridge import snaphot_init
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
from unicorn.unicorn import UcError

if __name__ == "__main__":
    ql, entry = snaphot_init.from_snapshot("x86_64", "dummy_rootfs", "ql_snapshot")
    try:
        ql.emu_start(begin=entry, end=entry + 5)
    except UcError as e:
        print(f"Emulation failed: {e}")
        pc = ql.arch.regs.arch_pc

        code = ql.mem.read(pc, 16)

        print(f"PC: 0x{pc:x}")
        print("Bytes:", code.hex(" "))

        md = Cs(CS_ARCH_X86, CS_MODE_64)

        for insn in md.disasm(code, pc):
            print(f"0x{insn.address:x}: {insn.mnemonic} {insn.op_str}")
