#!/usr/bin/env python3
"""Check generated asset bytes, gzip integrity, hashes and deterministic output."""
import gzip, hashlib, json, re, subprocess, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
web=root/'Pala_One_2_1/src/web'
manifest=json.loads((web/'web-assets-manifest.json').read_text())
source=(web/'web_assets_data.cpp').read_text()
for item in manifest['assets']:
    raw=(web/'assets'/item['file']).read_bytes()
    match=re.search(r'asset'+item['name']+r'\[\]=\{([^}]+)',source)
    packed=bytes(int(n) for n in re.findall(r'\d+',match[1]))
    assert gzip.decompress(packed)==raw
    assert len(raw)==item['rawBytes'] and len(packed)==item['gzipBytes']
    digest=hashlib.sha256(raw).hexdigest()
    assert digest==item['rawSHA256'] and digest[:12] in item['url']
    assert hashlib.sha256(packed).hexdigest()==item['gzipSHA256']
with tempfile.TemporaryDirectory() as temp:
    subprocess.run(['python3',str(root/'scripts/generate_web_assets.py'),'--assets',str(web/'assets'),'--out',temp,'--quiet'],check=True)
    for name in ['web_assets.h','web_assets_data.cpp','web-assets-manifest.json']:
        assert (Path(temp)/name).read_bytes()==(web/name).read_bytes(),name
print('All browser stylesheets decode exactly and regenerate deterministically.')
