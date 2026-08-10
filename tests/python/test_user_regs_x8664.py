from __future__ import annotations

from process_bridge.x86_64.user_regs_struct import (
    SnapshotInfo,
    UserRegsStruct,
    dump_regs,
)


def test_dump_regs_copies_all_gprs_and_special_regs():
    from fakes import FakeQiling

    snapshot = SnapshotInfo()
    regs = snapshot.regs
    values = {
        "rax": 0x1111,
        "rbx": 0x2222,
        "rcx": 0x3333,
        "rdx": 0x4444,
        "rsi": 0x5555,
        "rdi": 0x6666,
        "rbp": 0x7777,
        "rsp": 0x8888,
        "r8": 0x9,
        "r9": 0xA,
        "r10": 0xB,
        "r11": 0xC,
        "r12": 0xD,
        "r13": 0xE,
        "r14": 0xF,
        "r15": 0x10,
        "rip": 0xDEADBEEF,
        "elflags": 0x246,
        "fs_base": 0x7F0000000000,
        "gs_base": 0x7F0000001000,
    }
    for field, value in values.items():
        setattr(regs, field, value)

    ql = FakeQiling()
    dump_regs(ql, snapshot)

    assert ql.arch.regs.rax == 0x1111
    assert ql.arch.regs.rbx == 0x2222
    assert ql.arch.regs.rcx == 0x3333
    assert ql.arch.regs.rdx == 0x4444
    assert ql.arch.regs.rsi == 0x5555
    assert ql.arch.regs.rdi == 0x6666
    assert ql.arch.regs.rbp == 0x7777
    assert ql.arch.regs.rsp == 0x8888
    assert ql.arch.regs.r8 == 0x9
    assert ql.arch.regs.r9 == 0xA
    assert ql.arch.regs.r10 == 0xB
    assert ql.arch.regs.r11 == 0xC
    assert ql.arch.regs.r12 == 0xD
    assert ql.arch.regs.r13 == 0xE
    assert ql.arch.regs.r14 == 0xF
    assert ql.arch.regs.r15 == 0x10
    assert ql.arch.regs.rip == 0xDEADBEEF
    assert ql.arch.regs.eflags == 0x246
    assert ql.arch.regs.fsbase == 0x7F0000000000
    assert ql.arch.regs.gsbase == 0x7F0000001000


def test_user_regs_struct_defaults_are_zero():
    regs = UserRegsStruct()
    assert regs.rax == 0
    assert regs.rip == 0
    assert regs.fs_base == 0
