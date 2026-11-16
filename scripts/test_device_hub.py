#!/usr/bin/env python3
"""Render/test production native settings with actual U8g2 fonts and adapted hardware."""
import argparse, subprocess, tempfile, struct, zlib
from pathlib import Path
def png(path,width,height,pixels):
    def chunk(kind,data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    rows=b''.join(b'\0'+pixels[y*width*3:(y+1)*width*3] for y in range(height))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b''))


p=argparse.ArgumentParser(description=__doc__);p.add_argument('--firmware',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--languages',default='de,en,fr,es,pl,it,cs,ru,uk,nl,pt');a=p.parse_args()
root=Path(__file__).resolve().parents[1];test=root/'test/native/pala-settings';src=a.firmware/'Pala_One_2_1';lib=a.firmware/'.pio/libdeps/wireless-paper-v1_2-en/U8g2_for_Adafruit_GFX/src'
a.out.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='pala-settings-') as folder:
 binary=str(Path(folder)/'settings')
 subprocess.run(['clang++','-std=c++17','-fsanitize=address,undefined','-DDISPLAY_V1_2','-DLANG_EN','-I'+str(a.firmware/'scripts/preview_stubs'),'-I'+str(lib),'-I'+str(test/'stubs'),'-I'+str(src),'-include',str(src/'src/ui/pala_chrome.h'),'-include',str(test/'stubs/src/config.h'),str(test/'main.cpp'),str(lib/'U8g2_for_Adafruit_GFX.cpp'),str(src/'src/ui/language_fonts.cpp'),str(src/'src/lang/locale_core.cpp'),str(src/'src/lang/locale_data.cpp'),'-lz','-o',binary],check=True)
 for lang in a.languages.split(','):
  out=a.out/lang;out.mkdir(exist_ok=True);subprocess.run([binary,str(out),lang],check=True)
  for path in out.glob('*.pbm'):
   bits=path.read_bytes().split(b'\n',2)[2]
   pixels=b''.join(bytes([0 if bits[y*32+x//8]&(0x80>>(x%8)) else 255])*3 for y in range(122) for x in range(250))
   png(path.with_suffix('.png'),250,122,pixels)
print('PASS native settings, all requested languages: '+str(a.out))
