from __future__ import annotations

from ctypes import Structure, c_uint32, c_uint64, sizeof

import pytest

from process_bridge import arch as arch_mod
from process_bridge import maps_entry as maps_entry_mod
from process_bridge import read_snapshot
from process_bridge.read_snapshot import load_snapshot


class _Point(Structure):
    _fields_ = [("x", c_uint32), ("y", c_uint32)]


def test_read_struct_success_at_offset_zero():
    buf = bytearray(sizeof(_Point))
    _Point.from_buffer(buf).x = 7
    _Point.from_buffer(buf).y = 9

    obj, new_offset = read_snapshot.read_struct(_Point, memoryview(buf), 0)

    assert (obj.x, obj.y) == (7, 9)
    assert new_offset == sizeof(_Point)


def test_read_struct_success_at_nonzero_offset():
    padding = b"\x00" * 3
    point = _Point(x=1, y=2)
    buf = bytearray(padding) + bytearray(point)

    obj, new_offset = read_snapshot.read_struct(_Point, memoryview(buf), 3)

    assert (obj.x, obj.y) == (1, 2)
    assert new_offset == 3 + sizeof(_Point)


def test_read_struct_raises_eof_when_buffer_too_short():
    buf = bytearray(sizeof(_Point) - 1)

    with pytest.raises(EOFError):
        read_snapshot.read_struct(_Point, memoryview(buf), 0)


def test_read_struct_raises_eof_when_offset_beyond_buffer():
    buf = bytearray(sizeof(_Point))

    with pytest.raises(EOFError):
        read_snapshot.read_struct(_Point, memoryview(buf), sizeof(_Point) + 1)


class _FakeSnapshotHeader(Structure):
    _fields_ = [("marker", c_uint32)]


class _FakeMapsEntry(Structure):
    _fields_ = [
        ("start", c_uint64),
        ("end", c_uint64),
        ("read", c_uint32),
        ("write", c_uint32),
        ("exec", c_uint32),
        ("shared", c_uint32),
        ("file_offset", c_uint64),
        ("path", c_uint32),
    ]


def _build_snapshot_file(tmp_path, mappings: list[tuple[int, bytes]]):
    path = tmp_path / "snapshot.bin"
    with open(path, "wb") as f:
        f.write(bytes(_FakeSnapshotHeader(marker=0xAABBCCDD)))
        for start, content in mappings:
            end = start + len(content)
            f.write(bytes(_FakeMapsEntry(start=start, end=end)))
            f.write(content)
    return path


def test_load_snapshot_dispatches_regs_and_all_mappings(tmp_path, monkeypatch):
    monkeypatch.setattr(arch_mod, "SnapshotInfo", _FakeSnapshotHeader)
    monkeypatch.setattr(maps_entry_mod, "get_maps_entry_cls", lambda: _FakeMapsEntry)

    dump_regs_calls = []
    finalize_calls = []
    monkeypatch.setattr(
        arch_mod, "dump_regs", lambda ql, snap: dump_regs_calls.append(snap.marker)
    )
    monkeypatch.setattr(
        arch_mod, "finalize", lambda ql, snap: finalize_calls.append(snap.marker)
    )

    dump_mapping_calls = []
    monkeypatch.setattr(
        read_snapshot,
        "dump_mapping",
        lambda ql, entry, content: dump_mapping_calls.append(
            (entry.start, entry.end, content)
        ),
    )

    content_a = bytes(range(16))
    content_b = b"\xde\xad\xbe\xef"
    path = _build_snapshot_file(tmp_path, [(0x1000, content_a), (0x2000, content_b)])

    load_snapshot(ql=object(), snapshot_path=str(path))

    assert dump_regs_calls == [0xAABBCCDD]
    assert finalize_calls == [0xAABBCCDD]
    assert dump_mapping_calls == [
        (0x1000, 0x1000 + len(content_a), content_a),
        (0x2000, 0x2000 + len(content_b), content_b),
    ]


def test_load_snapshot_raises_on_truncated_mapping_content(tmp_path, monkeypatch):
    monkeypatch.setattr(arch_mod, "SnapshotInfo", _FakeSnapshotHeader)
    monkeypatch.setattr(maps_entry_mod, "get_maps_entry_cls", lambda: _FakeMapsEntry)
    monkeypatch.setattr(arch_mod, "dump_regs", lambda ql, snap: None)
    monkeypatch.setattr(arch_mod, "finalize", lambda ql, snap: None)

    path = tmp_path / "truncated.bin"
    with open(path, "wb") as f:
        f.write(bytes(_FakeSnapshotHeader(marker=1)))
        f.write(bytes(_FakeMapsEntry(start=0x1000, end=0x1010)))
        f.write(b"\x01\x02\x03\x04")

    with pytest.raises(EOFError):
        load_snapshot(ql=object(), snapshot_path=str(path))


def test_load_snapshot_wraps_missing_file_as_oserror(tmp_path):
    missing = tmp_path / "does_not_exist.bin"

    with pytest.raises(OSError):
        load_snapshot(ql=object(), snapshot_path=str(missing))
