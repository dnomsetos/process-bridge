from __future__ import annotations

import pytest

from process_bridge.x86.user_regs_struct import SnapshotInfo, dump_regs, restore_tls


def test_dump_regs_copies_gprs_and_eflags():
    from fakes import FakeQiling

    snapshot = SnapshotInfo()
    for field, value in {
        "eax": 1,
        "ebx": 2,
        "ecx": 3,
        "edx": 4,
        "esi": 5,
        "edi": 6,
        "ebp": 7,
        "esp": 8,
        "eip": 0xCAFE,
        "eflags": 0x202,
    }.items():
        setattr(snapshot.regs, field, value)

    ql = FakeQiling()
    dump_regs(ql, snapshot)

    assert ql.arch.regs.eax == 1
    assert ql.arch.regs.ebx == 2
    assert ql.arch.regs.ecx == 3
    assert ql.arch.regs.edx == 4
    assert ql.arch.regs.esi == 5
    assert ql.arch.regs.edi == 6
    assert ql.arch.regs.ebp == 7
    assert ql.arch.regs.esp == 8
    assert ql.arch.regs.eip == 0xCAFE
    assert ql.arch.regs.eflags == 0x202


FLAG_SEG_32BIT = 1 << 0
FLAG_READ_EXEC_ONLY = 1 << 3
FLAG_LIMIT_IN_PAGES = 1 << 4

A_PRESENT = 0x01
A_PRIV_3 = 0x02
A_DATA = 0x04
A_DATA_E = 0x08
A_DESC_DATA = 0x10
A_DATA_W = 0x20


def test_restore_tls_registers_writable_segment_with_byte_limit():
    from fakes import FakeGdtm, FakeQiling

    snapshot = SnapshotInfo()
    snapshot.seg.base_addr = 0x7F000000
    snapshot.seg.limit = 0xFFF
    snapshot.seg.flags = FLAG_SEG_32BIT

    gdtm = FakeGdtm(free_idx=3)
    ql = FakeQiling(gdtm=gdtm)

    restore_tls(ql, snapshot)

    assert len(gdtm.registered) == 1
    index, base_addr, byte_limit, access = gdtm.registered[0]
    assert index == 3
    assert base_addr == 0x7F000000
    assert byte_limit == 0xFFF
    expected_access = A_PRESENT | A_PRIV_3 | A_DESC_DATA | A_DATA | A_DATA_E | A_DATA_W
    assert access == expected_access

    assert ql.arch.regs.gs == (3 << 3) | 0x3


def test_restore_tls_read_exec_only_segment_has_no_write_bit():
    from fakes import FakeGdtm, FakeQiling

    snapshot = SnapshotInfo()
    snapshot.seg.base_addr = 0x1000
    snapshot.seg.limit = 0x10
    snapshot.seg.flags = FLAG_READ_EXEC_ONLY

    ql = FakeQiling(gdtm=FakeGdtm(free_idx=5))

    restore_tls(ql, snapshot)

    _, _, _, access = ql.os.gdtm.registered[0]
    assert access & A_DATA_W == 0
    expected_access = A_PRESENT | A_PRIV_3 | A_DESC_DATA | A_DATA | A_DATA_E
    assert access == expected_access


def test_restore_tls_limit_in_pages_is_expanded_to_byte_limit():
    from fakes import FakeGdtm, FakeQiling

    snapshot = SnapshotInfo()
    snapshot.seg.base_addr = 0x2000
    snapshot.seg.limit = 2
    snapshot.seg.flags = FLAG_LIMIT_IN_PAGES

    ql = FakeQiling(gdtm=FakeGdtm(free_idx=0))

    restore_tls(ql, snapshot)

    _, _, byte_limit, _ = ql.os.gdtm.registered[0]
    assert byte_limit == (2 << 12) | 0xFFF


def test_restore_tls_raises_when_no_free_gdt_slot():
    from fakes import FakeGdtm, FakeQiling

    snapshot = SnapshotInfo()
    ql = FakeQiling(gdtm=FakeGdtm(free_idx=None))

    with pytest.raises(RuntimeError):
        restore_tls(ql, snapshot)
