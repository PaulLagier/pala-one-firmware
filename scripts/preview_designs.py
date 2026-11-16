#!/usr/bin/env python3
"""Render the actual U8g2 font tables and shared firmware layout on the host.
Rounded-box rasterization uses the small host GFX adapter; text uses the
unchanged upstream decoder. Firmware screenshot capture is authoritative.
"""
from pathlib import Path
import re, subprocess
root=Path(__file__).resolve().parents[1]
lib=root/'.pio/libdeps/wireless-paper-v1_2-en/U8g2_for_Adafruit_GFX/src'
out=root/'test/build-local';out.mkdir(exist_ok=True)
fonts=(lib/'u8g2_fonts.c').read_text(encoding='latin1')
parts=['#include "u8g2_fonts.h"\n']
for name in ['u8g2_font_helvB12_te','u8g2_font_helvB10_te','u8g2_font_helvR10_te','u8g2_font_helvB18_te']:
    parts.append('extern '+re.search(r'const uint8_t '+name+r'\[.*?";(?=\s*\n)' ,fonts,re.S).group(0))
(out/'preview_fonts.cpp').write_text('\n'.join(parts))
subprocess.run(['clang++','-std=c++17','-I'+str(root/'scripts/preview_stubs'),'-I'+str(lib),'-I'+str(root/'Pala_One_2_1'),str(root/'scripts/preview_designs.cpp'),str(lib/'U8g2_for_Adafruit_GFX.cpp'),str(out/'preview_fonts.cpp'),'-o',str(out/'preview_designs')],check=True)
subprocess.run([str(out/'preview_designs'),str(out)],check=True)
