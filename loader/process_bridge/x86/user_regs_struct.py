from ctypes import Structure, c_uint32
from qiling import Qiling
from unicorn.x86_const import *
from qiling.arch.x86_const import (
    QL_X86_A_PRESENT,
    QL_X86_A_PRIV_3,
    QL_X86_A_DATA,
    QL_X86_A_DATA_E,
    QL_X86_A_DESC_DATA,
    QL_X86_A_DATA_W,
)

from ..log import get_logger

log = get_logger(__name__)


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


class TLSSegment(Structure):
    base_addr: int = 0
    limit: int = 0
    flags: int = 0

    _fields_ = [
        ("base_addr", c_uint32),
        ("limit", c_uint32),
        ("flags", c_uint32),
    ]


class SnapshotInfo(Structure):
    regs: UserRegsStruct = UserRegsStruct()
    seg: TLSSegment = TLSSegment()

    _fields_ = [
        ("regs", UserRegsStruct),
        ("seg", TLSSegment),
    ]


def dump_regs(ql: Qiling, snapshot_info: SnapshotInfo) -> None:
    ql.arch.regs.eax = snapshot_info.regs.eax
    ql.arch.regs.ebx = snapshot_info.regs.ebx
    ql.arch.regs.ecx = snapshot_info.regs.ecx
    ql.arch.regs.edx = snapshot_info.regs.edx
    ql.arch.regs.esi = snapshot_info.regs.esi
    ql.arch.regs.edi = snapshot_info.regs.edi
    ql.arch.regs.ebp = snapshot_info.regs.ebp
    ql.arch.regs.esp = snapshot_info.regs.esp
    ql.arch.regs.eip = snapshot_info.regs.eip
    ql.arch.regs.eflags = snapshot_info.regs.eflags

    log.debug(
        "restored regs: eax=%#x ebx=%#x ecx=%#x edx=%#x esi=%#x edi=%#x "
        "ebp=%#x esp=%#x eip=%#x eflags=%#x orig_eax=%#x xcs=%#x xds=%#x "
        "xes=%#x xfs=%#x xgs=%#x xss=%#x | tls base=%#x limit=%#x flags=%#x",
        snapshot_info.regs.eax,
        snapshot_info.regs.ebx,
        snapshot_info.regs.ecx,
        snapshot_info.regs.edx,
        snapshot_info.regs.esi,
        snapshot_info.regs.edi,
        snapshot_info.regs.ebp,
        snapshot_info.regs.esp,
        snapshot_info.regs.eip,
        snapshot_info.regs.eflags,
        snapshot_info.regs.orig_eax,
        snapshot_info.regs.xcs,
        snapshot_info.regs.xds,
        snapshot_info.regs.xes,
        snapshot_info.regs.xfs,
        snapshot_info.regs.xgs,
        snapshot_info.regs.xss,
        snapshot_info.seg.base_addr,
        snapshot_info.seg.limit,
        snapshot_info.seg.flags,
    )


def restore_tls(ql: Qiling, snapshot_info: SnapshotInfo) -> None:
    seg_32bit = (snapshot_info.seg.flags >> 0) & 0x1
    contents = (snapshot_info.seg.flags >> 1) & 0x3
    read_exec_only = (snapshot_info.seg.flags >> 3) & 0x1
    limit_in_pages = (snapshot_info.seg.flags >> 4) & 0x1
    seg_not_present = (snapshot_info.seg.flags >> 5) & 0x1
    useable = (snapshot_info.seg.flags >> 6) & 0x1

    if limit_in_pages:
        byte_limit = (snapshot_info.seg.limit << 12) | 0xFFF
    else:
        byte_limit = snapshot_info.seg.limit

    access = (
        QL_X86_A_PRESENT
        | QL_X86_A_PRIV_3
        | QL_X86_A_DESC_DATA
        | QL_X86_A_DATA
        | QL_X86_A_DATA_E
    )

    if not read_exec_only:
        access |= QL_X86_A_DATA_W

    index = ql.os.gdtm.get_free_idx(12)
    if index is None:
        raise RuntimeError(
            "no free GDT descriptor slot available to restore the "
            "snapshot's TLS segment"
        )

    selector = ql.os.gdtm.register_gdt_segment(
        index, snapshot_info.seg.base_addr, byte_limit, access
    )
    ql.arch.regs.gs = selector

    log.debug(
        "restored TLS segment: base=%#x limit=%#x gdt_index=%d selector=%#x",
        snapshot_info.seg.base_addr,
        byte_limit,
        index,
        selector,
    )
