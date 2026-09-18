from __future__ import annotations

import struct
from ctypes import sizeof

import pytest

import process_bridge.i386.user_regs_struct as user_regs
from fakes import FakeGdtm, FakeQiling


TRAMPOLINE_ADDR = 0x100000
GDT_BASE = 0xF000
CODE_ADDR = 0x400000
STACK_ADDR = 0x800000

REG_EAX = 1
REG_EBX = 2
REG_ECX = 3
REG_EDX = 4
REG_ESI = 5
REG_EDI = 6
REG_EBP = 7
REG_DS = 8
REG_ES = 9
REG_FS = 10
REG_GS = 11
REG_CS = 12
REG_SS = 13


class FakeUc:
    def __init__(self) -> None:
        self.registers: dict[int, int] = {}
        self.emu_start_calls: list[tuple[int, int, int]] = []
        self.emu_start_hook = None

    def reg_write(self, reg: int, value: int) -> None:
        self.registers[reg] = value

    def reg_read(self, reg: int) -> int:
        return self.registers.get(reg, 0)

    def emu_start(
        self,
        begin: int,
        until: int,
        *,
        count: int,
    ) -> None:
        self.emu_start_calls.append((begin, until, count))

        if self.emu_start_hook is not None:
            self.emu_start_hook(begin, until, count)


@pytest.fixture
def ql(monkeypatch: pytest.MonkeyPatch) -> FakeQiling:
    """
    Создаёт FakeQiling с минимальным UC-интерфейсом,
    который используется user_regs_struct.py.
    """
    for name, value in {
        "UC_X86_REG_EAX": REG_EAX,
        "UC_X86_REG_EBX": REG_EBX,
        "UC_X86_REG_ECX": REG_ECX,
        "UC_X86_REG_EDX": REG_EDX,
        "UC_X86_REG_ESI": REG_ESI,
        "UC_X86_REG_EDI": REG_EDI,
        "UC_X86_REG_EBP": REG_EBP,
        "UC_X86_REG_DS": REG_DS,
        "UC_X86_REG_ES": REG_ES,
        "UC_X86_REG_FS": REG_FS,
        "UC_X86_REG_GS": REG_GS,
        "UC_X86_REG_CS": REG_CS,
        "UC_X86_REG_SS": REG_SS,
    }.items():
        monkeypatch.setattr(user_regs, name, value, raising=False)

    result = FakeQiling(gdtm=FakeGdtm(free_idx=12))
    result.uc = FakeUc()

    result.arch.regs.eax = 0
    result.arch.regs.ebx = 0
    result.arch.regs.ecx = 0
    result.arch.regs.edx = 0
    result.arch.regs.esi = 0
    result.arch.regs.edi = 0
    result.arch.regs.ebp = 0
    result.arch.regs.esp = 0
    result.arch.regs.eip = 0
    result.arch.regs.eflags = 0

    next_addr = TRAMPOLINE_ADDR

    def map_anywhere(
        size: int,
        perms: int = 0,
        minaddr: int = 0,
        info: str = "",
    ) -> int:
        del minaddr, info

        result.mem.map(next_addr, size, perms)
        return next_addr

    result.mem.map_anywhere = map_anywhere

    result.mem.map(GDT_BASE, 16 * 8)

    return result


def make_snapshot() -> user_regs.SnapshotInfo:
    snapshot = user_regs.SnapshotInfo()

    snapshot.regs.ebx = 0x11111111
    snapshot.regs.ecx = 0x22222222
    snapshot.regs.edx = 0x33333333
    snapshot.regs.esi = 0x44444444
    snapshot.regs.edi = 0x55555555
    snapshot.regs.ebp = 0x66666666
    snapshot.regs.eax = 0x77777777
    snapshot.regs.orig_eax = 0x88888888

    snapshot.regs.eip = CODE_ADDR
    snapshot.regs.esp = STACK_ADDR
    snapshot.regs.eflags = 0x202

    snapshot.regs.xcs = 5 << 3 | 3
    snapshot.regs.xss = 2 << 3 | 3
    snapshot.regs.xds = 2 << 3 | 3
    snapshot.regs.xes = 2 << 3 | 3
    snapshot.regs.xfs = 6 << 3 | 3
    snapshot.regs.xgs = 7 << 3 | 3

    return snapshot


