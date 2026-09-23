import struct

from ctypes import Structure, c_uint32, c_uint16, c_uint
from qiling import Qiling
from unicorn import UC_PROT_ALL, UC_HOOK_CODE
from unicorn.x86_const import *
from process_bridge.gdt_utils import *

from ..log import get_logger

log = get_logger(__name__)

COMPATIBILITY_GDT_ENTRY_DEFAULT_USER32_CS = 4
COMPATIBILITY_GDT_ENTRY_DEFAULT_USER_DS = 5

TRUE_GDT_ENTRY_DEFAULT_USER_CS = 14
TRUE_GDT_ENTRY_DEFAULT_USER_DS = 15

RAW_R0_DS_DESCRIPTOR = bytes.fromhex("FFFF00000093CF00")
NULL_DESCRIPTOR = b"\x00" * 8
TRAMPOLINE_SIZE = 0x1000
FRAME_OFFSET = 0x800
IRET32 = b"\xcf"


class UserRegsStruct(Structure):
    ebp: int = 0
    ebx: int = 0
    eax: int = 0
    ecx: int = 0
    edx: int = 0
    esi: int = 0
    edi: int = 0
    orig_eax: int = 0
    eip: int = 0
    xcs: int = 0
    eflags: int = 0
    esp: int = 0
    xss: int = 0
    xds: int = 0
    xes: int = 0
    xfs: int = 0
    xgs: int = 0

    _fields_ = [
        ("ebx", c_uint32),
        ("ecx", c_uint32),
        ("edx", c_uint32),
        ("esi", c_uint32),
        ("edi", c_uint32),
        ("ebp", c_uint32),
        ("eax", c_uint32),
        ("xds", c_uint32),
        ("xes", c_uint32),
        ("xfs", c_uint32),
        ("xgs", c_uint32),
        ("orig_eax", c_uint32),
        ("eip", c_uint32),
        ("xcs", c_uint32),
        ("eflags", c_uint32),
        ("esp", c_uint32),
        ("xss", c_uint32),
    ]


class UserDesc(Structure):
    entry_number: int = 0
    base_addr: int = 0
    limit: int = 0
    seg_32bit: int = 0
    contents: int = 0
    read_exec_only: int = 0
    limit_in_pages: int = 0
    seg_not_present: int = 0
    useable: int = 0

    _fields_ = [
        ("entry_number", c_uint32),
        ("base_addr", c_uint32),
        ("limit", c_uint32),
        ("seg_32bit", c_uint, 1),
        ("contents", c_uint, 2),
        ("read_exec_only", c_uint, 1),
        ("limit_in_pages", c_uint, 1),
        ("seg_not_present", c_uint, 1),
        ("useable", c_uint, 1),
    ]


class SnapshotInfo(Structure):
    regs: UserRegsStruct = UserRegsStruct()
    tls: list[UserDesc] = [UserDesc()] * 3

    _fields_ = [("regs", UserRegsStruct), ("tls", UserDesc * 3)]


class DescStruct(Structure):
    limit0: int = 0
    base0: int = 0
    base1: int = 0
    type: int = 0
    s: int = 0
    dpl: int = 0
    p: int = 0
    limit1: int = 0
    avl: int = 0
    l: int = 0
    d: int = 0
    g: int = 0
    base2: int = 0

    _fields_ = [
        ("limit0", c_uint16),
        ("base0", c_uint16),
        ("base1", c_uint, 8),
        ("type", c_uint, 4),
        ("s", c_uint, 1),
        ("dpl", c_uint, 2),
        ("p", c_uint, 1),
        ("limit1", c_uint, 4),
        ("avl", c_uint, 1),
        ("l", c_uint, 1),
        ("d", c_uint, 1),
        ("g", c_uint, 1),
        ("base2", c_uint, 8),
    ]


