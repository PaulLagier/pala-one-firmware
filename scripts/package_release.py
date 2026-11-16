#!/usr/bin/env python3
"""Package a private Pala One release candidate, never flash or publish it."""
import argparse,hashlib,json,re,shutil,subprocess,zipfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--firmware',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
p.add_argument('--build-log',type=Path,required=True);p.add_argument('--checks',type=Path,required=True)
p.add_argument('--nm',type=Path,required=True)
a=p.parse_args();root=a.firmware.resolve();build=root/'.pio/build/companion-v1_2-release'
def sha(data):return hashlib.sha256(data).hexdigest()
code=[root/'platformio.ini',root/'Pala_One_2_1/Pala_One_2_1.ino']+list((root/'Pala_One_2_1').glob('*.h'))
code += [q for base in ['Pala_One_2_1/src','lib'] for q in (root/base).rglob('*') if q.suffix in ('.h','.cpp')]
digest=hashlib.sha256()
for q in sorted(code):digest.update(str(q.relative_to(root)).encode());digest.update(q.read_bytes())
version='local-rc-'+digest.hexdigest()[:8];binary=(build/'firmware.bin').read_bytes();elf=(build/'firmware.elf').read_bytes()
assert version.encode()+b'\0' in binary,'Release image does not match the current source fingerprint'
assert len(binary)<0x1D0000,'Application exceeds unchanged OTA slot'
log=a.build_log.read_text();assert f'FW_VERSION={version}' in log and re.search(r'companion-v1_2-release\s+SUCCESS',log)
symbols=subprocess.check_output([str(a.nm),'-C',str(build/'firmware.elf')],text=True)
for name in ['Diagnostics::','PackedDiagnostics::','WindowDiagnostics::','RpcDiagnostics::']:
 assert name not in symbols,f'Diagnostic code linked into release: {name}'
for marker in [b'[settings-rows]',b'[reader-menu-test]',b'[locale-failure]',b'[rpc-test]']:
 assert marker not in binary,f'Diagnostic marker in release: {marker!r}'
assert 'companionBleTick()' in symbols,'Bluetooth implementation missing'
partition=(root/'Pala_One_2_1/partitions.csv').read_bytes()
rows=[line.split('#')[0].strip().replace(' ','') for line in partition.decode().splitlines() if line.strip() and not line.lstrip().startswith('#')]
assert rows==['nvs,data,nvs,0x9000,0x5000,','otadata,data,ota,0xE000,0x2000,','ota_0,app,ota_0,0x10000,0x1D0000,','ota_1,app,ota_1,0x1E0000,0x1D0000,','spiffs,data,spiffs,0x3B0000,0x450000,'],'Unexpected partition changes'
checks=json.loads(a.checks.read_text());assert checks['automatedStatus']=='PASS'
a.out.mkdir(parents=True,exist_ok=True)
for name in ['firmware.bin','firmware.elf']:shutil.copyfile(build/name,a.out/name)
files=[]
for folder in ['Pala_One_2_1','lib','scripts','test','.github']:
 for q in (root/folder).rglob('*'):
  rel=q.relative_to(root)
  if not q.is_file() or q.is_symlink():continue
  if any(part.startswith('build') or part in ['.git','.pio','__pycache__','.DS_Store'] for part in rel.parts[:-1]):continue
  if q.name=='.DS_Store':continue
  if '.ino ' in q.name or q.suffix in ['.pyc','.bin','.elf','.o','.a']:continue
  files.append(q)
files += [q for q in root.iterdir() if q.is_file() and (q.name=='platformio.ini' or q.name=='.gitignore' or q.name in ['LOCAL-RELEASE.md','LOCAL-MAJOR-UPDATE.md','LOCAL-LINE-SPACING.md','LOCAL-WAKE-LATENCY.md'] or q.name.startswith(('README','LICENSE')))]
assert (root/'scripts/build_info.py') in files
assert (root/'Pala_One_2_1/src/lang/catalogs/uk.json') in files
archive=a.out/'source.zip';inventory={}
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for q in sorted(set(files)):
  rel=str(q.relative_to(root));data=q.read_bytes();inventory[rel]=sha(data)
  info=zipfile.ZipInfo('pala-one-firmware/'+rel,(2026,1,1,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16;z.writestr(info,data)
 assert z.testzip() is None
(a.out/'source-files.json').write_text(json.dumps(inventory,indent=2)+'\n')
shutil.copyfile(a.build_log,a.out/'build.log');shutil.copyfile(a.checks,a.out/'checks.json')
manifest=dict(status='private release candidate; pending physical acceptance',private=True,published=False,flashed=False,version=version,sourceFingerprintSHA256=digest.hexdigest(),environment='companion-v1_2-release',appOffset='0x10000',appBytes=len(binary),slotBytes=0x1D0000,slotFreeBytes=0x1D0000-len(binary),filesystemBytes=0x450000,partitionSHA256=sha(partition),appSHA256=sha(binary),elfSHA256=sha(elf),sourceZipSHA256=sha(archive.read_bytes()),diagnosticsLinked=False,languages=['de','en','fr','es','pl','it','cs','ru','uk','nl','pt'],checks=checks)
(a.out/'manifest.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n')
(a.out/'LIESMICH.txt').write_text(f'''Pala One – {version}
Privater lokaler Release-Kandidat. Nicht veröffentlicht und nicht geflasht.

Bluetooth, Browser und alle elf Sprachen sind enthalten. USB-Testbefehle und
Bildschirmmitschnitt sind im Release-Image nicht verlinkt.
App: {len(binary):,} Bytes; freier Platz im OTA-Slot: {0x1D0000-len(binary):,} Bytes.
Dual-OTA und 4.521.984 Bytes Dateisystem bleiben unverändert.

firmware.bin ist nur das Anwendungsimage für Heltec V1.2, Offset 0x10000.
Kein Factory-Image. Partitionen, NVS und Bücher nicht löschen.
source.zip enthält Quellcode, Generatoren und Bibliothekslizenzen, keine
Gerätesicherungen, Schlüssel, lokalen Arbeitsprotokolle oder Build-Caches.
Prüfsummen: manifest.json und source-files.json.

Automatische Tests und noch offene Gerätetests stehen in checks.json.
Eine frühere Geräteprüfung ist ausdrücklich kein Hardwaretest dieses Builds.
''')
print(json.dumps({k:manifest[k] for k in ['version','appBytes','slotFreeBytes','diagnosticsLinked','status']},indent=2))
