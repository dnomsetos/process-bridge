# process-bridge

[![CI](https://github.com/dnomsetos/process-bridge/actions/workflows/ci.yml/badge.svg)](https://github.com/dnomsetos/process-bridge/actions/workflows/ci.yml) [![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

**process-bridge** snapshots the execution state of a native Linux process and restores that state in an emulator.

The project is split into two parts:

- **snapshotter** — a native C program that launches and traces a target process, stops it at a breakpoint, captures its CPU state and memory mappings, and writes a snapshot file;
- **loader** — a native C library that restores a snapshot into a [Unicorn](https://www.unicorn-engine.org/) CPU emulator.

A small Python package exposes the loader through <code>ctypes</code> and provides a helper for creating a [Qiling](https://qiling.io/) instance from a snapshot.

The main use case is to reach a difficult execution state natively, capture it once, and then reproduce that state in an emulator for debugging, analysis, instrumentation, or fuzzing.

## How it works

The basic workflow is:

~~~text
               native execution
                     │
                     ▼
              target process
                     │
            breakpoint reached
                     │
                     ▼
                snapshot
              ┌──────┴──────┐
              │ CPU state   │
              │ memory      │
              │ mappings    │
              └──────┬──────┘
                     │
                     ▼
              Unicorn emulator
                     │
                     ▼
             resumed execution
~~~

The snapshot contains:

- integer CPU registers;
- process memory mappings;
- the contents of each captured mapping;
- mapping permissions, sharing information, file offset, and path.

For <code>i386</code>, the snapshot also contains the TLS descriptors needed to restore the captured CPU state.

Some special Linux mappings cannot be copied meaningfully and are skipped, including <code>[vvar]</code>, <code>[vvar_vclock]</code>, and <code>[vsyscall]</code>. If a readable mapping cannot be copied completely, the affected chunks are replaced with zeroes.

## Supported architectures

Currently supported:

- <code>x86_64</code>
- <code>i386</code>

The native snapshotter must be built for the architecture of the target process.

## Requirements

### Runtime

- Linux
- Python >= 3.10
- Clang
- CMake >= 3.20
- Ninja

Python dependencies are declared in <code>pyproject.toml</code>:

- <code>unicorn==2.1.4</code>
- <code>qiling==1.4.6</code>

Qiling is only needed when using the Python Qiling helper. The native loader itself works directly with Unicorn.

### Building i386 binaries

To build and run the 32-bit snapshotter on a Debian/Ubuntu system, the host also needs the 32-bit development libraries/toolchain, for example:

~~~bash
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install libc6-dev:i386 gcc-multilib g++-multilib
~~~

The repository provides a matching Ubuntu 24.04 Docker environment in [<code>Dockerfile</code>](Dockerfile).

## Installation

The Python package builds the native loader automatically through <code>scikit-build-core</code>.

Create a virtual environment and install the project:

~~~bash
python -m venv .venv
source .venv/bin/activate

python -m pip install --upgrade pip
python -m pip install .
~~~

The installed package exposes:

~~~python
import process_bridge

process_bridge.restore_snapshot(...)
process_bridge.make_qiling_from_snapshot(...)
~~~

The snapshotter is a separate native executable and is built with CMake.

## Building from source

The project uses CMake presets for the native components.

### Snapshotter

Build the <code>x86_64</code> release version:

~~~bash
cmake --preset snapshotter-x86_64-release
cmake --build --preset snapshotter-x86_64-release
~~~

The executable is produced at:

~~~text
build/snapshotter-x86_64-release/snapshotter/pb-snapshotter
~~~

For <code>i386</code>:

~~~bash
cmake --preset snapshotter-i386-release
cmake --build --preset snapshotter-i386-release
~~~

The debug presets enable AddressSanitizer and UndefinedBehaviorSanitizer:

~~~bash
cmake --preset snapshotter-x86_64-debug
cmake --build --preset snapshotter-x86_64-debug
~~~

### Loader

The loader can be built directly with CMake:

~~~bash
cmake --preset loader-release
cmake --build --preset loader-release
~~~

The resulting library is:

~~~text
build/loader-release/loader/libpb-loader.so
~~~

The Python package build performs the loader build automatically and installs the loader and its Unicorn dependency into the package's <code>_native</code> directory.

## Taking a snapshot

The snapshotter executable has the following interface:

~~~text
pb-snapshotter <breakpoint_offset_hex> <child_path> [child_args...]
~~~

For example:

~~~bash
./build/snapshotter-x86_64-release/snapshotter/pb-snapshotter 0x114d ./hello
~~~

The snapshotter:

1. forks and starts the target under <code>ptrace</code>;
2. determines the target image base;
3. places an <code>INT3</code> breakpoint at <code>image_base + breakpoint_offset_hex</code>;
4. resumes the target until the breakpoint is hit;
5. captures the CPU registers;
6. reads the target's <code>/proc/&lt;pid&gt;/maps</code> layout and memory;
7. writes the result to <code>ql_snapshot</code>;
8. terminates the target process.

The snapshot file is written relative to the current working directory.

### Choosing the breakpoint

The breakpoint argument is an **offset from the loaded image base**, not necessarily an absolute runtime address.

For PIE executables, use the virtual address of the instruction in the ELF image. Tools such as <code>objdump</code> or <code>nm</code> can be used to locate the instruction or symbol:

~~~bash
objdump -d ./hello
nm ./hello
~~~

For a useful snapshot point, place the breakpoint at the instruction where you want emulation to resume. For example, placing it on a <code>call</code> instruction captures the state immediately before the call is executed, including the arguments already prepared by the caller.

See [<code>examples/hello</code>](examples/hello) and [<code>examples/nginx</code>](examples/nginx) for complete scenarios.

## Restoring a snapshot with Unicorn

The lowest-level Python API restores a snapshot into an existing <code>unicorn.Uc</code> instance:

~~~python
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
~~~

<code>restore_snapshot()</code> returns the restored instruction pointer. The emulator's memory mappings and CPU registers are populated from the snapshot before execution resumes.

The C API exposed by <code>libpb-loader.so</code> provides the same core operation:

~~~c
uint64_t pb_restore_snapshot(
    uc_engine *uc,
    const char *snapshot_path
);
~~~

A convenience function is also available when the caller wants the loader to create the Unicorn engine itself:

~~~c
uint64_t pb_create_from_snapshot(
    uc_arch arch,
    uc_mode mode,
    uc_engine **engine,
    const char *snapshot_path
);
~~~

Currently supported Unicorn configurations are:

- <code>UC_ARCH_X86 + UC_MODE_32</code>
- <code>UC_ARCH_X86 + UC_MODE_64</code>

## Restoring a snapshot with Qiling

The Python package provides a Qiling-specific helper:

~~~python
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
~~~

Qiling normally creates its own memory mappings during initialization. The helper removes those mappings and then restores the mappings saved in the process snapshot.

Qiling still requires a <code>rootfs</code> when the emulator is created. For the current snapshot workflow, an empty directory is sufficient because the process state is restored afterwards:

~~~bash
mkdir -p dummy_rootfs
~~~

The target program does not need to be executed from the Qiling rootfs to restore the captured CPU and memory state.

## Logging

Both native components use the <code>PROCESS_BRIDGE_LOG_LEVEL</code> environment variable.

Supported levels:

- <code>DEBUG</code>
- <code>INFO</code>
- <code>WARN</code>
- <code>ERROR</code>

The default is <code>INFO</code>.

Example:

~~~bash
PROCESS_BRIDGE_LOG_LEVEL=DEBUG ./build/snapshotter-x86_64-release/snapshotter/pb-snapshotter 0x114d ./hello
~~~

Debug logging is useful when diagnosing breakpoint placement, image-base calculation, register state, and restored memory mappings.

## Project layout

~~~text
.
├── common/                  Shared Linux data structures
├── snapshotter/             Native process snapshotter
│   ├── include/
│   ├── src/
│   └── tests/
├── loader/                  Unicorn-based snapshot loader
│   ├── include/
│   ├── src/
│   └── tests/
├── python/process_bridge/   Python bindings and Qiling helper
├── examples/                Example workflows
├── utilities/               Shared native utilities
├── CMakeLists.txt
├── CMakePresets.json
├── Dockerfile
└── pyproject.toml
~~~

## Testing

The project has separate CTest suites for the snapshotter and loader.

Run the snapshotter tests:

~~~bash
cmake --preset snapshotter-x86_64-debug
cmake --build --preset snapshotter-x86_64-debug
ctest --preset snapshotter-x86_64-debug --output-on-failure
~~~

Run the loader tests:

~~~bash
cmake --preset loader-debug
cmake --build --preset loader-debug
ctest --preset loader-debug --output-on-failure
~~~

The debug presets build with AddressSanitizer and UndefinedBehaviorSanitizer.

## Docker

The repository includes an Ubuntu 24.04 development image with Clang, CMake, Ninja, Git, and 32-bit development support.

Build it with:

~~~bash
docker build -t process-bridge .
~~~

Run the project directory mounted into the container:

~~~bash
docker run --rm -it -v "$PWD:/workspace" process-bridge
~~~

Inside the container, both the native CMake workflow and the Python package installation can be used.

## Limitations

The snapshot is a snapshot of **process memory and CPU state**, not a complete operating-system process checkpoint.

The current implementation does not restore external process resources such as:

- file descriptors;
- sockets;
- kernel-backed resources;
- signal state;
- threads other than the traced thread;
- other process-wide runtime state not represented by the captured memory and registers.

This means a restored process can only continue successfully when the execution path does not depend on unsupported external state.

The project is still under active development, and the snapshot format is currently an internal implementation detail rather than a stable interchange format.

## License

<code>process-bridge</code> is released under the [MIT License](LICENSE).
