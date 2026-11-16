#!/usr/bin/env python3
"""Compile production font-switch helper with actual upstream glyph decoder."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
lib=root/'.pio/libdeps/wireless-paper-v1_2-local/U8g2_for_Adafruit_GFX/src'
if not lib.exists():
    lib=root/'.pio/libdeps/wireless-paper-v1_2-en/U8g2_for_Adafruit_GFX/src'
out=root/'test/build-local';out.mkdir(exist_ok=True)
subprocess.run(['clang++','-std=c++17','-fsanitize=address,undefined',
 '-I'+str(root/'scripts/preview_stubs'),'-I'+str(lib),'-I'+str(root/'Pala_One_2_1'),
 str(root/'scripts/reader_font_probe.cpp'),str(root/'Pala_One_2_1/src/pure/paginator.cpp'),
 str(root/'Pala_One_2_1/src/pure/text_util.cpp'),str(lib/'U8g2_for_Adafruit_GFX.cpp'),
 '-o',str(out/'reader_font_probe')],check=True)
subprocess.run([str(out/'reader_font_probe')],check=True)
