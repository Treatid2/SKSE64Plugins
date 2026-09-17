#!/usr/bin/env python3
"""Inspect an Address Library v1/v2 binary without loading game code.

Usage: inspect-address-library.py FILE OFFSET [OFFSET ...]
Offsets may be decimal or hexadecimal.  The command prints every exact
offset-to-ID match, which is useful for deriving documented cross-runtime
relocations from independently published Address Library data.
"""

from __future__ import annotations

import argparse
import pathlib
import struct


def read_value(data: bytes, position: int, fmt: str) -> tuple[int, int]:
    value = struct.unpack_from(fmt, data, position)[0]
    return value, position + struct.calcsize(fmt)


def unpack_mappings(path: pathlib.Path) -> list[tuple[int, int]]:
    data = path.read_bytes()
    position = 0
    file_format, position = read_value(data, position, "<i")
    if file_format not in (1, 2):
        raise ValueError(f"unsupported Address Library format {file_format}")

    position += struct.calcsize("<4i")
    name_length, position = read_value(data, position, "<i")
    position += name_length
    pointer_size, position = read_value(data, position, "<i")
    address_count, position = read_value(data, position, "<i")

    previous_id = 0
    previous_offset = 0
    mappings: list[tuple[int, int]] = []
    for _ in range(address_count):
        entry_type = data[position]
        position += 1
        id_encoding = entry_type & 0xF
        offset_encoding = entry_type >> 4

        if id_encoding == 0:
            identifier, position = read_value(data, position, "<Q")
        elif id_encoding == 1:
            identifier = previous_id + 1
        elif id_encoding == 2:
            identifier = previous_id + data[position]
            position += 1
        elif id_encoding == 3:
            identifier = previous_id - data[position]
            position += 1
        elif id_encoding == 4:
            delta, position = read_value(data, position, "<H")
            identifier = previous_id + delta
        elif id_encoding == 5:
            delta, position = read_value(data, position, "<H")
            identifier = previous_id - delta
        elif id_encoding == 6:
            identifier, position = read_value(data, position, "<H")
        elif id_encoding == 7:
            identifier, position = read_value(data, position, "<I")
        else:
            raise ValueError(f"unsupported ID encoding {id_encoding}")

        scaled = bool(offset_encoding & 8)
        previous_base = previous_offset // pointer_size if scaled else previous_offset
        offset_encoding &= 7
        if offset_encoding == 0:
            offset, position = read_value(data, position, "<Q")
        elif offset_encoding == 1:
            offset = previous_base + 1
        elif offset_encoding == 2:
            offset = previous_base + data[position]
            position += 1
        elif offset_encoding == 3:
            offset = previous_base - data[position]
            position += 1
        elif offset_encoding == 4:
            delta, position = read_value(data, position, "<H")
            offset = previous_base + delta
        elif offset_encoding == 5:
            delta, position = read_value(data, position, "<H")
            offset = previous_base - delta
        elif offset_encoding == 6:
            offset, position = read_value(data, position, "<H")
        elif offset_encoding == 7:
            offset, position = read_value(data, position, "<I")
        else:
            raise AssertionError("unreachable")

        if scaled:
            offset *= pointer_size
        mappings.append((identifier, offset))
        previous_id = identifier
        previous_offset = offset

    return mappings


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("file", type=pathlib.Path)
    parser.add_argument("offset", nargs="+", type=lambda value: int(value, 0))
    args = parser.parse_args()

    by_offset: dict[int, list[int]] = {}
    for identifier, offset in unpack_mappings(args.file):
        by_offset.setdefault(offset, []).append(identifier)

    for offset in args.offset:
        identifiers = ",".join(str(value) for value in by_offset.get(offset, [])) or "-"
        print(f"0x{offset:08X} {identifiers}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
