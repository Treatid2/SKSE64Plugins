"""Report Skyrim VR code references to selected VRUI setting strings."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

import capstone
import pefile


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("settings", nargs="+")
    args = parser.parse_args()

    pe = pefile.PE(str(args.image), fast_load=True)
    image = args.image.read_bytes()
    image_base = pe.OPTIONAL_HEADER.ImageBase
    text = next(section for section in pe.sections if section.Name.rstrip(b"\0") == b".text")
    text_bytes = text.get_data()
    text_va = image_base + text.VirtualAddress

    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    decoder.detail = True
    instructions = list(decoder.disasm(text_bytes, text_va))

    for setting in args.settings:
        needle = setting.encode("ascii") + b"\0"
        file_offset = image.find(needle)
        if file_offset < 0:
            print(f"{setting}: string not found")
            continue
        string_rva = pe.get_rva_from_offset(file_offset)
        string_va = image_base + string_rva
        print(f"{setting}: RVA 0x{string_rva:X}")
        candidate_vas = {string_va}
        pointer_needle = struct.pack("<Q", string_va)
        pointer_offset = image.find(pointer_needle)
        while pointer_offset >= 0:
            pointer_rva = pe.get_rva_from_offset(pointer_offset)
            candidate_vas.add(image_base + pointer_rva)
            nearby = image[pointer_offset:pointer_offset + 24]
            words = struct.unpack("<QIIII", nearby)
            float_words = [struct.unpack("<f", struct.pack("<I", word))[0] for word in words[1:]]
            print(
                f"  pointer RVA 0x{pointer_rva:X}; trailing u32={words[1:]}; "
                f"f32={float_words}"
            )
            pointer_offset = image.find(pointer_needle, pointer_offset + 1)

        matches: list[int] = []
        for index, insn in enumerate(instructions):
            for operand in insn.operands:
                if operand.type != capstone.x86.X86_OP_MEM:
                    continue
                memory = operand.mem
                if memory.base != capstone.x86.X86_REG_RIP:
                    continue
                target = insn.address + insn.size + memory.disp
                if target in candidate_vas:
                    matches.append(index)
        if not matches:
            print("  no direct .text RIP reference")
            continue
        for index in matches:
            start = max(0, index - 12)
            end = min(len(instructions), index + 22)
            print(f"  reference RVA 0x{instructions[index].address - image_base:X}")
            for current in instructions[start:end]:
                marker = ">" if current.address == instructions[index].address else " "
                print(
                    f"  {marker} {current.address - image_base:08X}  "
                    f"{current.mnemonic:<8} {current.op_str}"
                )
    return 0


if __name__ == "__main__":
    sys.exit(main())