def test_user_regs_struct_matches_i386_linux_layout():
    assert sizeof(user_regs.UserRegsStruct) == 17 * 4

    fields = [
        "ebx",
        "ecx",
        "edx",
        "esi",
        "edi",
        "ebp",
        "eax",
        "xds",
        "xes",
        "xfs",
        "xgs",
        "orig_eax",
        "eip",
        "xcs",
        "eflags",
        "esp",
        "xss",
    ]

    offsets = {
        field_name: getattr(
            user_regs.UserRegsStruct,
            field_name,
        ).offset
        for field_name, _ in user_regs.UserRegsStruct._fields_
    }

    assert [offsets[name] for name in fields] == [
        index * 4 for index in range(17)
    ]


def test_user_desc_bitfields_round_trip():
    desc = user_regs.UserDesc()

    desc.entry_number = 7
    desc.base_addr = 0x12345678
    desc.limit = 0xABCDEF
    desc.seg_32bit = 1
    desc.contents = 3
    desc.read_exec_only = 1
    desc.limit_in_pages = 1
    desc.seg_not_present = 1
    desc.useable = 1

    assert desc.entry_number == 7
    assert desc.base_addr == 0x12345678
    assert desc.limit == 0xABCDEF
    assert desc.seg_32bit == 1
    assert desc.contents == 3
    assert desc.read_exec_only == 1
    assert desc.limit_in_pages == 1
    assert desc.seg_not_present == 1
    assert desc.useable == 1


def test_user_desc_has_expected_size():
    assert sizeof(user_regs.UserDesc) == 16


def test_snapshot_info_has_expected_layout():
    assert sizeof(user_regs.SnapshotInfo) == (
        sizeof(user_regs.UserRegsStruct)
        + 3 * sizeof(user_regs.UserDesc)
    )


@pytest.mark.parametrize(
    ("base", "limit", "seg_32bit", "limit_in_pages"),
    [
        (0x00000000, 0x00000000, 0, 0),
        (0x12345678, 0x00000ABC, 0, 0),
        (0x7F001234, 0x0000FFFF, 1, 0),
        (0xCAFEBABE, 0x00ABCDEF, 1, 1),
    ],
)
def test_user_desc_to_gdt_serializes_base_limit_and_flags(
    base: int,
    limit: int,
    seg_32bit: int,
    limit_in_pages: int,
):
    desc = user_regs.UserDesc()
    desc.base_addr = base
    desc.limit = limit
    desc.seg_32bit = seg_32bit
    desc.limit_in_pages = limit_in_pages

    result = user_regs.user_desc_to_gdt(desc)

    access = 0xF2

    flags = 0
    if limit_in_pages:
        flags |= 0x8

    if seg_32bit:
        flags |= 0x4

    expected_value = (
        (limit & 0xFFFF)
        | ((base & 0xFFFF) << 16)
        | (((base >> 16) & 0xFF) << 32)
        | (access << 40)
        | (((limit >> 16) & 0xF) << 48)
        | (flags << 52)
        | (((base >> 24) & 0xFF) << 56)
    )

    assert result == expected_value.to_bytes(8, "little")


def test_user_desc_to_gdt_uses_writable_user_data_access():
    desc = user_regs.UserDesc()
    desc.base_addr = 0x12345000
    desc.limit = 0xFFF

    result = int.from_bytes(
        user_regs.user_desc_to_gdt(desc),
        "little",
    )

    access = (result >> 40) & 0xFF

    assert access == 0xF2

    # P=1, DPL=3, S=1, data, writable.
    assert access & 0x80  # P
    assert access & 0x60 == 0x60  # DPL=3
    assert access & 0x10  # S
    assert access & 0x02  # Writable data


def test_user_desc_to_gdt_preserves_20_bit_limit():
    desc = user_regs.UserDesc()
    desc.limit = 0xABCDEF
    desc.limit_in_pages = 1

    result = int.from_bytes(
        user_regs.user_desc_to_gdt(desc),
        "little",
    )

    # G = bit 55.
    assert result & (1 << 55)

    encoded_limit = (
        (result & 0xFFFF)
        | (((result >> 48) & 0xF) << 16)
    )

    assert encoded_limit == (desc.limit & 0xFFFFF)


def test_user_desc_to_gdt_preserves_32bit_flag():
    desc = user_regs.UserDesc()
    desc.seg_32bit = 1

    result = int.from_bytes(
        user_regs.user_desc_to_gdt(desc),
        "little",
    )

    # D/B = bit 54.
    assert result & (1 << 54)


