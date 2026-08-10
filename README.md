# process-bridge

[![CI](https://github.com/dnomsetos/process-bridge/actions/workflows/ci.yml/badge.svg)](https://github.com/dnomsetos/process-bridge/actions/workflows/ci.yml) [![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A tool for snapshotting a native Linux process at a breakpoint (registers +
memory mappings) and restoring that state inside the [Qiling](https://qiling.io/)
emulator to continue execution there. It has two parts:

- `native/` — a C utility (`process-bridge-x64` / `process-bridge-x86`) that
  forks the target process, sets a breakpoint at an offset from the image
  base, runs the process until the breakpoint hits, and dumps a snapshot;
- `loader/process_bridge/` — a Python/Qiling wrapper that reads the dump and
  resumes execution inside the emulator.

## Requirements

- CMake ≥ 3.20 and `gcc`
- Python ≥ 3.10 and the `qiling` package (for the Python side)
- Linux — the native part relies on `ptrace(2)` and headers under
  `native/include/linux/`; it won't build on other platforms
  (`platform.h` deliberately fails the build there)

## Building

`CMakeLists.txt` builds **both** targets at once — 64-bit and 32-bit
(`add_arch_targets(x64 -m64)` and `add_arch_targets(x86 -m32)`) — so even a
plain build requires a compiler that can produce 32-bit code.

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j"$(nproc)"
```

After building, `build/` will contain four artifacts:

- `process-bridge-x64`, `libprocess-bridge-lib-x64.a`
- `process-bridge-x86`, `libprocess-bridge-lib-x86.a`

### Building the x86 target on an x86_64 host

Since the host is x86_64, `-m32` needs the 32-bit multilib versions of libc
and the headers — without them, `process-bridge-x86` fails to link.

```bash
# Debian/Ubuntu
sudo apt update
sudo apt install gcc-multilib g++-multilib

# a normal build now builds both targets
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j"$(nproc)"
```

## Usage (native part)

```bash
./build/process-bridge-x64 <breakpoint_offset_hex> <path-to-target-binary> [target-args...]
```

- `breakpoint_offset_hex` — the breakpoint's offset from the loaded image's
  base address, in hex, without a `0x` prefix
- `path-to-target-binary` — path to the executable to trace
- `target-args...` — optional arguments passed through to the target process

The utility forks and launches `target-binary`, sets a breakpoint at
`image_base + breakpoint_offset_hex`, runs the process up to that point, and
writes a snapshot to the `ql_snapshot` directory in the current working
directory.

For a 32-bit target, use `process-bridge-x86` the same way.

For a PIE binary, `breakpoint_offset_hex` is simply the symbol's virtual
address as reported by `nm`/`objdump -d` on the binary — see
[`examples/hello`](examples/hello) for a walkthrough.

## Usage (Python/Qiling part)

```bash
pip install .
process-bridge-x64   # run the compiled x64 binary through the installed wrapper
process-bridge-x86   # same, for x86
```

`pip install .` compiles the native binaries via CMake (through
`scikit-build-core`) and bundles them into the `process_bridge` package;
`process-bridge-x64` / `process-bridge-x86` on your `PATH` just forward their
arguments to the matching compiled binary.

For development, `pip install -e .` also works. Editable installs don't run
CMake's `install()` step into the package tree, so `process-bridge-x64` /
`process-bridge-x86` fall back to looking for the binaries in the plain
`build/` directory from the [Building](#building) section above — build the
project there first:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
pip install -e .
```

If you keep binaries somewhere else, point `PROCESS_BRIDGE_BIN_DIR` at that
directory instead.

Once a snapshot exists, load and resume it from Python with
`process_bridge.snaphot_init.from_snapshot(...)`:

```python
from process_bridge import snaphot_init

ql, entry = snaphot_init.from_snapshot("x86_64", "dummy_rootfs", "ql_snapshot", QL_VERBOSE.DEBUG)
ql.emu_start(begin=entry, end=...)
```

`rootfs_path` just needs to point at an existing (can be empty) directory —
Qiling requires one to initialize, but since every default mapping it creates
is unmapped and replaced by the snapshot's own mappings before execution
resumes, its contents don't matter here.

See [`examples/hello`](examples/hello) and [`examples/nginx`](examples/nginx)
for two complete, runnable examples.
