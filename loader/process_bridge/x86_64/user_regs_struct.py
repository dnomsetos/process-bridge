from process_bridge.gdt_utils import *

from ctypes import Structure, c_ulonglong
from qiling import Qiling
from unicorn.x86_const import *

from ..log import get_logger

log = get_logger(__name__)

GDT_ENTRY_DEFAULT_USER_DS = 5
GDT_ENTRY_DEFAULT_USER_CS = 6


class UserRegsStruct(Structure):
    r15: int = 0
    r14: int = 0
    r13: int = 0
    r12: int = 0
    rbp: int = 0
    rbx: int = 0
    r11: int = 0
    r10: int = 0
    r9: int = 0
    r8: int = 0
    rax: int = 0
    rcx: int = 0
    rdx: int = 0
    rsi: int = 0
    rdi: int = 0
    orig_rax: int = 0
    rip: int = 0
    cs: int = 0
    elflags: int = 0
    rsp: int = 0
    ss: int = 0
    fs_base: int = 0
    gs_base: int = 0
    ds: int = 0
    es: int = 0
    fs: int = 0
    gs: int = 0

    _fields_ = [
        ("r15", c_ulonglong),
        ("r14", c_ulonglong),
        ("r13", c_ulonglong),
        ("r12", c_ulonglong),
        ("rbp", c_ulonglong),
        ("rbx", c_ulonglong),
        ("r11", c_ulonglong),
        ("r10", c_ulonglong),
        ("r9", c_ulonglong),
        ("r8", c_ulonglong),
        ("rax", c_ulonglong),
        ("rcx", c_ulonglong),
        ("rdx", c_ulonglong),
        ("rsi", c_ulonglong),
        ("rdi", c_ulonglong),
        ("orig_rax", c_ulonglong),
        ("rip", c_ulonglong),
        ("cs", c_ulonglong),
        ("elflags", c_ulonglong),
        ("rsp", c_ulonglong),
        ("ss", c_ulonglong),
        ("fs_base", c_ulonglong),
        ("gs_base", c_ulonglong),
        ("ds", c_ulonglong),
        ("es", c_ulonglong),
        ("fs", c_ulonglong),
        ("gs", c_ulonglong),
    ]


class SnapshotInfo(Structure):
    regs: UserRegsStruct = UserRegsStruct()

    _fields_ = [("regs", UserRegsStruct)]


