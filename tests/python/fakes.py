from __future__ import annotations

from types import SimpleNamespace


class FakeRegs(SimpleNamespace):
    pass


from dataclasses import dataclass


@dataclass
class MemoryRegion:
    start: int
    size: int
    perms: int
    data: bytearray

    @property
    def end(self) -> int:
        return self.start + self.size


class FakeMem:
    def __init__(self) -> None:
        self.memory: list[MemoryRegion] = []
        self.pagesize: int = 4096

    def map(self, addr: int, size: int, perms: int = 0) -> None:
        if size <= 0:
            raise ValueError(f"Invalid mapping size: {size}")

        end = addr + size

        for region in self.memory:
            if addr < region.end and region.start < end:
                raise ValueError(
                    f"Memory overlap: "
                    f"{addr:#x}-{end:#x} with "
                    f"{region.start:#x}-{region.end:#x}"
                )

        self.memory.append(
            MemoryRegion(
                start=addr,
                size=size,
                perms=perms,
                data=bytearray(size),
            )
        )

        self.memory.sort(key=lambda region: region.start)

    def unmap(self, addr: int, size: int) -> None:
        if size <= 0:
            raise ValueError(f"Invalid unmapping size: {size}")

        end = addr + size

        for i, region in enumerate(self.memory):
            if region.start == addr and region.end == end:
                del self.memory[i]
                return

        raise ValueError(f"Mapping not found: {addr:#x}-{end:#x}")

    def is_mapped(self, addr: int, size: int = 1) -> bool:
        if size <= 0:
            return False

        end = addr + size

        for region in self.memory:
            if region.start <= addr and end <= region.end:
                return True

            if region.start > addr:
                break

        return False

    def _find_region(self, addr: int, size: int) -> MemoryRegion:
        if size < 0:
            raise ValueError(f"Invalid size: {size}")

        end = addr + size

        for region in self.memory:
            if region.start <= addr and end <= region.end:
                return region

            if region.start > addr:
                break

        raise ValueError(f"Memory not mapped: {addr:#x}-{end:#x}")

    def write(self, addr: int, data: bytes | bytearray) -> None:
        region = self._find_region(addr, len(data))

        offset = addr - region.start
        region.data[offset : offset + len(data)] = data

    def read(self, addr: int, size: int) -> bytes:
        region = self._find_region(addr, size)

        offset = addr - region.start
        return bytes(region.data[offset : offset + size])


class FakeGdtSegment(SimpleNamespace):
    pass


class FakeGdtm:
    def __init__(self, free_idx: int = 12) -> None:
        self._free_idx = free_idx
        self.registered: list[tuple[int, int, int, int]] = []
        self.array = SimpleNamespace(base=0xF000)

    def get_free_idx(self) -> int:
        return self._free_idx

    def register_gdt_segment(
        self,
        index: int,
        base_addr: int,
        limit: int,
        access: int,
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