def test_dump_regs_restores_gprs_and_eflags(
    ql: FakeQiling,
):
    snapshot = make_snapshot()

    ql.mem.map(CODE_ADDR, 1)

    uc: FakeUc = ql.uc

    def emulate_iret(
        _begin: int,
        _until: int,
        _count: int,
    ) -> None:
        ql.arch.regs.eip = snapshot.regs.eip
        ql.arch.regs.esp = snapshot.regs.esp
        ql.arch.regs.eflags = snapshot.regs.eflags

        uc.reg_write(REG_CS, snapshot.regs.xcs)
        uc.reg_write(REG_SS, snapshot.regs.xss)

    uc.emu_start_hook = emulate_iret

    user_regs.dump_regs(ql, snapshot)

    assert ql.arch.regs.eax == 0x77777777
    assert ql.arch.regs.ebx == 0x11111111
    assert ql.arch.regs.ecx == 0x22222222
    assert ql.arch.regs.edx == 0x33333333
    assert ql.arch.regs.esi == 0x44444444
    assert ql.arch.regs.edi == 0x55555555
    assert ql.arch.regs.ebp == 0x66666666
    assert ql.arch.regs.eflags == 0x202
    assert ql.arch.regs.esp == snapshot.regs.esp
    assert ql.arch.regs.eip == snapshot.regs.eip


def test_dump_regs_restores_segment_registers(
    ql: FakeQiling,
):
    snapshot = make_snapshot()

    ql.mem.map(CODE_ADDR, 1)

    uc: FakeUc = ql.uc

    def emulate_iret(
        _begin: int,
        _until: int,
        _count: int,
    ) -> None:
        ql.arch.regs.eip = snapshot.regs.eip
        ql.arch.regs.esp = snapshot.regs.esp

        uc.reg_write(REG_CS, snapshot.regs.xcs)
        uc.reg_write(REG_SS, snapshot.regs.xss)

    uc.emu_start_hook = emulate_iret

    user_regs.dump_regs(ql, snapshot)

    assert uc.reg_read(REG_DS) == snapshot.regs.xds
    assert uc.reg_read(REG_ES) == snapshot.regs.xes
    assert uc.reg_read(REG_FS) == snapshot.regs.xfs
    assert uc.reg_read(REG_GS) == snapshot.regs.xgs
    assert uc.reg_read(REG_CS) == snapshot.regs.xcs
    assert uc.reg_read(REG_SS) == snapshot.regs.xss


def test_dump_regs_writes_code_and_data_descriptors(
    ql: FakeQiling,
):
    snapshot = make_snapshot()

    ql.mem.map(CODE_ADDR, 1)

    uc: FakeUc = ql.uc

    def emulate_iret(
        _begin: int,
        _until: int,
        _count: int,
    ) -> None:
        ql.arch.regs.eip = snapshot.regs.eip
        ql.arch.regs.esp = snapshot.regs.esp

        uc.reg_write(REG_CS, snapshot.regs.xcs)
        uc.reg_write(REG_SS, snapshot.regs.xss)

    uc.emu_start_hook = emulate_iret

    user_regs.dump_regs(ql, snapshot)

    ds_raw = ql.mem.read(
        GDT_BASE + (snapshot.regs.xds >> 3) * 8,
        8,
    )
    cs_raw = ql.mem.read(
        GDT_BASE + (snapshot.regs.xcs >> 3) * 8,
        8,
    )

    assert ds_raw == user_regs.RAW_DS_DESCRIPTOR
    assert cs_raw == user_regs.RAW_CS_DESCRIPTOR


def test_dump_regs_restores_nonpresent_tls_as_zero_descriptor(
    ql: FakeQiling,
):
    snapshot = make_snapshot()

    snapshot.tls[0].entry_number = 6
    snapshot.tls[0].seg_not_present = 1

    ql.mem.map(CODE_ADDR, 1)

    uc: FakeUc = ql.uc

    def emulate_iret(
        _begin: int,
        _until: int,
        _count: int,
    ) -> None:
        ql.arch.regs.eip = snapshot.regs.eip
        ql.arch.regs.esp = snapshot.regs.esp

        uc.reg_write(REG_CS, snapshot.regs.xcs)
        uc.reg_write(REG_SS, snapshot.regs.xss)

    uc.emu_start_hook = emulate_iret

    ql.mem.write(
        GDT_BASE + 6 * 8,
        b"\xAA" * 8,
    )

    user_regs.dump_regs(ql, snapshot)

    assert ql.mem.read(
        GDT_BASE + 6 * 8,
        8,
    ) == b"\xAA" * 8


