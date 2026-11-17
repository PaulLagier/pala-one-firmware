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
tests=root/'test/native/reader_window'
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
    webRaw=("Unique beginning\n\nCanonical target marker here.\n\n" + "Different later text.\n"*60).encode()
    (data/'web.raw').write_bytes(webRaw)
    packed=zlib.compress(webRaw,6)
    record=struct.pack('<II',len(packed),zlib.crc32(webRaw))+packed
    (data/'web.pbt').write_bytes(b'PBT1'+struct.pack('<III',len(webRaw),4096,1)+struct.pack('<II',24,24+len(record))+record)
    production=['lang/join.cpp','web/bookmarks.cpp','pure/paths.cpp','ui/bookmark_settings.cpp','ui/reader.cpp','ui/text.cpp','ui/font.cpp','ui/language_fonts.cpp','ui/statusbar.cpp','storage/page_cache.cpp','storage/book_metadata.cpp','pure/paginator.cpp','pure/text_util.cpp','pure/hashing.cpp','pure/bookmarks_codec.cpp','pure/bookmark_label.cpp']
    subprocess.run(['clang++','-std=c++17','-O1','-g','-DLANG_EN','-fsanitize=address,undefined','-I'+str(tests/'stubs'),'-I'+str(root/'test/native/font_catalog/decoder'),'-I'+str(source),str(tests/'main.cpp'),str(tests/'legacy_fonts.cpp'),*[str(source/'src'/name) for name in production],str(root/'test/native/font_catalog/decoder/U8g2_for_Adafruit_GFX.cpp'),'-lz','-o',str(temp/'reader')],check=True)
    subprocess.run([str(temp/'reader'),str(data)],check=True)
