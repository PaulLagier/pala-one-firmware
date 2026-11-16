#!/usr/bin/env python3
"""Exercise/render production LibraryScreen and catalog assembly with adapted hardware."""
import argparse,subprocess,tempfile,struct,zlib
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--firmware',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[1];src=a.firmware/'Pala_One_2_1';test=root/'test/native/pala-library';lib=a.firmware/'.pio/libdeps/wireless-paper-v1_2-en/U8g2_for_Adafruit_GFX/src';a.out.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='pala-library-')as folder:
 t=Path(folder);stubs=t/'stubs';stubs.mkdir()
 for name in ['hal/display.h','hal/battery.h','ui/screen.h','storage/library.h','storage/list_items.h','ui/font.h','ui/reader.h','ui/header_title.h','ui/widgets.h',*[f'ui/screens/{s}_screen.h'for s in ['about','apps','update','list','reader','statistics','settings','upload']],'ui/screens/bookmarks/book_select_screen.h','ui/screens/bookmarks/session.h']:
  f=stubs/'src'/name;f.parent.mkdir(parents=True,exist_ok=True);f.write_text('#pragma once\n#include "runtime.h"\n')
 (stubs/'src/ui/screens/library_screen.h').write_bytes((src/'src/ui/screens/library_screen.h').read_bytes())
 binary=t/'check'
 subprocess.run(['clang++','-std=c++17','-DLANG_EN','-fsanitize=address,undefined','-I'+str(stubs),'-I'+str(test),'-I'+str(a.firmware/'scripts/preview_stubs'),'-I'+str(lib),'-I'+str(root/'test/native/pala-reader-window/stubs'),'-I'+str(src),str(test/'main.cpp'),*[str(src/'src'/f)for f in ['pure/library_nav.cpp','pure/paths.cpp','ui/language_fonts.cpp','lang/locale_core.cpp','lang/locale_data.cpp']],str(lib/'U8g2_for_Adafruit_GFX.cpp'),'-lz','-o',str(binary)],check=True)
 for lang in 'de en fr es pl it cs ru uk nl pt'.split():
  out=a.out/lang;out.mkdir(exist_ok=True);subprocess.run([str(binary),str(out),lang],check=True)
  for f in out.glob('*.pbm'):
   bits=f.read_bytes().split(b'\n',2)[2];rows=b''.join(b'\0'+b''.join(bytes([0 if bits[y*32+x//8]&(0x80>>(x%8))else 255])*3 for x in range(250))for y in range(122))
   def chunk(k,d):return struct.pack('>I',len(d))+k+d+struct.pack('>I',zlib.crc32(k+d))
   f.with_suffix('.png').write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',250,122,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b''))
print('PASS Library actual handlers, geometry and fonts in all11 languages: '+str(a.out))
