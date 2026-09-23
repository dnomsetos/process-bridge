from qiling import Qiling
from qiling.const import QL_ARCH
from unicorn import UcError

from .log import get_logger

log = get_logger(__name__)


def clean_vas(ql: Qiling, ql_arch: QL_ARCH) -> int:
    gdt_base = ql.os.gdtm.array.base

    unmapped = 0

    for lbound, ubound, _, label, *_ in list(ql.mem.map_info):
        if lbound == gdt_base:
            log.debug("keeping GDT mapping at %#x, not unmapping", lbound)
            ql.mem.write(lbound, b"\x00" * (ubound - lbound))
            continue

        try:
            ql.mem.unmap(lbound, ubound - lbound)
            unmapped += 1
        except UcError:
            log.exception(
                "failed to unmap qiling's default mapping [%#x-%#x] (%s) "
                "before loading the snapshot",
                lbound,
                ubound,
                label,
            )
            raise

    return unmapped
