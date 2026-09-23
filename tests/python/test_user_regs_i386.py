import ctypes
import struct

import pytest

pytest.importorskip("qiling")
pytest.importorskip("unicorn")

from unicorn.x86_const import (
    UC_X86_REG_CS,
    UC_X86_REG_SS,
)
from unittest.mock import MagicMock

from process_bridge.i386.user_regs_struct import (
    DescStruct,
    FRAME_OFFSET,
    IRET32,
    NULL_DESCRIPTOR,
    RAW_R0_DS_DESCRIPTOR,
    COMPATIBILITY_GDT_ENTRY_DEFAULT_USER32_CS,
    TRAMPOLINE_SIZE,
    TRUE_GDT_ENTRY_DEFAULT_USER_CS,
    SnapshotInfo,
    UserDesc,
    UserRegsStruct,
    dump_regs,
    enter_ring3,
    read_gdt,
    user_desc_to_real_desc_bytes,
)


class TestStructLayouts:
    def test_descstruct_is_eight_bytes(self):
        assert ctypes.sizeof(DescStruct) == 8

    def test_userdesc_is_sixteen_bytes(self):
        assert ctypes.sizeof(UserDesc) == 16

    def test_userregsstruct_is_i386_pt_regs_size(self):
        assert ctypes.sizeof(UserRegsStruct) == 17 * 4

    def test_userregsstruct_field_order_matches_kernel_layout(self):
        values = list(range(1, 18))
        raw = struct.pack("<17I", *values)
        regs = UserRegsStruct.from_buffer_copy(raw)

        assert regs.ebx == 1
        assert regs.ecx == 2
        assert regs.edx == 3
        assert regs.esi == 4
        assert regs.edi == 5
        assert regs.ebp == 6
        assert regs.eax == 7
        assert regs.xds == 8
        assert regs.xes == 9
        assert regs.xfs == 10
        assert regs.xgs == 11
        assert regs.orig_eax == 12
        assert regs.eip == 13
        assert regs.xcs == 14
        assert regs.eflags == 15
        assert regs.esp == 16
        assert regs.xss == 17

    def test_snapshotinfo_holds_regs_and_three_tls_entries(self):
        assert ctypes.sizeof(SnapshotInfo) == ctypes.sizeof(UserRegsStruct) + 3 * ctypes.sizeof(UserDesc)


def make_user_desc(**overrides):
    defaults = dict(
        entry_number=6,
        base_addr=0x08048000,
        limit=0xFFFFF,
        seg_32bit=1,
        contents=0,
        read_exec_only=0,
        limit_in_pages=1,
        seg_not_present=0,
        useable=1,
    )
    defaults.update(overrides)
    return UserDesc(**defaults)


class TestUserDescToRealDescBytes:
    def test_returns_eight_bytes(self):
        result = user_desc_to_real_desc_bytes(make_user_desc())
        assert len(result) == 8

    def test_data_segment_fields_mapped_correctly(self):
        desc = make_user_desc(
            base_addr=0x08048000,
            limit=0xFFFFF,
            seg_32bit=1,
            contents=0,
            read_exec_only=0,
            limit_in_pages=1,
            seg_not_present=0,
            useable=1,
        )
        decoded = DescStruct.from_buffer_copy(user_desc_to_real_desc_bytes(desc))

        assert decoded.limit0 == 0xFFFF
        assert decoded.base0 == 0x8000
        assert decoded.base1 == 0x04
        assert decoded.base2 == 0x08
        assert decoded.limit1 == 0xF
        assert decoded.type == 3
        assert decoded.s == 1
        assert decoded.dpl == 0x3
        assert decoded.p == 1
        assert decoded.avl == 1
        assert decoded.d == 1
        assert decoded.g == 1
        assert decoded.l == 0

    def test_code_segment_read_exec_only_flag_changes_type(self):
        desc = make_user_desc(contents=2, read_exec_only=1)
        decoded = DescStruct.from_buffer_copy(user_desc_to_real_desc_bytes(desc))
        assert decoded.type == 9

    def test_not_present_segment_clears_p_bit(self):
        desc = make_user_desc(seg_not_present=1)
        decoded = DescStruct.from_buffer_copy(user_desc_to_real_desc_bytes(desc))
        assert decoded.p == 0

    def test_dpl_is_always_ring3(self):
        desc = make_user_desc()
        decoded = DescStruct.from_buffer_copy(user_desc_to_real_desc_bytes(desc))
        assert decoded.dpl == 3


def make_snapshot_info(**reg_overrides):
    info = SnapshotInfo()
    for field, value in dict(
        eip=0x08049000,
        xcs=0x1B,
        eflags=0x202,
        esp=0xFFFFE000,
        xss=0x23,
    ).items():
        setattr(info.regs, field, reg_overrides.get(field, value))
    for i in range(3):
        info.tls[i].seg_not_present = 1
    return info


