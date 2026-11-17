#!/usr/bin/env python3
"""Decode actual generated literals and syntax-check every localized script."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

probe, node = sys.argv[1:]
cases = ['', "Quotes: '\" \\ \n\r\t\x01", '</script><script>alert(1)</script>',
         '&<>', '\u2028\u2029', 'Größe, čtení, français, Русский, Українська 📚']
root = Path(__file__).resolve().parents[1]
for path in (root / 'Pala_One_2_1/src/lang/catalogs').glob('*.json'):
    values = json.loads(path.read_text())
    cases.extend(v for k,v in values.items() if '_JS_' in k)
    script = subprocess.check_output([probe, path.stem], text=True)
    # A translated closing tag must remain escaped, not create another script.
    assert script.count('<script>') == script.count('</script>') == 4, path
    with tempfile.TemporaryDirectory() as folder:
        p = Path(folder)/'scripts.js'
        p.write_text(script.replace('<script>', '').replace('</script>', ''))
        subprocess.run([node, '--check', str(p)], check=True, capture_output=True)
for value in cases:
    literal = subprocess.check_output([probe], input=value, text=True)
    assert json.loads(literal) == value, repr(value)
    assert not any(c in literal for c in '<>&\u2028\u2029'), repr(value)
print(f'{len(cases)} literal round trips and all eleven reader/editor script pairs passed')
