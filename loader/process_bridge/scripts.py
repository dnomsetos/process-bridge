import subprocess
import sys
from importlib.resources import files

from .log import get_logger

log = get_logger(__name__)


def _run(binary_name: str) -> None:
    exe = files("process_bridge") / "bin" / binary_name

    if not exe.is_file():
        log.error(
            "native binary '%s' not found at %s — the package may not have "
            "been built/installed correctly",
            binary_name,
            exe,
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