def make_ql(*, is_mapped=True, free_idx=6, trampoline_addr=0x40000000, hook_handle="hook-1"):
    ql = MagicMock()
    ql.mem.is_mapped.return_value = is_mapped
    ql.os.gdtm.get_free_idx.return_value = free_idx
    ql.os.gdtm.array.base = 0x1000
    ql.mem.map_anywhere.return_value = trampoline_addr
    ql.uc.hook_add.return_value = hook_handle
    return ql


class TestEnterRing3:
    def test_raises_if_eip_not_mapped(self):
        ql = make_ql(is_mapped=False)
        info = make_snapshot_info()
        with pytest.raises(RuntimeError, match="not mapped"):
            enter_ring3(ql, info)

    def test_raises_if_no_free_gdt_entry(self):
        ql = make_ql(free_idx=-1)
        info = make_snapshot_info()
        with pytest.raises(RuntimeError, match="GDT entry"):
            enter_ring3(ql, info)

    def test_raises_if_trampoline_placed_at_zero(self):
        ql = make_ql(trampoline_addr=0)
        info = make_snapshot_info()
        with pytest.raises(RuntimeError, match="address 0"):
            enter_ring3(ql, info)

    def test_returns_trampoline_address(self):
        ql = make_ql(trampoline_addr=0x40000000)
        info = make_snapshot_info()
        assert enter_ring3(ql, info) == 0x40000000

    def test_writes_scratch_ds_descriptor_and_iret_stub(self):
        ql = make_ql(free_idx=6, trampoline_addr=0x40000000)
        info = make_snapshot_info()
        enter_ring3(ql, info)

        gdt_base = ql.os.gdtm.array.base
        ql.mem.write.assert_any_call(gdt_base + 6 * 8, RAW_R0_DS_DESCRIPTOR)
        ql.mem.write.assert_any_call(0x40000000, IRET32)

    def test_writes_iret_frame_with_snapshot_registers(self):
        ql = make_ql(trampoline_addr=0x40000000)
        info = make_snapshot_info(eip=0x1111, xcs=0x1B, eflags=0x202, esp=0x2222, xss=0x23)
        enter_ring3(ql, info)

        expected_frame = struct.pack("<IIIII", 0x1111, 0x1B, 0x202, 0x2222, 0x23)
        ql.mem.write.assert_any_call(0x40000000 + FRAME_OFFSET, expected_frame)

    def test_sets_ss_and_esp_for_the_trampoline_stack(self):
        ql = make_ql(free_idx=6, trampoline_addr=0x40000000)
        info = make_snapshot_info()
        enter_ring3(ql, info)

        ql.uc.reg_write.assert_any_call(UC_X86_REG_SS, 6 << 3)
        assert ql.arch.regs.esp == 0x40000000 + FRAME_OFFSET

    def test_registers_breakpoint_hook_at_target_eip(self):
        ql = make_ql()
        info = make_snapshot_info(eip=0x08049000)
        enter_ring3(ql, info)

        _, kwargs = ql.uc.hook_add.call_args
        assert kwargs["begin"] == 0x08049000
        assert kwargs["end"] == 0x08049000

    def test_hook_callback_cleans_up_on_successful_ring3_switch(self):
        ql = make_ql(free_idx=6, trampoline_addr=0x40000000, hook_handle="hook-1")
        info = make_snapshot_info()
        enter_ring3(ql, info)

        callback = ql.uc.hook_add.call_args[0][1]
        uc = MagicMock()
        uc.reg_read.side_effect = lambda reg: 0x1B if reg == UC_X86_REG_CS else 0x23

        callback(uc, 0x08049000, 0, None)

        uc.hook_del.assert_called_once_with("hook-1")
        ql.mem.write.assert_any_call(ql.os.gdtm.array.base + 6 * 8, NULL_DESCRIPTOR)
        ql.mem.unmap.assert_called_once_with(0x40000000, TRAMPOLINE_SIZE)

    def test_hook_callback_raises_if_still_in_ring0(self):
        ql = make_ql()
        info = make_snapshot_info()
        enter_ring3(ql, info)

        callback = ql.uc.hook_add.call_args[0][1]
        uc = MagicMock()
        uc.reg_read.side_effect = lambda reg: 0x08 if reg == UC_X86_REG_CS else 0x23

        with pytest.raises(RuntimeError, match="ring3 switch failed"):
            callback(uc, 0x08049000, 0, None)