def test_dump_regs_restores_present_tls_descriptor(
    ql: FakeQiling,
):
    snapshot = make_snapshot()

    snapshot.tls[0].entry_number = 6
    snapshot.tls[0].base_addr = 0x12345678
    snapshot.tls[0].limit = 0xABCDEF
    snapshot.tls[0].seg_32bit = 1
    snapshot.tls[0].limit_in_pages = 1

    ql.mem.map(CODE_ADDR, 1)

    uc: FakeUc = ql.uc

    def emulate_iret(
        _begin: int,
        _until: int,
        _count: int,
    ) -> None:
        ql.arch.regs.eip = snapshot.regs.eip
        ql.arch.regs.esp = snapshot.regs.esp

        uc.reg_write(REG_CS, snapshot.regs.xcs)
        uc.reg_write(REG_SS, snapshot.regs.xss)

    uc.emu_start_hook = emulate_iret

    user_regs.dump_regs(ql, snapshot)

    actual = ql.mem.read(
        GDT_BASE + 6 * 8,
        8,
    )
    expected = user_regs.user_desc_to_gdt(
        snapshot.tls[0],
    )

    assert actual == expected


def test_enter_ring3_builds_correct_iret_frame(
    ql: FakeQiling,
):
    snapshot = make_snapshot()

    ql.mem.map(CODE_ADDR, 1)

    uc: FakeUc = ql.uc
    captured: dict[str, object] = {}

    def emulate_iret(
        begin: int,
        until: int,
        count: int,
    ) -> None:
        captured["begin"] = begin
        captured["until"] = until
        captured["count"] = count

        frame = ql.mem.read(
            TRAMPOLINE_ADDR + user_regs.FRAME_OFFSET,
            20,
        )

        captured["frame"] = frame

        eip, cs, eflags, esp, ss = struct.unpack(
            "<IIIII",
            frame,
        )

        ql.arch.regs.eip = eip
        ql.arch.regs.esp = esp
        ql.arch.regs.eflags = eflags

        uc.reg_write(REG_CS, cs)
        uc.reg_write(REG_SS, ss)

    uc.emu_start_hook = emulate_iret

    user_regs.enter_ring3(ql, snapshot)

    assert captured["begin"] == TRAMPOLINE_ADDR
    assert captured["until"] == 0
    assert captured["count"] == 1

    assert captured["frame"] == struct.pack(
        "<IIIII",
        snapshot.regs.eip,
        snapshot.regs.xcs,
        snapshot.regs.eflags,
        snapshot.regs.esp,
        snapshot.regs.xss,
    )

    assert ql.arch.regs.eip == snapshot.regs.eip
    assert ql.arch.regs.esp == snapshot.regs.esp
    assert ql.arch.regs.eflags == snapshot.regs.eflags

    assert uc.reg_read(REG_CS) == snapshot.regs.xcs
    assert uc.reg_read(REG_SS) == snapshot.regs.xss

    assert uc.emu_start_calls == [
        (TRAMPOLINE_ADDR, 0, 1),
    ]

    assert not ql.mem.is_mapped(
        TRAMPOLINE_ADDR,
        user_regs.TRAMPOLINE_SIZE,
    )

    assert ql.mem.read(
        GDT_BASE + 12 * 8,
        8,
    ) == user_regs.NULL_DESCRIPTOR


def test_enter_ring3_writes_ring0_stack_descriptor_before_iret(
    ql: FakeQiling,
):
    snapshot = make_snapshot()

    ql.mem.map(CODE_ADDR, 1)

    uc: FakeUc = ql.uc
    captured: dict[str, bytes] = {}

    def emulate_iret(
        _begin: int,
        _until: int,
        _count: int,
    ) -> None:
        captured["descriptor"] = ql.mem.read(
            GDT_BASE + 12 * 8,
            8,
        )

        ql.arch.regs.eip = snapshot.regs.eip
        ql.arch.regs.esp = snapshot.regs.esp

        uc.reg_write(REG_CS, snapshot.regs.xcs)
        uc.reg_write(REG_SS, snapshot.regs.xss)

    uc.emu_start_hook = emulate_iret

    user_regs.enter_ring3(ql, snapshot)

    assert captured["descriptor"] == user_regs.RAW_R0_DS_DESCRIPTOR

    assert ql.arch.regs.eip == snapshot.regs.eip
    assert ql.arch.regs.esp == snapshot.regs.esp
    assert uc.reg_read(REG_CS) == snapshot.regs.xcs
    assert uc.reg_read(REG_SS) == snapshot.regs.xss


