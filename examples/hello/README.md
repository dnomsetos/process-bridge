# hello

The simplest possible walkthrough: snapshot a small native process immediately
before a function call, then resume it inside Qiling.

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

The breakpoint is placed on the `call repeat` instruction. The process is
snapshotted immediately before entering `repeat`, and execution is then
resumed from that instruction inside Qiling.

## 1. Build the target

Build the example with Clang, explicitly specifying the target architecture
and disabling optimizations:

```bash
clang --target=x86_64-pc-linux-gnu -O0 -o hello hello.c
```

The `-O0` option is important because the example relies on the generated
instruction sequence remaining predictable.

For an i386 target, use:

```bash
clang --target=i386-pc-linux-gnu -O0 -o hello hello.c
```

## 2. Find the `call repeat` instruction

`process-bridge` expects the breakpoint as an offset from the image base.

In this example, the breakpoint should be placed on the `call repeat`
instruction rather than on the first instruction of `repeat`.

Use `objdump` to locate it:

```bash
objdump -D hello | grep repeat
```

For example:

```text
114d: e8 ed ff ff ff    call   113f <repeat>
```

In this case, the breakpoint offset is `114d`.

The exact address will depend on the compiler and the generated binary.

## 3. Take the snapshot

Install `process-bridge` from the root of the project:

```bash
pip install .
```

Then run the x86_64 native component:

```bash
process-bridge-x86_64-linux 0x114d hello
```

This starts `hello`, stops it immediately before the `call repeat` instruction,
and writes the snapshot to:

```text
ql_snapshot
```

To enable detailed diagnostic output, set the logging level through
`PROCESS_BRIDGE_LOG_LEVEL`:

```bash
PROCESS_BRIDGE_LOG_LEVEL=DEBUG process-bridge-x86_64-linux 0x114d ./hello
```

## 4. Resume execution in Qiling

`emu_script.py` loads the snapshot and resumes execution from the restored
instruction pointer:

```python
from process_bridge import snaphot_init

if __name__ == "__main__":
  ql, entry = snaphot_init.from_snapshot(
    "x86_64",
    "dummy_rootfs",
    "ql_snapshot",
  )

  ql.emu_start(begin=entry, end=0)
```

`entry` is the address restored from the process's `rip` and points to the
`call repeat` instruction.

The example intentionally uses:

```python
ql.emu_start(begin=entry, end=entry + 5)
```

This limits emulation to the small address range containing the `call`
instruction. The goal is to demonstrate that execution resumes from the exact
instruction at which the native process was snapshotted.

The required Qiling `rootfs` does not need to contain anything for this
example. `dummy_rootfs` only needs to exist as a directory because Qiling
requires a rootfs during initialization. The default mappings created by
Qiling are replaced with the mappings restored from the snapshot.

Create the directory and run the script:

```bash
mkdir dummy_rootfs
python3 emu_script.py
```

The example demonstrates the complete flow:

```text
native process
     │
     │ call repeat
     ▼
  snapshot
     │
     │ restore
     ▼
   Qiling
     │
     ▼
resumed execution
```

