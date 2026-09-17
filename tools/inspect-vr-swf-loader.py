"""Bounded static inspection of the Skyrim VR 1.4.15 SWF file adapter.

Requires pefile and capstone. No debugger attachment or executable modification.
"""
import argparse
import hashlib
import re
import struct
import capstone
import pefile

parser = argparse.ArgumentParser()
parser.add_argument('image')
parser.add_argument('--dump')
parser.add_argument('--base', type=lambda x: int(x,16))
parser.add_argument('--functions', nargs='*', type=lambda x: int(x, 16), default=[])
args = parser.parse_args()
pe = pefile.PE(args.image, fast_load=True)
base = pe.OPTIONAL_HEADER.ImageBase
print('image-sha256:', hashlib.sha256(pe.__data__).hexdigest())
read = pe.get_data
if args.dump:
    if not args.base:
        parser.error('--dump requires the verified module --base')
    dump = open(args.dump, 'rb')
    header = dump.read(32)
    if header[:4] != b'MDMP':
        raise ValueError('Not a minidump')
    count, directory = struct.unpack_from('<II', header, 8)
    if count > 1000:
        raise ValueError('Too many streams')
    dump.seek(directory)
    entries = [struct.unpack('<III', dump.read(12)) for _ in range(count)]
    stream = next(rva for kind, size, rva in entries if kind == 9)
    dump.seek(stream)
    ranges, offset = struct.unpack('<QQ', dump.read(16))
    if ranges > 100000:
        raise ValueError('Too many memory ranges')
    spans = []
    for _ in range(ranges):
        address, size = struct.unpack('<QQ', dump.read(16))
        spans.append((address, size, offset))
        offset += size
    base = args.base
    def read(rva, length):
        if not 0 < length <= 32*1024*1024:
            raise ValueError('Unbounded read')
        address = base+rva
        chunks = []
        while length:
            start, size, off = next(span for span in spans if span[0] <= address < span[0]+span[1])
            portion = min(length, start+size-address)
            dump.seek(off+address-start)
            data = dump.read(portion)
            if len(data) != portion:
                raise ValueError('Truncated dump')
            chunks.append(data)
            address += portion
            length -= portion
        return b''.join(chunks)
    print('immutable-dump:', args.dump, 'module-base:', hex(base))
decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
def disasm(rva, length=256):
    print(f'function {rva:x}, first {length} bytes:')
    for ins in decoder.disasm(read(rva, length), base+rva):
        print(f'{ins.address-base:08x} {ins.bytes.hex():24} {ins.mnemonic:8} {ins.op_str}')
        if ins.mnemonic == 'ret':
            break

tables = [(0x18660C0, 19, 'GFile'), (0x1866160, 19, 'GMemoryFile'),
          (0x1866220, 4, 'GFxFileOpenerBase'), (0x1866248, 4, 'BSScaleformFileOpener')]
for rva, count, name in tables:
    print(name, hex(rva))
    for slot in range(count):
        address = struct.unpack('<Q', read(rva + slot*8, 8))[0]
        print(f'  {slot:02d} {address-base:x}')
    text = next(s for s in pe.sections if s.Name.rstrip(b'\0') == b'.text')
    code = read(text.VirtualAddress, text.Misc_VirtualSize)
    # Only RIP-relative LEA candidates referencing this exact qualified vtable.
    for m in re.finditer(rb'[\x48\x4c]\x8d[\x05\x0d\x15\x1d\x25\x2d\x35\x3d]', code):
        offset = m.start()
        if offset+7 <= len(code) and text.VirtualAddress+offset+7+struct.unpack_from('<i',code,offset+3)[0] == rva:
            disasm(text.VirtualAddress+max(0, offset-32), 192)
for rva in args.functions:
    disasm(rva, 640)
