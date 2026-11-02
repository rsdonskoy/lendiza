#!/usr/bin/env python3
"""Extract the .text section of a PE image into a raw binary corpus.

Usage: extract_text.py <input.dll> <output.bin>

The bytes are taken from the file as-is (raw section data, no relocation
fixups); a length decoder only cares about the byte stream, and this matches
how the xendiza bench fixtures were produced.
"""

import struct
import sys


def extract_text(path: str) -> bytes:
    with open(path, "rb") as f:
        data = f.read()

    if data[:2] != b"MZ":
        raise SystemExit(f"{path}: not a PE file (missing MZ)")
    (e_lfanew,) = struct.unpack_from("<I", data, 0x3C)
    if data[e_lfanew : e_lfanew + 4] != b"PE\0\0":
        raise SystemExit(f"{path}: not a PE file (missing PE\\0\\0)")

    file_hdr = e_lfanew + 4
    (num_sections, _, _, _, _, size_opt) = struct.unpack_from("<HHIIIH", data, file_hdr)
    opt_hdr = file_hdr + 20

    # Optional header magic: 0x10B PE32, 0x20B PE32+
    (magic,) = struct.unpack_from("<H", data, opt_hdr)
    if magic == 0x20B:
        sect_align_off = opt_hdr + 32
    elif magic == 0x10B:
        sect_align_off = opt_hdr + 32
    else:
        raise SystemExit(f"{path}: unknown optional header magic 0x{magic:04X}")

    section = opt_hdr + size_opt
    for _ in range(num_sections):
        name = data[section : section + 8].split(b"\0", 1)[0]
        (vsize, vaddr, raw_size, raw_ptr) = struct.unpack_from("<IIII", data, section + 8)
        if name == b".text":
            return data[raw_ptr : raw_ptr + raw_size]
        section += 40

    raise SystemExit(f"{path}: no .text section")


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    blob = extract_text(sys.argv[1])
    with open(sys.argv[2], "wb") as f:
        f.write(blob)
    print(f"{sys.argv[2]}: {len(blob)} bytes")


if __name__ == "__main__":
    main()
