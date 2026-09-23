_DESC_ACCESSED = 0x0001
_DESC_DATA_WRITABLE = 0x0002
_DESC_CODE_READABLE = 0x0002
_DESC_DATA_EXPAND_DOWN = 0x0004
_DESC_CODE_CONFORMING = 0x0004
_DESC_CODE_EXECUTABLE = 0x0008

_DESC_S = 0x0010
_DESC_PRESENT = 0x0080

_DESC_LONG_CODE = 0x2000
_DESC_DB = 0x4000
_DESC_GRANULARITY_4K = 0x8000

_DESC_DATA = _DESC_S | _DESC_PRESENT | _DESC_ACCESSED | _DESC_DATA_WRITABLE
_DESC_CODE = (
    _DESC_S
    | _DESC_PRESENT
    | _DESC_ACCESSED
    | _DESC_CODE_READABLE
    | _DESC_CODE_EXECUTABLE
)

DESC_DATA32 = _DESC_DATA | _DESC_GRANULARITY_4K | _DESC_DB
DESC_CODE32 = _DESC_CODE | _DESC_GRANULARITY_4K | _DESC_DB

DESC_DATA64 = _DESC_DATA | _DESC_GRANULARITY_4K | _DESC_DB
DESC_CODE64 = _DESC_CODE | _DESC_GRANULARITY_4K | _DESC_LONG_CODE

DESC_USER = 0x0060


def make_gdt_entry_init(flags: int, base: int, limit: int) -> bytes:
    descriptor = (
        ((limit & 0xFFFF) << 0)
        | (((base >> 0) & 0xFFFF) << 16)
        | (((base >> 16) & 0xFF) << 32)
        | (((flags >> 0) & 0x0F) << 40)
        | (((flags >> 4) & 0x01) << 44)
        | (((flags >> 5) & 0x03) << 45)
        | (((flags >> 7) & 0x01) << 47)
        | (((limit >> 16) & 0x0F) << 48)
        | (((flags >> 12) & 0x01) << 52)
        | (((flags >> 13) & 0x01) << 53)
        | (((flags >> 14) & 0x01) << 54)
        | (((flags >> 15) & 0x01) << 55)
        | (((base >> 24) & 0xFF) << 56)
    )

    return descriptor.to_bytes(8, byteorder="little")
