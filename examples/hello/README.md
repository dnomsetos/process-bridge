# hello

The simplest possible walkthrough: snapshot a tiny process at the entry of a
function, then resume it inside Qiling and let it run to completion.

`hello.c` prints `before repeat`, calls `repeat()` (which prints `hello` five
times), then prints `after repeat`:

```c
void repeat() {
  for (int i = 0; i < 5; ++i) {
    printf("hello\n");
  }
}

int main() {
  printf("before repeat\n");
  repeat();
  printf("after repeat\n");
}
```

We'll set the breakpoint at the very first instruction of `repeat`, snapshot
the process there, and resume it inside the emulator — so `repeat` (and the
final `printf`) actually run inside Qiling, not natively.

## 1. Build the target

```bash
cc -fPIE -pie -g -o hello hello.c
```

## 2. Find the breakpoint offset

`process-bridge` wants the breakpoint as an offset from the image's base
address. For a PIE binary that's just the symbol's virtual address as `nm`
reports it:

```bash
nm hello | grep ' repeat$'
# e.g.: 0000000000001149 T repeat
```

Here the offset is `1149`.

## 3. Take the snapshot

From the `build/` directory of the main project (or wherever
`process-bridge-x64` was built/installed):

```bash
process-bridge-x64 1149 /path/to/examples/hello/hello
```

This runs `hello`, stops it right as `repeat` is entered, and writes a
snapshot to `./ql_snapshot`.

## 4. Resume it in Qiling

`emu_script.py` loads that snapshot and emulates from `repeat`'s entry point
for a few instructions:

```python
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
```

`entry` is `repeat`'s address, taken straight from the restored `rip`.
`emu_start(begin=entry, end=entry + 5)` intentionally runs only a handful of
bytes, not a real address — it's there mostly to show the failure path: if
Unicorn faults (e.g. hits unmapped memory or an unsupported instruction), the
`except` block prints the faulting address and disassembles the bytes around
it, which is handy for debugging snapshot/restore issues rather than for
running `hello` to a meaningful stopping point.

`rootfs_path` (`"dummy_rootfs"`) just needs to exist as a directory — Qiling
requires one to initialize, but its contents are irrelevant here since every
mapping it creates by default gets unmapped and replaced by the snapshot.

```bash
mkdir -p dummy_rootfs
python3 emu_script.py
```
