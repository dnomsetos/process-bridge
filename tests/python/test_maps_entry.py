from __future__ import annotations

from types import SimpleNamespace

import pytest
from qiling.const import QL_ARCH

from process_bridge import arch as arch_mod
from process_bridge import maps_entry


@pytest.fixture(autouse=True)
def _reset_arch_state():
    arch_mod.SNAPSHOT_ARCH = None
    maps_entry._CLASS_CACHE.clear()
    yield
    arch_mod.SNAPSHOT_ARCH = None
    maps_entry._CLASS_CACHE.clear()


def test_get_maps_entry_cls_without_selected_arch_raises():
    with pytest.raises(RuntimeError):
        maps_entry.get_maps_entry_cls()


def test_get_maps_entry_cls_uses_pack_8_for_x8664():
    arch_mod.SNAPSHOT_ARCH = QL_ARCH.X8664
    cls = maps_entry.get_maps_entry_cls()
    assert cls._pack_ == 8


def test_get_maps_entry_cls_uses_pack_4_for_x86():
    arch_mod.SNAPSHOT_ARCH = QL_ARCH.X86
    cls = maps_entry.get_maps_entry_cls()
    assert cls._pack_ == 4


def test_get_maps_entry_cls_is_cached_per_pack():
    arch_mod.SNAPSHOT_ARCH = QL_ARCH.X8664
    cls_a = maps_entry.get_maps_entry_cls()
    cls_b = maps_entry.get_maps_entry_cls()
    assert cls_a is cls_b


def test_get_maps_entry_cls_differs_between_arches():
    arch_mod.SNAPSHOT_ARCH = QL_ARCH.X8664
    cls_64 = maps_entry.get_maps_entry_cls()

    arch_mod.SNAPSHOT_ARCH = QL_ARCH.X86
    cls_32 = maps_entry.get_maps_entry_cls()

    assert cls_64 is not cls_32
    assert cls_64._pack_ != cls_32._pack_


@pytest.mark.parametrize(
    ("read", "write", "exec_", "expected"),
    [
        (False, False, False, 0),
        (True, False, False, 1),
        (False, True, False, 2),
        (False, False, True, 4),
        (True, True, False, 3),
        (True, False, True, 5),
        (False, True, True, 6),
        (True, True, True, 7),
    ],
)
def test_to_uc_perms_combines_flags(read, write, exec_, expected):
    entry = SimpleNamespace(read=read, write=write, exec=exec_)
    assert maps_entry.to_uc_perms(entry) == expected


def test_dump_mapping_maps_then_writes_with_correct_perms():
    from fakes import FakeQiling

    ql = FakeQiling()
    entry = SimpleNamespace(
        start=0x1000,
        end=0x3000,
        read=True,
        write=True,
        exec=False,
        shared=False,
        path=b"/x",
    )
    content = b"\x90" * (entry.end - entry.start)

    maps_entry.dump_mapping(ql, entry, content)

    assert ql.mem.map_calls == [(0x1000, 0x2000, 3)]
    assert ql.mem.write_calls == [(0x1000, content)]


def test_dump_mapping_propagates_map_failure():
    from fakes import FakeQiling

    class ExplodingMem:
        def map(self, *_args, **_kwargs):
            raise RuntimeError("boom")

    ql = FakeQiling()
    ql.mem = ExplodingMem()
    entry = SimpleNamespace(
        start=0x1000,
        end=0x2000,
        read=True,
        write=False,
        exec=False,
        shared=False,
        path=b"",
    )

    with pytest.raises(RuntimeError, match="boom"):
        maps_entry.dump_mapping(ql, entry, b"\x00" * 0x1000)
