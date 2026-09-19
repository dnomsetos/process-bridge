# process-bridge

[![CI](https://github.com/dnomsetos/process-bridge/actions/workflows/ci.yml/badge.svg)](https://github.com/dnomsetos/process-bridge/actions/workflows/ci.yml) [![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

`process-bridge` is a tool for creating a snapshot of a native Linux process at a breakpoint and restoring that state inside the [Qiling](https://qiling.io/) emulator to continue execution from the same point.

The snapshot contains, among other things, the process register state and memory mappings.

The project consists of two main parts:

* `native/` — the native C implementation that creates a snapshot of the target process;
* `loader/process_bridge/` — the Python part that loads the snapshot and restores the process in the Qiling environment.

## Supported Architectures

The following architectures are currently supported:

* `x86_64`
* `i386`

## Requirements

`process-bridge` works only on **Linux**.

The Python package requires:

* Python ≥ 3.12
* `pip`

The native components are automatically built with CMake and Clang during package installation.

## Installation

Create a virtual environment and install the package:

```bash
python -m venv .venv
source .venv/bin/activate

pip install --upgrade pip
pip install .
```

During installation, `scikit-build-core` invokes CMake and builds the native components for both supported architectures.

After installation, two commands will be available:

```bash
process-bridge-x86_64-linux
process-bridge-i386-linux
```

Each command runs the native component for the corresponding architecture.

## Logging

The logging level can be configured using the `PROCESS_BRIDGE_LOG_LEVEL` environment variable.

The following levels are supported:

* `DEBUG`
* `INFO`
* `WARN`
* `WARNING`
* `ERROR`

For example:

```bash
PROCESS_BRIDGE_LOG_LEVEL=DEBUG process-bridge-x86_64-linux ...
```

or:

```bash
PROCESS_BRIDGE_LOG_LEVEL=ERROR process-bridge-i386-linux ...
```

This variable is used by both the Python and native parts of `process-bridge`, so the same setting controls the logging level of the entire tool.

The default logging level is `INFO`.

## Usage

### x86_64

After installation, the package provides:

```bash
process-bridge-x86_64-linux \
    <breakpoint_offset_hex> \
    <path-to-target-binary> \
    [target-args...]
```

### i386

For a 32-bit process, use:

```bash
process-bridge-i386-linux \
    <breakpoint_offset_hex> \
    <path-to-target-binary> \
    [target-args...]
```

Arguments:

* `breakpoint_offset_hex` — the breakpoint offset relative to the loaded image base, specified in hexadecimal;
* `path-to-target-binary` — path to the executable to trace;
* `target-args...` — optional arguments passed to the target process.

The tool launches the target process, sets a breakpoint at:

```text
image_base + breakpoint_offset_hex
```

and creates a snapshot when the breakpoint is reached.

The snapshot is written to:

```text
ql_snapshot
```

relative to the current working directory.

### PIE binaries

For a PIE binary, `breakpoint_offset_hex` corresponds to the virtual address of the symbol in the binary.

The address can be obtained, for example, with:

```bash
nm <binary>
```

or:

```bash
objdump -d <binary>
```

See [`examples/hello`](examples/hello) for a complete example.

## Loading a Snapshot from Python

Once a snapshot has been created, it can be loaded from Python and execution can be resumed in Qiling:

```python
from process_bridge import snaphot_init

ql, entry = snaphot_init.from_snapshot(
    "x86_64",
    "dummy_rootfs",
    "ql_snapshot",
    QL_VERBOSE.DEBUG,
)

ql.emu_start(
    begin=entry,
    end=...,
)
```

`rootfs_path` must point to an existing directory. The directory may be empty.

Qiling requires a `rootfs` during initialization, but its contents do not matter in this scenario: the default memory mappings created by Qiling are removed and replaced with the mappings restored from the snapshot before execution resumes.

See the complete examples:

* [`examples/hello`](examples/hello)
* [`examples/nginx`](examples/nginx)

## Building from Source

For development, the native components can be built directly with CMake.

The target architecture is selected using a CMake preset.

### x86_64

```bash
cmake --preset x86_64-linux
cmake --build --preset x86_64-linux
```

The build output will be located in:

```text
build/x86_64-linux/
```

The main artifacts are:

```text
process-bridge
libprocess-bridge-lib.so
```

### i386

```bash
cmake --preset i386-linux
cmake --build --preset i386-linux
```

The build output will be located in:

```text
build/i386-linux/
```

### Native Tests

For x86_64:

```bash
ctest --preset x86_64-linux --output-on-failure
```

For i386:

```bash
ctest --preset i386-linux --output-on-failure
```

## Docker

A `Dockerfile` based on Ubuntu 24.04 is provided for a reproducible development environment.

Build the image:

```bash
docker build -t process-bridge .
```

Run the container with the project sources mounted:

```bash
docker run --rm -it \
  -v "$PWD:/workspace" \
  process-bridge
```

Once inside the container, the package can be installed normally:

```bash
pip install .
```

Alternatively, the native components can be built directly with CMake.

## License

This project is licensed under the MIT License.

See [`LICENSE`](LICENSE).

### Third-party Dependencies

The project uses the [Qiling Framework](https://qiling.io/) as its emulation environment.

Qiling is an external dependency of the project and is distributed under its own license. This does not affect the MIT license applied to the `process-bridge` source code.

## Instruction Compatibility

The restored process may contain CPU instructions that are not supported by
the Unicorn version used by Qiling. For example, applications using AVX2,
AVX-512, or instructions such as `XSAVEC` may fail during emulation.

In such cases, it may be necessary to disable the corresponding CPU features
when starting the target process. For example:

```bash
GLIBC_TUNABLES=glibc.cpu.hwcaps=-XSAVEC,-AVX,-AVX2,-AVX512 \
process-bridge-x86_64-linux <breakpoint_offset_hex> <path-to-target-binary> [target-args...]
```

This makes glibc avoid using the specified CPU features when selecting its
optimized implementations, which can prevent unsupported instructions from
being executed by the restored process.

## Project Status

`process-bridge` is a new project and is currently in an early stage of
development. Snapshot support is intentionally limited at this point.

The current implementation only captures and restores:

* process memory mappings;
* integer CPU registers.

Many other parts of the process state are not supported yet, including file
descriptors, sockets, and other OS resources.

As a result, `process-bridge` currently works best with applications and
execution points that do not depend heavily on unsupported process resources.
Support for additional types of process state may be added in the future.

