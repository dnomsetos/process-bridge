from __future__ import annotations

from types import SimpleNamespace


class FakeRegs(SimpleNamespace):
    pass


class FakeMem:
    def __init__(self) -> None:
        self.map_calls: list[tuple[int, int, int]] = []
        self.write_calls: list[tuple[int, bytes]] = []
        self.map_info: list[tuple[int, int, int, str]] = []

    def map(self, addr: int, size: int, perms: int = 0) -> None:
        self.map_calls.append((addr, size, perms))

    def write(self, addr: int, data: bytes) -> None:
        self.write_calls.append((addr, bytes(data)))

    def unmap(self, addr: int, size: int) -> None:
        pass


class FakeGdtSegment(SimpleNamespace):
    pass


class FakeGdtm:
    def __init__(self, free_idx: int | None = 12) -> None:
        self._free_idx = free_idx
        self.registered: list[tuple[int, int, int, int]] = []
        self.array = SimpleNamespace(base=0xF000)

    def get_free_idx(self, _count: int) -> int | None:
        return self._free_idx

    def register_gdt_segment(
        self, index: int, base_addr: int, limit: int, access: int
    ) -> int:
        self.registered.append((index, base_addr, limit, access))
        return (index << 3) | 0x3

    def __iter__(self):
        return iter(())


class FakeOs(SimpleNamespace):
    pass


class FakeQiling:
    def __init__(self, gdtm: FakeGdtm | None = None) -> None:
        self.arch = SimpleNamespace(regs=FakeRegs())
        self.mem = FakeMem()
        self.os = FakeOs(gdtm=gdtm or FakeGdtm())