def test_enter_ring3_raises_when_eip_is_not_mapped(
    ql: FakeQiling,
):
    snapshot = make_snapshot()

    with pytest.raises(
        RuntimeError,
        match="is not mapped",
    ):
        user_regs.enter_ring3(ql, snapshot)


def test_enter_ring3_raises_when_no_free_gdt_slot(
    ql: FakeQiling,
):
    snapshot = make_snapshot()

    ql.mem.map(CODE_ADDR, 1)
    ql.os.gdtm._free_idx = -1

    with pytest.raises(
        RuntimeError,
        match="no free GDT entry",
    ):
        user_regs.enter_ring3(ql, snapshot)


def test_enter_ring3_refuses_zero_trampoline_address(
    ql: FakeQiling,
):
    snapshot = make_snapshot()

    ql.mem.map(CODE_ADDR, 1)

    def map_anywhere(
        _size: int,
        perms: int = 0,
        minaddr: int = 0,
        info: str = "",
    ) -> int:
        del perms, minaddr, info
        return 0

    ql.mem.map_anywhere = map_anywhere

    with pytest.raises(
        RuntimeError,
        match="address 0",
    ):
        user_regs.enter_ring3(ql, snapshot)


def test_enter_ring3_cleans_up_after_emulation_error(
    ql: FakeQiling,
):
    snapshot = make_snapshot()

    ql.mem.map(CODE_ADDR, 1)

    uc: FakeUc = ql.uc

    def fail_emulation(
        *_args: object,
        **_kwargs: object,
    ) -> None:
        raise RuntimeError("emulation failed")

    uc.emu_start_hook = fail_emulation

    with pytest.raises(
        RuntimeError,
        match="emulation failed",
    ):
        user_regs.enter_ring3(ql, snapshot)

    assert not ql.mem.is_mapped(
        TRAMPOLINE_ADDR,
        user_regs.TRAMPOLINE_SIZE,
    )

    assert ql.mem.read(
        GDT_BASE + 12 * 8,
        8,
    ) == user_regs.NULL_DESCRIPTOR


def test_enter_ring3_detects_failed_ring3_switch(
    ql: FakeQiling,
):
    snapshot = make_snapshot()

    ql.mem.map(CODE_ADDR, 1)

    uc: FakeUc = ql.uc

    def emulate_broken_iret(
        _begin: int,
        _until: int,
        _count: int,
    ) -> None:
        ql.arch.regs.eip = 0xDEADBEEF
        ql.arch.regs.esp = snapshot.regs.esp

        uc.reg_write(REG_CS, 0x08)
        uc.reg_write(REG_SS, 0x10)

    uc.emu_start_hook = emulate_broken_iret

    with pytest.raises(
        RuntimeError,
        match="ring3 switch failed",
    ):
        user_regs.enter_ring3(ql, snapshot)

    assert not ql.mem.is_mapped(
        TRAMPOLINE_ADDR,
        user_regs.TRAMPOLINE_SIZE,
    )

    assert ql.mem.read(
        GDT_BASE + 12 * 8,
        8,
    ) == user_regs.NULL_DESCRIPTOR


def test_enter_ring3_requires_user_cs_and_ss(
    ql: FakeQiling,
):
    snapshot = make_snapshot()

    ql.mem.map(CODE_ADDR, 1)

    uc: FakeUc = ql.uc

    def emulate_wrong_privilege(
        _begin: int,
        _until: int,
        _count: int,
    ) -> None:
        ql.arch.regs.eip = snapshot.regs.eip
        ql.arch.regs.esp = snapshot.regs.esp

        uc.reg_write(
            REG_CS,
            snapshot.regs.xcs & ~0x3,
        )
        uc.reg_write(
            REG_SS,
            snapshot.regs.xss & ~0x3,
        )

    uc.emu_start_hook = emulate_wrong_privilege

    with pytest.raises(
        RuntimeError,
        match="ring3 switch failed",
    ):
        user_regs.enter_ring3(ql, snapshot)

