#!/usr/bin/env python3
"""Package exact assembled firmware artifacts with instructions and SHA-256 hashes."""
import argparse
import hashlib
import json
import re
import struct
import zipfile
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--site',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
p.add_argument('--version',required=True)
p.add_argument('--commit',required=True)
a=p.parse_args()
if not re.fullmatch(r'v\d+\.\d+(?:\.\d+)?',a.version):p.error('A stable version tag is required')
if not re.fullmatch(r'[0-9a-f]{40}',a.commit):p.error('A full source commit is required')
table=(a.site/'partitions.bin').read_bytes()
records=[struct.unpack('<HBBII16sI',table[n:n+32]) for n in range(0,len(table)-31,32) if table[n:n+2]==b'\xaaP']
for expected in [(0,0x10,0x10000,0x1d0000),(0,0x11,0x1e0000,0x1d0000),(1,0x82,0x3b0000,0x450000)]:
    if not any(record[1:5]==expected for record in records):raise SystemExit('Unexpected partition layout')
a.out.mkdir(parents=True,exist_ok=True)
sha=lambda data:hashlib.sha256(data).hexdigest()
index={'version':a.version,'commit':a.commit,'languages':['de','en','fr','es','it','pl','cs','ru','uk','nl','pt'],'artifacts':[]}
for board in ['v1_1','v1_2']:
    for language in ['en','es','fr']:
        name=f'pala-one-{a.version}-{board}-{language}'
        files={n:(a.site/n).read_bytes() for n in ['bootloader.bin','partitions.bin','boot_app0.bin',f'firmware-{board}-{language}.bin']}
        firmware=files[f'firmware-{board}-{language}.bin']
        if not firmware or len(firmware)>0x1d0000 or firmware[0]!=0xe9 or firmware[12:14]!=b'\x09\x00':raise SystemExit('Invalid ESP32-S3 application image')
        if a.version.encode()+b'\x00' not in firmware:raise SystemExit('Application version does not match release tag')
        files['README.txt']=(f'''Pala One {a.version}\nSource: {a.commit}\nBoard: Heltec Wireless Paper {board.replace('v','V').replace('_','.')}\nInitial language: {language}; all eleven runtime languages are included.\n\nUse the installer at https://paullagier.github.io/pala-one-firmware/stable/\nand select the board revision printed on your PCB. Never choose by enclosure alone.\nClose other serial programs and use a USB data cable. If automatic reset fails,\nhold BOOT, tap RESET, release BOOT and reconnect.\n\nManual installation with esptool:\npython -m esptool --chip esp32s3 --port YOUR_PORT --baud 115200 write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 firmware-{board}-{language}.bin\n\nVerify SHA256SUMS before flashing. This command does not erase the data partition.\nBack up books, screensavers and settings before changing partition layouts or\nrolling back to older firmware. Keep the matching board package for rollback.\nOrdinary V1/V2 bookmarks remain compatible; older firmware cannot read V3\nbookmarks created beyond page 65535. Canonical text offsets remain unchanged.\n\nReading-focused menus can be selected in the Pala app or the reader web settings.\nThe full custom menu order and installed apps are retained. Language is selected\nseparately from board revision. V1.2 has device evidence; V1.1 is build-verified.\n''').encode()
        files['SHA256SUMS']=''.join(f'{sha(data)}  {n}\n' for n,data in sorted(files.items())).encode()
        archive=a.out/(name+'.zip')
        with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
            for n,data in sorted(files.items()):
                info=zipfile.ZipInfo(f'{name}/{n}',(2026,1,1,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16;z.writestr(info,data)
        index['artifacts'].append({'board':board,'initialLanguage':language,'file':archive.name,'bytes':archive.stat().st_size,'sha256':sha(archive.read_bytes()),'firmwareFile':f'firmware-{board}-{language}.bin','firmwareBytes':len(firmware),'firmwareSHA256':sha(firmware)})
(a.out/'release.json').write_text(json.dumps(index,indent=2)+'\n')
(a.out/'SHA256SUMS').write_text(''.join(f'{sha(f.read_bytes())}  {f.name}\n' for f in sorted(a.out.glob('*.zip'))+[a.out/'release.json']))
print(f'Packaged {len(index["artifacts"])} exact board/language images for {a.version}')