def dump_regs(ql: Qiling, snapshot_info: SnapshotInfo) -> int:
    gdt_base = ql.os.gdtm.array.base

    segment_regs = [
        snapshot_info.regs.cs,
        snapshot_info.regs.ds,
        snapshot_info.regs.es,
        snapshot_info.regs.ss,
        snapshot_info.regs.fs,
        snapshot_info.regs.gs,
    ]

    if any(seg & 0x4 for seg in segment_regs):
        raise RuntimeError("LDT not supported")

    if (snapshot_info.regs.cs >> 3) != GDT_ENTRY_DEFAULT_USER_CS:
        raise RuntimeError("cs is not the default user cs")

    code_desc = make_gdt_entry_init(DESC_CODE64 | DESC_USER, 0, 0xFFFFF)
    ql.mem.write(gdt_base + (snapshot_info.regs.cs >> 3) * 8, code_desc)
    log.debug("restored code descriptor: %#x", int.from_bytes(code_desc, "little"))

    if (snapshot_info.regs.ss >> 3) != GDT_ENTRY_DEFAULT_USER_DS:
        raise RuntimeError("ss is not the default user ds")

    data_desc = make_gdt_entry_init(DESC_DATA64 | DESC_USER, 0, 0xFFFFF)
    ql.mem.write(gdt_base + (snapshot_info.regs.ss >> 3) * 8, data_desc)
    log.debug("restored data descriptor: %#x", int.from_bytes(data_desc, "little"))

    if (snapshot_info.regs.ds >> 3) != 0 and (
        snapshot_info.regs.ds >> 3
    ) != GDT_ENTRY_DEFAULT_USER_DS:
        raise RuntimeError("ds is not the default user ds")

    if (snapshot_info.regs.es >> 3 != 0) and (
        snapshot_info.regs.es >> 3
    ) != GDT_ENTRY_DEFAULT_USER_DS:
        raise RuntimeError("es is not the default user ds")

    ql.arch.regs.rax = snapshot_info.regs.rax
    ql.arch.regs.rbx = snapshot_info.regs.rbx
    ql.arch.regs.rcx = snapshot_info.regs.rcx
    ql.arch.regs.rdx = snapshot_info.regs.rdx
    ql.arch.regs.rsi = snapshot_info.regs.rsi
    ql.arch.regs.rdi = snapshot_info.regs.rdi
    ql.arch.regs.rbp = snapshot_info.regs.rbp
    ql.arch.regs.rsp = snapshot_info.regs.rsp
    ql.arch.regs.r8 = snapshot_info.regs.r8
    ql.arch.regs.r9 = snapshot_info.regs.r9
    ql.arch.regs.r10 = snapshot_info.regs.r10
    ql.arch.regs.r11 = snapshot_info.regs.r11
    ql.arch.regs.r12 = snapshot_info.regs.r12
    ql.arch.regs.r13 = snapshot_info.regs.r13
    ql.arch.regs.r14 = snapshot_info.regs.r14
    ql.arch.regs.r15 = snapshot_info.regs.r15
    ql.arch.regs.rip = snapshot_info.regs.rip
    ql.arch.regs.eflags = snapshot_info.regs.elflags

    ql.uc.reg_write(UC_X86_REG_DS, snapshot_info.regs.ds)
    ql.uc.reg_write(UC_X86_REG_ES, snapshot_info.regs.es)
    ql.uc.reg_write(UC_X86_REG_FS, snapshot_info.regs.fs)
    ql.uc.reg_write(UC_X86_REG_GS, snapshot_info.regs.gs)
    ql.uc.reg_write(UC_X86_REG_CS, snapshot_info.regs.cs)
    ql.uc.reg_write(UC_X86_REG_SS, snapshot_info.regs.ss)

    ql.uc.reg_write(UC_X86_REG_FS_BASE, snapshot_info.regs.fs_base)
    ql.uc.reg_write(UC_X86_REG_GS_BASE, snapshot_info.regs.gs_base)

    log.debug(
        "restored regs: rax=%#x rbx=%#x rcx=%#x rdx=%#x rsi=%#x rdi=%#x "
        "rbp=%#x rsp=%#x r8=%#x r9=%#x r10=%#x r11=%#x r12=%#x r13=%#x "
        "r14=%#x r15=%#x rip=%#x eflags=%#x fs=%#x gs=%#x fs_base=%#x gs_base=%#x",
        snapshot_info.regs.rax,
        snapshot_info.regs.rbx,
        snapshot_info.regs.rcx,
        snapshot_info.regs.rdx,
        snapshot_info.regs.rsi,
        snapshot_info.regs.rdi,
        snapshot_info.regs.rbp,
        snapshot_info.regs.rsp,
        snapshot_info.regs.r8,
        snapshot_info.regs.r9,
        snapshot_info.regs.r10,
        snapshot_info.regs.r11,
        snapshot_info.regs.r12,
        snapshot_info.regs.r13,
        snapshot_info.regs.r14,
        snapshot_info.regs.r15,
        snapshot_info.regs.rip,
        snapshot_info.regs.elflags,
        ql.uc.reg_read(UC_X86_REG_FS),
        ql.uc.reg_read(UC_X86_REG_GS),
        ql.uc.reg_read(UC_X86_REG_FS_BASE),
        ql.uc.reg_read(UC_X86_REG_GS_BASE),
    )

    return ql.arch.regs.rip
