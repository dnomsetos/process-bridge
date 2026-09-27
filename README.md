# process-bridge

[![CI](https://github.com/dnomsetos/process-bridge/actions/workflows/ci.yml/badge.svg)](https://github.com/dnomsetos/process-bridge/actions/workflows/ci.yml) [![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

**process-bridge** snapshots the execution state of a native Linux process and restores that state in an emulator.

The project consists of two main components:

* **snapshotter** — a native C program that launches and traces a target process, stops it at a breakpoint, captures its CPU state and memory mappings, and writes a snapshot;
* **loader** — a native C library that restores a snapshot into [Unicorn](https://www.unicorn-engine.org/).

The Python package provides a `ctypes` wrapper around the loader and a helper for creating a [Qiling](https://qiling.io/) instance from a snapshot.

The main use case is to reach a difficult execution state natively, capture it once, and then reproduce it in an emulator for debugging, analysis, instrumentation, or fuzzing.

## Supported architectures

* `x86_64`
* `i386`

The snapshotter must be built for the architecture of the target process.

## Requirements

`process-bridge` requires Linux.

Native components:

* Clang
* CMake >= 3.20
* Ninja

Python package:

* Python >= 3.10
* `unicorn==2.1.4`
* `qiling==1.4.6`

Qiling is only required when using the Python Qiling helper. The native loader uses Unicorn directly.

For building the 32-bit snapshotter on Debian/Ubuntu:

```bash
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install libc6-dev:i386 gcc-multilib g++-multilib
```

A development environment is also provided by [`Dockerfile`](Dockerfile).

## Installation

The Python package builds the native loader automatically through `scikit-build-core`:

```bash
python -m venv .venv
source .venv/bin/activate

python -m pip install --upgrade pip
python -m pip install .
```

The package exposes:

```python
import process_bridge

process_bridge.restore_snapshot(...)
process_bridge.make_qiling_from_snapshot(...)
```

The snapshotter is built separately with CMake.

## Building from source

### Snapshotter

Build the x86_64 release version:

```bash
cmake --preset snapshotter-x86_64-release
cmake --build --preset snapshotter-x86_64-release
```

The executable is produced at:

```text
build/snapshotter-x86_64-release/snapshotter/pb-snapshotter
```

For i386:

```bash
cmake --preset snapshotter-i386-release
cmake --build --preset snapshotter-i386-release
```

Debug presets enable AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
cmake --preset snapshotter-x86_64-debug
cmake --build --preset snapshotter-x86_64-debug
```

### Loader

Build the loader directly:

```bash
cmake --preset loader-release
cmake --build --preset loader-release
```

The resulting library is:

```text
build/loader-release/loader/libpb-loader.so
```

The Python package builds and installs the loader automatically.

## Taking a snapshot

The snapshotter interface is:

```text
pb-snapshotter <breakpoint_offset_hex> <child_path> [child_args...]
```

Example:

```bash
./build/snapshotter-x86_64-release/snapshotter/pb-snapshotter \
    0x114d \
    ./hello
```

The snapshotter:

1. starts the target under `ptrace`;
2. determines the target image base;
3. places an `INT3` breakpoint at `image_base + breakpoint_offset_hex`;
4. resumes execution until the breakpoint is reached;
5. captures the CPU state;
6. reads `/proc/<pid>/maps` and the target memory;
7. writes the snapshot to `ql_snapshot`;
8. terminates the target.

### Choosing the breakpoint

The breakpoint is an **offset from the loaded image base**, not necessarily an absolute runtime address.

For PIE executables, use the virtual address of the desired instruction in the ELF image:

```bash
objdump -d ./hello
nm ./hello
```

A useful snapshot point is often the instruction where execution should resume. For example, placing the breakpoint on a `call` instruction captures the state immediately before the call, including arguments already prepared by the caller.

See [`examples/hello`](examples/hello) and [`examples/nginx`](examples/nginx) for complete examples.

## Snapshot contents

A snapshot contains:

* integer CPU registers;
* process memory mappings;
* the contents of captured mappings;
* mapping permissions, sharing information, file offset, and path.

For `i386`, it also contains the TLS descriptors required to restore the captured CPU state.

Special mappings such as `[vvar]`, `[vvar_vclock]`, and `[vsyscall]` are skipped. If a readable mapping cannot be copied successfully, the affected chunks are replaced with zeroes.

## Restoring a snapshot with Unicorn

The Python API restores a snapshot into an existing `unicorn.Uc` instance:

```python
import unicorn
import process_bridge

uc = unicorn.Uc(unicorn.UC_ARCH_X86, unicorn.UC_MODE_64)

entry = process_bridge.restore_snapshot(
    uc,
    "ql_snapshot",
)

uc.emu_start(
    begin=entry,
    end=0,
)
```

`restore_snapshot()` returns the restored instruction pointer. Memory mappings and CPU registers are restored before execution resumes.

The native loader exposes:

```c
uint64_t pb_restore_snapshot(
    uc_engine *uc,
    const char *snapshot_path
);
```

It also provides:

```c
uint64_t pb_create_from_snapshot(
    uc_arch arch,
    uc_mode mode,
    uc_engine **engine,
    const char *snapshot_path
);
```

Supported Unicorn configurations:

* `UC_ARCH_X86 + UC_MODE_32`
* `UC_ARCH_X86 + UC_MODE_64`

## Restoring a snapshot with Qiling

The Python package includes a Qiling helper:

```python
import process_bridge
from qiling.const import QL_ARCH, QL_VERBOSE

ql, entry = process_bridge.make_qiling_from_snapshot(
    "ql_snapshot",
    QL_ARCH.X8664,
    verbose=QL_VERBOSE.DEBUG,
)

ql.emu_start(
    begin=entry,
    end=0,
)
```

The helper removes Qiling's initial memory mappings and replaces them with the mappings restored from the snapshot.

Qiling still requires a rootfs. For the current workflow, an empty directory is sufficient:

```bash
mkdir -p dummy_rootfs
```

## Instruction compatibility

The restored process may contain CPU instructions that are not supported by the Unicorn version used by the emulator. Applications using AVX2, AVX-512, or instructions such as `XSAVEC` may therefore fail during emulation.

In such cases, it can be useful to disable CPU features when starting the target process. For example:

```bash
GLIBC_TUNABLES=glibc.cpu.hwcaps=-XSAVEC,-AVX,-AVX2,-AVX512 \
./build/snapshotter-x86_64-release/snapshotter/pb-snapshotter \
    <breakpoint_offset_hex> \
    <path-to-target-binary> \
    [target-args...]
```

This makes glibc avoid using the specified CPU features when selecting optimized implementations, which can prevent unsupported instructions from being executed in the restored process.

`GLIBC_TUNABLES` only affects glibc's CPU feature selection. It does not add support for unsupported instructions to Unicorn.

## Logging

The native components use `PROCESS_BRIDGE_LOG_LEVEL`.

Supported levels:

* `DEBUG`
* `INFO`
* `WARN`
* `ERROR`

The default is `INFO`.

Example:

```bash
PROCESS_BRIDGE_LOG_LEVEL=DEBUG \
./build/snapshotter-x86_64-release/snapshotter/pb-snapshotter \
    0x114d ./hello
```

## Project layout

```text
.
├── common/                  Shared Linux data structures
├── snapshotter/             Native process snapshotter
├── loader/                  Unicorn-based snapshot loader
├── python/process_bridge/   Python bindings and Qiling helper
├── examples/                Example workflows
├── utilities/               Shared native utilities
├── CMakeLists.txt
├── CMakePresets.json
├── Dockerfile
└── pyproject.toml
```

## Testing

Snapshotter tests:

```bash
cmake --preset snapshotter-x86_64-debug
cmake --build --preset snapshotter-x86_64-debug
ctest --preset snapshotter-x86_64-debug --output-on-failure
```

Loader tests:

```bash
cmake --preset loader-debug
cmake --build --preset loader-debug
ctest --preset loader-debug --output-on-failure
```

The debug presets enable AddressSanitizer and UndefinedBehaviorSanitizer.

## Docker

Build the development image:

```bash
docker build -t process-bridge .
```

Run it with the project mounted into `/workspace`:

```bash
docker run --rm -it \
    -v "$PWD:/workspace" \
    process-bridge
```

## Limitations

The snapshot is a snapshot of **process memory and CPU state**, not a complete operating-system checkpoint.

The current implementation does not restore external resources such as:

* file descriptors;
* sockets;
* kernel-backed resources;
* signal state;
* threads other than the traced thread;
* other runtime state not represented by captured memory and registers.

Restored execution therefore has to avoid depending on unsupported external state.

The snapshot format is currently an internal implementation detail and is not intended as a stable interchange format.

## License

`process-bridge` is released under the [MIT License](LICENSE).

