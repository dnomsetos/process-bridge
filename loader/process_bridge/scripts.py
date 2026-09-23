import os
import subprocess
import sys
from pathlib import Path


def get_native_binary(arch: str) -> Path:
    binary = Path(__file__).parent / "bin" / arch / "process-bridge"

    if not binary.is_file():
        raise RuntimeError(f"Native process-bridge binary not found: {binary}")

    return binary


def _run(arch: str) -> None:
    binary = get_native_binary(arch)

    result = subprocess.run([str(binary), *sys.argv[1:]])

    sys.exit(result.returncode)


def x86_64_linux() -> None:
    _run("x86_64")


def i386_linux() -> None:
    _run("i386")