def user_desc_to_real_desc_bytes(user_desc: UserDesc) -> bytes:
    desc_struct = DescStruct(
        limit0=user_desc.limit & 0xFFFF,
        base0=user_desc.base_addr & 0xFFFF,
        base1=(user_desc.base_addr & 0xFF0000) >> 16,
        type=((user_desc.read_exec_only ^ 1) << 1) | (user_desc.contents << 2) | 1,
        s=1,
        dpl=0x3,
        p=user_desc.seg_not_present ^ 1,
        limit1=(user_desc.limit & 0xF0000) >> 16,
        avl=user_desc.useable,
        d=user_desc.seg_32bit,
        g=user_desc.limit_in_pages,
        base2=(user_desc.base_addr & 0xFF000000) >> 24,
    )

    return bytes(desc_struct)


def enter_ring3(ql: Qiling, snapshot_info: SnapshotInfo) -> int:
    regs = snapshot_info.regs

    if not ql.mem.is_mapped(regs.eip, 1):
        raise RuntimeError(
            f"target eip {regs.eip:#x} is not mapped — restore memory "
            f"mappings before calling enter_ring3()"
        )

    gdt_base = ql.os.gdtm.array.base

    scratch_idx = ql.os.gdtm.get_free_idx()
    if scratch_idx < 0:
        raise RuntimeError("no free GDT entry for the ring0 trampoline stack")

    trampoline = ql.mem.map_anywhere(
        TRAMPOLINE_SIZE,
        perms=UC_PROT_ALL,
        minaddr=ql.mem.pagesize,
        info="[process-bridge:iret]",
    )

    if trampoline == 0:
        raise RuntimeError("refusing to place the iret trampoline at address 0")

    frame = trampoline + FRAME_OFFSET

    ql.mem.write(gdt_base + scratch_idx * 8, RAW_R0_DS_DESCRIPTOR)
    ql.mem.write(trampoline, IRET32)
    ql.mem.write(
        frame,
        struct.pack("<IIIII", regs.eip, regs.xcs, regs.eflags, regs.esp, regs.xss),
    )

    ql.uc.reg_write(UC_X86_REG_SS, scratch_idx << 3)
    ql.arch.regs.esp = frame

    log.debug(
        "iret trampoline at %#x, frame at %#x -> %#x:%#x (ss:esp %#x:%#x)",
        trampoline,
        frame,
        regs.xcs,
        regs.eip,
        regs.xss,
        regs.esp,
    )

    handle = []

    def verify_and_cleanup(uc, address, size, user_data):
        cs = uc.reg_read(UC_X86_REG_CS)
        ss = uc.reg_read(UC_X86_REG_SS)

        uc.hook_del(handle[0])

        if cs & 0b11 != 0b11 or ss & 0b11 != 0b11:
            raise RuntimeError(f"ring3 switch failed: cs={cs:#x} ss={ss:#x}")

        ql.mem.write(gdt_base + scratch_idx * 8, NULL_DESCRIPTOR)
        ql.mem.unmap(trampoline, TRAMPOLINE_SIZE)

    handle.append(
        ql.uc.hook_add(UC_HOOK_CODE, verify_and_cleanup, begin=regs.eip, end=regs.eip)
    )

    return trampoline


def read_gdt(ql: Qiling, base: int, count: int = 16) -> None:
    for i in range(count):
        raw = ql.mem.read(base + i * 8, 8)
        value = int.from_bytes(raw, "little")

        limit = (value & 0xFFFF) | (((value >> 48) & 0xF) << 16)

        segment_base = (
            ((value >> 16) & 0xFFFF)
            | (((value >> 32) & 0xFF) << 16)
            | (((value >> 56) & 0xFF) << 24)
        )

        access = (value >> 40) & 0xFF
        flags = (value >> 52) & 0xF

        log.debug(
            "GDT[%02d] raw=%016x base=%08x limit=%08x " "access=%02x flags=%x",
            i,
            value,
            segment_base,
            limit,
            access,
            flags,
        )


