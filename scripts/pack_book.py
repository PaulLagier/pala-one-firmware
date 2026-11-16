#!/usr/bin/env python3
"""Encode a UTF-8 text file as a seekable PBT1 book without changing its bytes."""
import argparse
from pathlib import Path
import struct
import zlib

BLOCK_SIZE = 4096
MAXIMUM_SIZE = 2 * 1024 * 1024

def pack_text(text: bytes, level: int = 9) -> bytes:
    if not 0 < len(text) <= MAXIMUM_SIZE:
        raise ValueError('Text must contain 1 through 2097152 bytes')
    text.decode('utf-8')
    blocks = []
    for offset in range(0, len(text), BLOCK_SIZE):
        decoded = text[offset:offset + BLOCK_SIZE]
        compressed = zlib.compress(decoded, level)
        raw = len(compressed) >= len(decoded)
        payload = decoded if raw else compressed
        stored = len(payload) | (0x80000000 if raw else 0)
        blocks.append(struct.pack('<II', stored, zlib.crc32(decoded)) + payload)
    cursor = 16 + 4 * (len(blocks) + 1)
    offsets = [cursor]
    for block in blocks:
        cursor += len(block)
        offsets.append(cursor)
    return (struct.pack('<4sIII', b'PBT1', len(text), BLOCK_SIZE, len(blocks))
            + struct.pack('<' + 'I' * len(offsets), *offsets) + b''.join(blocks))

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path, nargs='?')
    parser.add_argument('--level', type=int, choices=range(10), default=9)
    args = parser.parse_args()
    if not 0 < args.source.stat().st_size <= MAXIMUM_SIZE:
        parser.error('Source must contain 1 through 2097152 bytes')
    target = args.destination or args.source.with_suffix('.pbt')
    created = False
    try:
        packed = pack_text(args.source.read_bytes(), args.level)
        with target.open('xb') as stream:
            created = True
            if stream.write(packed) != len(packed):
                raise OSError('Incomplete packed-book write')
    except (OSError, ValueError) as error:
        if created:
            target.unlink(missing_ok=True)
        parser.error(str(error))
    print(f'{args.source.name}: {args.source.stat().st_size} -> {len(packed)} bytes; {target}')

if __name__ == '__main__':
    main()
