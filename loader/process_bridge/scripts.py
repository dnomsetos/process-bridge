import os
import subprocess
import sys
from importlib.resources import files
from pathlib import Path
from typing import Optional

from .log import get_logger

log = get_logger(__name__)

# `pip install -e .` (scikit-build-core's "redirect" editable mode) only
# redirects Python source files back to the repo; it does not run CMake's
# `install()` step into the package tree, so the compiled binaries never end
# up under the installed `process_bridge/bin/`. As a fallback for editable
# installs, look for the plain `build/` directory produced by building this
# repo directly with CMake (see the top-level README):
# loader/process_bridge/scripts.py -> ../../build/<binary_name>
_DEV_BUILD_DIR = Path(__file__).resolve().parents[2] / "build"


def _find_binary(binary_name: str) -> Optional[Path]:
    # 1. Regular installs (`pip install .` / a built wheel): binary is
    #    bundled as package data.
    packaged = files("process_bridge") / "bin" / binary_name
    if packaged.is_file():
        return Path(str(packaged))

    # 2. Explicit override, e.g. a custom CMake build directory.
    override_dir = os.environ.get("PROCESS_BRIDGE_BIN_DIR")
    if override_dir:
        candidate = Path(override_dir) / binary_name
        if candidate.is_file():
            return candidate

    # 3. Editable installs: fall back to the repo's own `build/` directory.
    candidate = _DEV_BUILD_DIR / binary_name
    if candidate.is_file():
        return candidate

    return None


def _run(binary_name: str) -> None:
    exe = _find_binary(binary_name)

    if exe is None:
        log.error(
            "native binary '%s' not found. Looked in the installed package, "
            "$PROCESS_BRIDGE_BIN_DIR, and '%s'. If you installed with "
            "`pip install -e .`, build the binaries yourself first "
            "(`cmake -B build && cmake --build build`, see the README) or "
            "point $PROCESS_BRIDGE_BIN_DIR at a directory containing them.",
            binary_name,
            _DEV_BUILD_DIR,
        )
        sys.exit(1)

    try:
        result = subprocess.run([str(exe), *sys.argv[1:]])
    except OSError:
        log.exception("failed to launch '%s'", exe)
        sys.exit(1)

    if result.returncode != 0:
        log.warning("%s exited with status %d", binary_name, result.returncode)

    sys.exit(result.returncode)


def run_x64() -> None:
    _run("process-bridge-x64")


def run_x86() -> None:
    _run("process-bridge-x86")