def dump_regs(ql: Qiling, snapshot_info: SnapshotInfo) -> int:
    gdt_base = ql.os.gdtm.array.base

    segment_regs = [
        snapshot_info.regs.xcs,
        snapshot_info.regs.xds,
        snapshot_info.regs.xes,
        snapshot_info.regs.xss,
        snapshot_info.regs.xfs,
        snapshot_info.regs.xgs,
    ]

    if any(seg & 0x4 for seg in segment_regs):
        raise RuntimeError("LDT not supported")

    code_desc = make_gdt_entry_init(DESC_CODE32 | DESC_USER, 0, 0xFFFFF)
    data_desc = -1

    if (snapshot_info.regs.xcs >> 3) == COMPATIBILITY_GDT_ENTRY_DEFAULT_USER32_CS:
        data_desc = make_gdt_entry_init(DESC_DATA64 | DESC_USER, 0, 0xFFFFF)
    elif (snapshot_info.regs.xcs >> 3) == TRUE_GDT_ENTRY_DEFAULT_USER_CS:
        data_desc = make_gdt_entry_init(DESC_DATA32 | DESC_USER, 0, 0xFFFFF)
    else:
        raise RuntimeError("cs is not the default user cs")

    ql.mem.write(gdt_base + (snapshot_info.regs.xcs >> 3) * 8, code_desc)
    log.debug("restored code descriptor: %#x", int.from_bytes(code_desc, "little"))

    ql.mem.write(gdt_base + (snapshot_info.regs.xss >> 3) * 8, data_desc)
    log.debug("restored data descriptor: %#x", int.from_bytes(data_desc, "little"))

    for desc in snapshot_info.tls:
        ql.mem.write(
            gdt_base + desc.entry_number * 8, user_desc_to_real_desc_bytes(desc)
        )

    ql.arch.regs.eax = snapshot_info.regs.eax
    ql.arch.regs.ebx = snapshot_info.regs.ebx
    ql.arch.regs.ecx = snapshot_info.regs.ecx
    ql.arch.regs.edx = snapshot_info.regs.edx
    ql.arch.regs.esi = snapshot_info.regs.esi
    ql.arch.regs.edi = snapshot_info.regs.edi
    ql.arch.regs.ebp = snapshot_info.regs.ebp
    ql.arch.regs.eflags = snapshot_info.regs.eflags

    ql.uc.reg_write(UC_X86_REG_DS, snapshot_info.regs.xds)
    ql.uc.reg_write(UC_X86_REG_ES, snapshot_info.regs.xes)
    ql.uc.reg_write(UC_X86_REG_FS, snapshot_info.regs.xfs)
    ql.uc.reg_write(UC_X86_REG_GS, snapshot_info.regs.xgs)

    trampoline = enter_ring3(ql, snapshot_info)

    log.debug(
        "restored regs: eax=%#x ebx=%#x ecx=%#x edx=%#x esi=%#x edi=%#x "
        "ebp=%#x esp=%#x eip=%#x eflags=%#x",
        ql.arch.regs.eax,
        ql.arch.regs.ebx,
        ql.arch.regs.ecx,
        ql.arch.regs.edx,
        ql.arch.regs.esi,
        ql.arch.regs.edi,
        ql.arch.regs.ebp,
        ql.arch.regs.esp,
        ql.arch.regs.eip,
        ql.arch.regs.eflags,
    )

    log.debug(
        "segment registers after write: " "cs=%#x ss=%#x ds=%#x es=%#x fs=%#x gs=%#x",
        ql.uc.reg_read(UC_X86_REG_CS),
        ql.uc.reg_read(UC_X86_REG_SS),
        ql.uc.reg_read(UC_X86_REG_DS),
        ql.uc.reg_read(UC_X86_REG_ES),
        ql.uc.reg_read(UC_X86_REG_FS),
        ql.uc.reg_read(UC_X86_REG_GS),
    )

    for i in range(3):
        log.debug(
            "restored user_desc: entry_number=%#x base_addr=%#x limit=%#x "
            "seg_32bit=%#x contents=%#x read_exec_only=%#x limit_in_pages=%#x "
            "seg_not_present=%#x useable=%#x",
            snapshot_info.tls[i].entry_number,
            snapshot_info.tls[i].base_addr,
            snapshot_info.tls[i].limit,
            snapshot_info.tls[i].seg_32bit,
            snapshot_info.tls[i].contents,
            snapshot_info.tls[i].read_exec_only,
            snapshot_info.tls[i].limit_in_pages,
            snapshot_info.tls[i].seg_not_present,
            snapshot_info.tls[i].useable,
        )
    read_gdt(ql, gdt_base)

    return trampoline
