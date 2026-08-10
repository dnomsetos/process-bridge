from __future__ import annotations

import enum
import sys
import types
from pathlib import Path

_LOADER_DIR = Path(__file__).resolve().parents[2] / "loader"
if str(_LOADER_DIR) not in sys.path:
    sys.path.insert(0, str(_LOADER_DIR))


def _install_fake_unicorn() -> None:
    unicorn_mod = types.ModuleType("unicorn")
    unicorn_mod.UC_PROT_READ = 1
    unicorn_mod.UC_PROT_WRITE = 2
    unicorn_mod.UC_PROT_EXEC = 4

    class UcError(Exception):
        def __init__(self, errno: int = 0) -> None:
            self.errno = errno
            super().__init__(f"UcError({errno})")

    unicorn_mod.UcError = UcError
    sys.modules["unicorn"] = unicorn_mod

    x86_const_mod = types.ModuleType("unicorn.x86_const")
    sys.modules["unicorn.x86_const"] = x86_const_mod
    unicorn_mod.x86_const = x86_const_mod


def _install_fake_qiling() -> None:
    qiling_mod = types.ModuleType("qiling")

    class Qiling:
        pass

    qiling_mod.Qiling = Qiling
    sys.modules["qiling"] = qiling_mod

    const_mod = types.ModuleType("qiling.const")

    class QL_ARCH(enum.Enum):
        X8664 = "x8664"
        X86 = "x86"
        ARM = "arm"

    class QL_OS(enum.Enum):
        LINUX = "linux"

    class QL_VERBOSE(enum.Enum):
        DEFAULT = "default"
        DEBUG = "debug"

    const_mod.QL_ARCH = QL_ARCH
    const_mod.QL_OS = QL_OS
    const_mod.QL_VERBOSE = QL_VERBOSE
    sys.modules["qiling.const"] = const_mod
    qiling_mod.const = const_mod

    arch_mod = types.ModuleType("qiling.arch")
    sys.modules["qiling.arch"] = arch_mod
    qiling_mod.arch = arch_mod

    x86_const_mod = types.ModuleType("qiling.arch.x86_const")
    x86_const_mod.QL_X86_A_PRESENT = 0x01
    x86_const_mod.QL_X86_A_PRIV_3 = 0x02
    x86_const_mod.QL_X86_A_DATA = 0x04
    x86_const_mod.QL_X86_A_DATA_E = 0x08
    x86_const_mod.QL_X86_A_DESC_DATA = 0x10
    x86_const_mod.QL_X86_A_DATA_W = 0x20
    sys.modules["qiling.arch.x86_const"] = x86_const_mod
    arch_mod.x86_const = x86_const_mod


def _purge_real_process_bridge() -> None:
    for name in list(sys.modules):
        if name == "process_bridge" or name.startswith("process_bridge."):
            del sys.modules[name]


_install_fake_unicorn()
_install_fake_qiling()
_purge_real_process_bridge()
