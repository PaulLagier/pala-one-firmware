"""Independent standard-library container checks plus encoder CLI regression tests."""
import importlib.util
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

ENCODER = Path(__file__).resolve().parents[1] / 'scripts' / 'pack_book.py'
spec = importlib.util.spec_from_file_location('pack_book', ENCODER)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def unpack(data):
    magic, size, block_size, count = struct.unpack_from('<4sIII', data)
    assert magic == b'PBT1' and block_size == 4096
    offsets = struct.unpack_from('<' + 'I' * (count + 1), data, 16)
    assert offsets[0] == 16 + 4 * (count + 1) and offsets[-1] == len(data)
    decoded = bytearray()
    for start, end in zip(offsets, offsets[1:]):
        stored, crc = struct.unpack_from('<II', data, start)
        payload = data[start + 8:end]
        assert len(payload) == stored & 0x7fffffff
        if stored & 0x80000000:
            block = payload
        else:
            decoder = zlib.decompressobj()
            block = decoder.decompress(payload, 4097)
            assert decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail
        assert len(block) == min(4096, size - len(decoded))
        assert zlib.crc32(block) == crc
        decoded.extend(block)
    assert len(decoded) == size
    return bytes(decoded)


class EncoderTests(unittest.TestCase):
    def test_boundaries_and_unicode(self):
        for size in [1, 4095, 4096, 4097, 8192, module.MAXIMUM_SIZE]:
            for level in [0, 9]:
                text = b'a' * size
                self.assertEqual(unpack(module.pack_text(text, level)), text)
        text = (('a' * 4095) + ' café Grüße\r\n\n  text\t').encode()
        self.assertEqual(unpack(module.pack_text(text)), text)

    def test_rejects_empty_oversized_invalid_utf8(self):
        for text in [b'', b'x' * (module.MAXIMUM_SIZE + 1), b'\xff']:
            with self.assertRaises((ValueError, UnicodeError)):
                module.pack_text(text)

    def test_cli_preserves_existing_destination_and_source(self):
        with tempfile.TemporaryDirectory() as folder:
            source = Path(folder) / 'text.txt'
            target = source.with_suffix('.pbt')
            original = 'Grüße\r\n\n  words\t'.encode()
            source.write_bytes(original)
            run = subprocess.run([sys.executable, '-B', str(ENCODER), str(source)], capture_output=True)
            self.assertEqual(run.returncode, 0, run.stderr)
            result = target.read_bytes()
            self.assertEqual(unpack(result), original)
            run = subprocess.run([sys.executable, '-B', str(ENCODER), str(source)], capture_output=True)
            self.assertNotEqual(run.returncode, 0)
            self.assertEqual(target.read_bytes(), result)
            self.assertEqual(source.read_bytes(), original)
            run = subprocess.run([sys.executable, '-B', str(ENCODER), str(source), str(source)], capture_output=True)
            self.assertNotEqual(run.returncode, 0)
            self.assertEqual(source.read_bytes(), original)


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--fixtures':
        folder = Path(sys.argv[2]); folder.mkdir(parents=True, exist_ok=True)
        text = (('a' * 4095) + 'é café Grüße\r\n\n  text\t' * 400).encode()
        (folder / 'book.txt').write_bytes(text)
        (folder / 'book.pbt').write_bytes(module.pack_text(text))
        (folder / 'raw.pbt').write_bytes(module.pack_text(text, 0))
    else:
        unittest.main()
