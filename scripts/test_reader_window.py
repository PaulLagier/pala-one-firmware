#!/usr/bin/env python3
"""Exercise the actual firmware reader and caches beyond their RAM window.

Only filesystem, NVS, display bus and platform miniz are host adapters. Page
turning, metadata, layout, font decoder, PBT streams and cache code are real.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--firmware',type=Path,required=True)
a=p.parse_args();root=Path(__file__).resolve().parents[1]
source=a.firmware.resolve()/'Pala_One_2_1'
tests=root/'test/native/pala-reader-window'
library=a.firmware.resolve()/'.pio/libdeps/wireless-paper-v1_2-en/U8g2_for_Adafruit_GFX/src'
with tempfile.TemporaryDirectory(prefix='pala-reader-window-') as directory:
    temp=Path(directory)
    import struct,zlib
    data=temp/'data';data.mkdir()
    pattern="Kapitel Eins\nMüller liest über Bluetooth. Absätze bleiben erhalten.\n\n".encode()
    raw=(pattern*((2*1024*1024)//len(pattern)+1))[:2*1024*1024]
    records=[];offsets=[];headerSize=16+4*((len(raw)+4095)//4096+1)
    offset=headerSize
    for start in range(0,len(raw),4096):
        block=raw[start:start+4096];packed=zlib.compress(block,6);stored=len(packed)>=len(block)
        payload=block if stored else packed;record=struct.pack('<II',len(payload)|(0x80000000 if stored else 0),zlib.crc32(block))+payload
        offsets.append(offset);records.append(record);offset+=len(record)
    offsets.append(offset)
    (data/'5.raw').write_bytes(raw)
    (data/'5.pbt').write_bytes(b'PBT1'+struct.pack('<III',len(raw),4096,len(records))+struct.pack('<'+'I'*len(offsets),*offsets)+b''.join(records))
    tables=(library/'u8g2_fonts.c').read_text(encoding='latin1')
    names=sorted(set(re.findall(r'\bu8g2_font_[a-zA-Z0-9_]+',(source/'src/ui/font.cpp').read_text())))
    fonts=['#include "u8g2_fonts.h"']
    for name in names:
        if name.startswith('u8g2_font_open_dys_'):continue
        fonts.append('extern '+re.search(r'const uint8_t '+name+r'\[.*?";(?=\s*\n)',tables,re.S).group(0))
    (temp/'fonts.cpp').write_text('\n'.join(fonts))
    # Compile the real ARDUINO bookmark/Preferences glue without redefining
    # ARDUINO globally (the host String adapter deliberately is not Arduino).
    metadata=(source/'src/storage/book_metadata.cpp').read_text()
    header=(source/'src/storage/book_metadata.h').read_text()
    declarations=header[header.index('uint8_t loadBookmarksForKey'):header.index('int  savedPageForBookPath')]
    glue=metadata[metadata.index('uint8_t loadBookmarksForKey'):metadata.index('int savedPageForBookPath')]
    (temp/'bookmark-glue.h').write_text('#include "src/storage/book_metadata.h"\n#include "src/storage/preferences_store.h"\n#include "src/state.h"\n'+declarations)
    (temp/'bookmark-glue.cpp').write_text(glue)
    production=['ui/screens/bookmarks/bookmark_list_screen.cpp','ui/reader.cpp','ui/text.cpp','ui/font.cpp','ui/language_fonts.cpp','ui/statusbar.cpp','storage/page_cache.cpp','storage/book_metadata.cpp','pure/paginator.cpp','pure/text_util.cpp','pure/hashing.cpp','pure/bookmarks_codec.cpp','pure/bookmark_label.cpp','lang/locale_core.cpp','lang/locale_data.cpp']
    subprocess.run(['clang++','-std=c++17','-O1','-g','-DLANG_EN','-fsanitize=address,undefined',
                    '-include',str(temp/'bookmark-glue.h'),'-I'+str(tests/'stubs'),'-I'+str(a.firmware/'scripts/preview_stubs'),'-I'+str(library),'-I'+str(source),
                    str(tests/'main.cpp'),str(root/'test/native/performance/paginator_reference.cpp'),str(temp/'bookmark-glue.cpp'),*[str(source/'src'/name) for name in production],
                    str(library/'U8g2_for_Adafruit_GFX.cpp'),str(temp/'fonts.cpp'),'-lz','-o',str(temp/'reader')],check=True)
    subprocess.run([str(temp/'reader'),str(temp/'data')],check=True)
    output=root/'test/build-local/reader-preview';output.mkdir(parents=True,exist_ok=True)
    import shutil
    for pattern in ['preview-*.pbm','bookmarks-*.pbm','spacing-*.pbm']:
        for file in (temp/'data').glob(pattern):shutil.copyfile(file,output/file.name)
