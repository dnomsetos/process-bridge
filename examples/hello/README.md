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
pip install -e .
```

Then run the x86_64 native component:

```bash
/path/to/process-bridge/build/snapshotter-x86_64-release/snapshotter/pb-snapshotter 0x114d hello
```

This starts `hello`, stops it immediately before the `call repeat` instruction,
and writes the snapshot to:

```text
ql_snapshot
```

To enable detailed diagnostic output, set the logging level through
`PROCESS_BRIDGE_LOG_LEVEL`:

```bash
PROCESS_BRIDGE_LOG_LEVEL=DEBUG /path/to/process-bridge/build/snapshotter-x86_64-release/snapshotter/pb-snapshotter 0x114d ./hello

```

## 4. Resume execution in Qiling

`emu_script.py` loads the snapshot and resumes execution from the restored
instruction pointer:

```python
import process_bridge

from qiling.const import QL_ARCH, QL_VERBOSE

if __name__ == "__main__":
    ql, entry = process_bridge.make_qiling_from_snapshot(
        "ql_snapshot", QL_ARCH.X8664, verbose=QL_VERBOSE.DEBUG
    )
    ql.emu_start(begin=entry, end=0)
```

`entry` is the address restored from the process's `rip` and points to the
`call repeat` instruction.

After `make_qiling_from_snapshot`, you can use the `Qiling` object as usual.
