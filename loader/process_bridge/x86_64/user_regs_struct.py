from ctypes import Structure, c_ulonglong
from qiling import Qiling


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


def dump_regs(ql: Qiling, snapshot_info: SnapshotInfo) -> None:
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
    ql.arch.regs.fsbase = snapshot_info.regs.fs_base
    ql.arch.regs.gsbase = snapshot_info.regs.gs_base
