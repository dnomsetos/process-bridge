from __future__ import annotations

import pytest
from qiling.const import QL_ARCH

from process_bridge import arch as arch_mod
from process_bridge.i386 import user_regs_struct as i386_impl
from process_bridge.x86_64 import user_regs_struct as x8664_impl


@pytest.fixture(autouse=True)
def _reset_arch_state():
    arch_mod.SnapshotInfo = None
    arch_mod.dump_regs = None
    arch_mod.finalize = None
    arch_mod.SNAPSHOT_ARCH = None
    yield
    arch_mod.SnapshotInfo = None
    arch_mod.dump_regs = None
    arch_mod.finalize = None
    arch_mod.SNAPSHOT_ARCH = None


def test_get_arch_impl_x8664_wires_up_module_globals():
    arch_mod.get_arch_impl(QL_ARCH.X8664)

    assert arch_mod.SNAPSHOT_ARCH == QL_ARCH.X8664
    assert arch_mod.SnapshotInfo is x8664_impl.SnapshotInfo
    assert arch_mod.dump_regs is x8664_impl.dump_regs


def test_get_arch_impl_x86_wires_up_module_globals():
    arch_mod.get_arch_impl(QL_ARCH.X86)

    assert arch_mod.SNAPSHOT_ARCH == QL_ARCH.X86
    assert arch_mod.SnapshotInfo is i386_impl.SnapshotInfo
    assert arch_mod.dump_regs is i386_impl.dump_regs


def test_get_arch_impl_rejects_unsupported_arch():
    with pytest.raises(ValueError, match="unsupported architecture"):
        arch_mod.get_arch_impl(QL_ARCH.ARM)
