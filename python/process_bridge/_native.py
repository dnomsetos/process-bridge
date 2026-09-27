import ctypes
import os
from pathlib import Path
from importlib.resources import files

_NATIVE_DIR = files("process_bridge") / "_native"

os.environ.setdefault("LIBUNICORN_PATH", str(_NATIVE_DIR))

import unicorn

_lib = ctypes.CDLL(str(_NATIVE_DIR / "libpb-loader.so"))
_lib.pb_restore_snapshot.argtypes = (ctypes.c_void_p, ctypes.c_char_p)
_lib.pb_restore_snapshot.restype = ctypes.c_uint64

PB_FAIL = (1 << 64) - 1


def restore_snapshot(uc: "unicorn.Uc", snapshot_path: str) -> int:
    entry = _lib.pb_restore_snapshot(uc._uch, str(snapshot_path).encode())
    if entry == PB_FAIL:
        raise RuntimeError(f"pb_restore_snapshot failed for {snapshot_path!r}")
    return entry
