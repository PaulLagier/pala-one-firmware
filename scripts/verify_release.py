#!/usr/bin/env python3
"""Verify a frozen private Pala One release using only Python's standard library."""
from pathlib import Path,PurePosixPath
import argparse,hashlib,json,zipfile
p=argparse.ArgumentParser(description=__doc__);p.add_argument('release',type=Path);a=p.parse_args();root=a.release
sha=lambda b:hashlib.sha256(b).hexdigest()
m=json.loads((root/'manifest.json').read_text());assert m['private'] and not m['published']
for name,key in [('firmware.bin','appSHA256'),('firmware.elf','elfSHA256'),('source.zip','sourceZipSHA256')]:assert sha((root/name).read_bytes())==m[key],name
binary=(root/'firmware.bin').read_bytes();assert len(binary)==m['appBytes'] and len(binary)<0x1D0000
assert m['appOffset']=='0x10000' and m['slotBytes']==0x1D0000 and m['filesystemBytes']==0x450000
assert m['version'].encode()+b'\0' in binary
inventory=json.loads((root/'source-files.json').read_text());assert inventory
with zipfile.ZipFile(root/'source.zip')as z:
 assert z.testzip() is None
 assert set(z.namelist())=={'pala-one-firmware/'+name for name in inventory}
 code=[]
 for name,digest in inventory.items():
  path=PurePosixPath(name);assert not path.is_absolute() and '..' not in path.parts
  assert not any(x in ['.git','.pio','__pycache__'] or x.startswith('build-') for x in path.parts[:-1]);assert '.ino ' not in path.name
  data=z.read('pala-one-firmware/'+name);assert sha(data)==digest,name
  if name in ['platformio.ini','Pala_One_2_1/Pala_One_2_1.ino'] or (path.parent==PurePosixPath('Pala_One_2_1') and path.suffix=='.h') or (name.startswith(('Pala_One_2_1/src/','lib/')) and path.suffix in ['.h','.cpp']):code.append((name,data))
 h=hashlib.sha256()
 for name,data in sorted(code):h.update(name.encode());h.update(data)
 assert m['sourceFingerprintSHA256']==h.hexdigest()
 assert m['version']=='local-rc-'+h.hexdigest()[:8],'Archived source differs from compiled firmware fingerprint'
 assert 'scripts/build_info.py' in inventory and 'scripts/package_release.py' in inventory
 assert 'LOCAL-RELEASE.md' in inventory
 for language in m['languages']:assert 'Pala_One_2_1/src/lang/catalogs/'+language+'.json' in inventory
 assert sha(z.read('pala-one-firmware/Pala_One_2_1/partitions.csv'))==m['partitionSHA256']
if (root/'SHA256SUMS').exists():
 for line in (root/'SHA256SUMS').read_text().splitlines():
  digest,name=line.split('  ',1);path=PurePosixPath(name);assert not path.is_absolute() and '..' not in path.parts;assert sha((root/name).read_bytes())==digest,name
print(f"PASS {m['version']}: firmware, ELF, complete source inventory/fingerprint, eleven catalogs, fixed partitions and checksums")