class TestReadGdt:
    def test_reads_default_sixteen_entries(self):
        ql = MagicMock()
        ql.mem.read.return_value = b"\x00" * 8
        read_gdt(ql, base=0x1000)
        assert ql.mem.read.call_count == 16

    def test_respects_custom_count(self):
        ql = MagicMock()
        ql.mem.read.return_value = b"\x00" * 8
        read_gdt(ql, base=0x1000, count=3)
        assert ql.mem.read.call_count == 3
        ql.mem.read.assert_any_call(0x1000 + 2 * 8, 8)

    def test_does_not_raise_on_a_populated_descriptor(self):
        ql = MagicMock()
        ql.mem.read.return_value = RAW_R0_DS_DESCRIPTOR
        read_gdt(ql, base=0x1000, count=1)


def make_dump_ql():
    ql = MagicMock()
    ql.os.gdtm.array.base = 0x1000
    return ql


def make_dump_snapshot(xcs, xss=0x23, tls_present=(False, False, False)):
    info = make_snapshot_info(xcs=xcs, xss=xss)
    for i, present in enumerate(tls_present):
        info.tls[i].seg_not_present = 0 if present else 1
        info.tls[i].entry_number = 20 + i
    return info


class TestDumpRegs:
    def test_rejects_ldt_selectors(self, monkeypatch):
        monkeypatch.setattr(
            "process_bridge.i386.user_regs_struct.enter_ring3",
            lambda ql, info: 0x40000000,
        )
        ql = make_dump_ql()
        info = make_dump_snapshot(xcs=TRUE_GDT_ENTRY_DEFAULT_USER_CS << 3, xss=0x27)
        with pytest.raises(RuntimeError, match="LDT not supported"):
            dump_regs(ql, info)

    def test_rejects_non_default_user_cs(self, monkeypatch):
        monkeypatch.setattr(
            "process_bridge.i386.user_regs_struct.enter_ring3",
            lambda ql, info: 0x40000000,
        )
        ql = make_dump_ql()
        info = make_dump_snapshot(xcs=0x08)
        with pytest.raises(RuntimeError, match="default user cs"):
            dump_regs(ql, info)

    def test_true_arch_cs_selects_data32_and_calls_enter_ring3(self, monkeypatch):
        enter_ring3_mock = MagicMock(return_value=0x40000000)
        monkeypatch.setattr(
            "process_bridge.i386.user_regs_struct.enter_ring3", enter_ring3_mock
        )
        ql = make_dump_ql()
        info = make_dump_snapshot(xcs=TRUE_GDT_ENTRY_DEFAULT_USER_CS << 3)

        result = dump_regs(ql, info)

        assert result == 0x40000000
        enter_ring3_mock.assert_called_once_with(ql, info)

    def test_compatibility_cs_selects_data64(self, monkeypatch):
        monkeypatch.setattr(
            "process_bridge.i386.user_regs_struct.enter_ring3",
            lambda ql, info: 0x40000000,
        )
        ql = make_dump_ql()
        info = make_dump_snapshot(xcs=COMPATIBILITY_GDT_ENTRY_DEFAULT_USER32_CS << 3)

        dump_regs(ql, info)

    def test_restores_general_purpose_registers(self, monkeypatch):
        monkeypatch.setattr(
            "process_bridge.i386.user_regs_struct.enter_ring3",
            lambda ql, info: 0x40000000,
        )
        ql = make_dump_ql()
        info = make_dump_snapshot(xcs=TRUE_GDT_ENTRY_DEFAULT_USER_CS << 3)
        info.regs.eax = 0xAA
        info.regs.ebx = 0xBB
        info.regs.ecx = 0xCC
        info.regs.edx = 0xDD

        dump_regs(ql, info)

        assert ql.arch.regs.eax == 0xAA
        assert ql.arch.regs.ebx == 0xBB
        assert ql.arch.regs.ecx == 0xCC
        assert ql.arch.regs.edx == 0xDD

    def test_writes_present_tls_entries_and_skips_absent_ones(self, monkeypatch):
        monkeypatch.setattr(
            "process_bridge.i386.user_regs_struct.enter_ring3",
            lambda ql, info: 0x40000000,
        )
        ql = make_dump_ql()
        info = make_dump_snapshot(
            xcs=TRUE_GDT_ENTRY_DEFAULT_USER_CS << 3,
            tls_present=(True, False, True),
        )

        dump_regs(ql, info)

        written_addrs = [c.args[0] for c in ql.mem.write.call_args_list]
        assert (0x1000 + 20 * 8) in written_addrs
        assert (0x1000 + 21 * 8) not in written_addrs
        assert (0x1000 + 22 * 8) in written_addrs

    def test_returns_trampoline_from_enter_ring3(self, monkeypatch):
        monkeypatch.setattr(
            "process_bridge.i386.user_regs_struct.enter_ring3",
            lambda ql, info: 0xDEADBEEF,
        )
        ql = make_dump_ql()
        info = make_dump_snapshot(xcs=TRUE_GDT_ENTRY_DEFAULT_USER_CS << 3)
        assert dump_regs(ql, info) == 0xDEADBEEF
