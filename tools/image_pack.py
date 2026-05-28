#!/usr/bin/env python3
"""
image_pack.py — Host-side tool to wrap a raw .bin firmware image in
CycloneBOOT image format for OTA update.

Output format:
  [ImageHeader (64 bytes)] [payload (N bytes)] [CRC32 check data (4 bytes)]

The CRC32 covers the payload only (init=0xFFFFFFFF, poly=0xEDB88320,
NO final XOR — stored as raw accumulator value).

Usage:
  python image_pack.py --input firmware.bin --output firmware.img \\
                       --version 1.2.3 --index 2

SPDX-License-Identifier: GPL-2.0-or-later
"""

import argparse
import struct
import time
import sys

# CycloneBOOT ImageHeader version (1.3.0) — matches v2.6.2 image.h
IMAGE_HEADER_VERSION = (1 << 16) | (3 << 8) | 0

# Image types
IMAGE_TYPE_APP = 0


def crc32_cyclone(data: bytes) -> int:
    """
    CRC32 matching CycloneBOOT's variant.
    Polynomial 0xEDB88320 (reflected), init 0xFFFFFFFF.
    Returns the raw accumulator (NOT XOR'd with 0xFFFFFFFF).
    """
    crc = 0xFFFFFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xEDB88320
            else:
                crc >>= 1
    return crc  # No final XOR — CycloneBOOT stores raw value


def parse_version(version_str: str) -> tuple:
    """Parse 'major.minor.revision' or 'major.minor.revision.build'."""
    parts = version_str.split(".")
    if len(parts) < 3:
        raise ValueError(f"Version must be at least major.minor.revision: {version_str}")
    major = int(parts[0])
    minor = int(parts[1])
    revision = int(parts[2])
    build = int(parts[3]) if len(parts) > 3 else 0
    return major, minor, revision, build


def build_image_header(
    payload_size: int,
    img_index: int,
    version_tuple: tuple,
    timestamp: int,
) -> bytes:
    """
    Build a 64-byte CycloneBOOT v2.6.2 ImageHeader.

    Layout (all little-endian):
      offset 0:  uint32  headVers      — header version (0x00010300)
      offset 4:  uint32  imgIndex      — image index number
      offset 8:  uint8   imgType       — IMAGE_TYPE_APP (0)
      offset 9:  uint32  dataPadding   — 0 (no padding)
      offset 13: uint32  dataSize      — total payload size (header excluded)
      offset 17: uint32  binarySize    — firmware binary size (same as dataSize)
      offset 21: ImageVersion (8 bytes) — major(1) minor(1) revision(2) buildNum(4)
      offset 29: uint64  imgTime       — Unix timestamp
      offset 37: 23 bytes reserved     — all zeros
      offset 60: uint32  headCrc       — CRC32 of bytes 0-59
    """
    major, minor, revision, build = version_tuple

    # Pack header fields (bytes 0-59)
    header_body = struct.pack(
        "<"       # little-endian
        "I"       # headVers       (4)
        "I"       # imgIndex       (4)
        "B"       # imgType        (1)
        "I"       # dataPadding    (4)
        "I"       # dataSize       (4)
        "I"       # binarySize     (4)
        "B"       # version.major  (1)
        "B"       # version.minor  (1)
        "H"       # version.rev    (2)
        "I"       # version.build  (4)
        "Q"       # imgTime        (8)
        "23s",    # reserved       (23)
        IMAGE_HEADER_VERSION,
        img_index,
        IMAGE_TYPE_APP,
        0,  # dataPadding
        payload_size,
        payload_size,  # binarySize = dataSize
        major,
        minor,
        revision,
        build,
        timestamp,
        b"\x00" * 23,
    )

    assert len(header_body) == 60, f"Header body is {len(header_body)} bytes, expected 60"

    # CRC32 of header body
    head_crc = crc32_cyclone(header_body)
    header = header_body + struct.pack("<I", head_crc)

    assert len(header) == 64, f"Header is {len(header)} bytes, expected 64"
    return header


def main():
    parser = argparse.ArgumentParser(
        description="Wrap a raw .bin firmware in CycloneBOOT image format"
    )
    parser.add_argument("--input", "-i", required=True, help="Input .bin file")
    parser.add_argument("--output", "-o", required=True, help="Output .img file")
    parser.add_argument(
        "--version", "-v", default="0.0.0",
        help="Firmware version (major.minor.revision[.build])"
    )
    parser.add_argument(
        "--index", "-n", type=int, default=1,
        help="Image index number (bootloader uses this to compare slot freshness)"
    )
    args = parser.parse_args()

    # Read input binary
    try:
        with open(args.input, "rb") as f:
            payload = f.read()
    except FileNotFoundError:
        print(f"Error: input file not found: {args.input}", file=sys.stderr)
        sys.exit(1)

    if len(payload) == 0:
        print("Error: input file is empty", file=sys.stderr)
        sys.exit(1)

    # Parse version
    try:
        version_tuple = parse_version(args.version)
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

    # Build header
    timestamp = int(time.time())
    header = build_image_header(
        payload_size=len(payload),
        img_index=args.index,
        version_tuple=version_tuple,
        timestamp=timestamp,
    )

    # Compute CRC32 check data for payload
    payload_crc = crc32_cyclone(payload)
    check_data = struct.pack("<I", payload_crc)

    # Write output
    with open(args.output, "wb") as f:
        f.write(header)
        f.write(payload)
        f.write(check_data)

    total_size = len(header) + len(payload) + len(check_data)
    print(f"Packed {len(payload)} bytes payload into {total_size} bytes image")
    print(f"  Header:    64 bytes (CRC32=0x{struct.unpack('<I', header[60:64])[0]:08X})")
    print(f"  Payload:   {len(payload)} bytes (CRC32=0x{payload_crc:08X})")
    print(f"  Check:     4 bytes")
    print(f"  Version:   {args.version}")
    print(f"  Index:     {args.index}")
    print(f"  Timestamp: {timestamp}")
    print(f"  Output:    {args.output}")


if __name__ == "__main__":
    main()
