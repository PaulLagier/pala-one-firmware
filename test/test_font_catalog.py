#!/usr/bin/env python3
"""Check every translated native glyph against every real selectable face."""
import hashlib
import json
import re
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
if not re.search(r'^\s*olikraus/U8g2_for_Adafruit_GFX\s*@\s*1\.8\.0\s*$',
                 (root/'platformio.ini').read_text(), re.M):
    raise SystemExit('Update the pinned native decoder fixture for this firmware dependency')
decoder = root / 'test/native/font_catalog/decoder' 
for name, expected in json.loads((decoder / 'SHA256.json').read_text()).items():
    if hashlib.sha256((decoder / name).read_bytes()).hexdigest() != expected:
        raise SystemExit(f'Decoder fixture changed: {name}')
lang = root / 'Pala_One_2_1/src/lang'
keys = json.loads((lang / 'interface-keys.json').read_text())
chars = set()
for path in (lang / 'catalogs').glob('*.json'):
    catalog = json.loads(path.read_text())
    chars.update(c for key in keys for c in catalog[key] if ord(c) >= 32)
result = subprocess.run([sys.argv[1]], input='\n'.join(str(ord(c)) for c in sorted(chars)),
                        text=True, capture_output=True)
print(result.stdout, end='')
print(result.stderr, end='')
raise SystemExit(result.returncode)
