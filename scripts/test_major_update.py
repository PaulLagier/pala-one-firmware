#!/usr/bin/env python3
"""Check production navigation, web settings and page equivalence under sanitizers."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1];src=root/'Pala_One_2_1';native=root/'test/native'
out=root/'test/build-local';out.mkdir(exist_ok=True)
lib=root/'.pio/libdeps/wireless-paper-v1_2-en/U8g2_for_Adafruit_GFX/src'
def run(args):subprocess.run([str(x) for x in args],check=True)
common=['clang++','-std=c++17','-O1','-g','-DLANG_EN','-fsanitize=address,undefined']
upload=native/'upload'
run(common+['-I'+str(native/'pala-reader-locales/web-stubs'),'-I'+str(native/'pala-reader-window/stubs'),'-I'+str(root/'scripts/preview_stubs'),'-I'+str(src),upload/'main.cpp',*[src/'src'/name for name in ['web/upload.cpp','pure/paths.cpp','pure/text_util.cpp','lang/join.cpp','lang/locale_core.cpp','lang/locale_data.cpp']],'-lz','-o',out/'upload-safety'])
run([out/'upload-safety'])
rotation=native/'display'
run(common+['-I'+str(rotation/'stubs'),'-I'+str(src),rotation/'main.cpp',src/'src/hal/display.cpp','-o',out/'display-rotation'])
run([out/'display-rotation'])
menu=native/'pala-reader-menu'
run(common+['-I'+str(menu/'stubs'),'-I'+str(src),menu/'main.cpp',src/'src/ui/reader_menu.cpp',src/'src/ui/screens/reader_screen.cpp',src/'src/pure/paths.cpp',src/'src/lang/locale_core.cpp',src/'src/lang/locale_data.cpp','-o',out/'simple-menu'])
run([out/'simple-menu'])
run(common+['-I'+str(root/'scripts/preview_stubs'),'-I'+str(lib),'-I'+str(src),native/'performance/main.cpp',native/'performance/paginator_reference.cpp',src/'src/pure/paginator.cpp',src/'src/pure/text_util.cpp',src/'src/ui/language_fonts.cpp',lib/'U8g2_for_Adafruit_GFX.cpp','-o',out/'pagination-equivalence'])
run([out/'pagination-equivalence'])
run(common+['-I'+str(root/'scripts/preview_stubs'),'-I'+str(lib),'-I'+str(src),native/'performance/vertical_bounds.cpp',src/'src/ui/language_fonts.cpp',lib/'U8g2_for_Adafruit_GFX.cpp','-o',out/'vertical-bounds'])
run([out/'vertical-bounds'])
for script,folder in [('test_library_native.py','library'),('test_device_hub.py','device'),('test_reader_menu_render.py','reader')]:
 run(['python3',root/'scripts'/script,'--firmware',root,'--out',out/'major-preview'/folder])
run(['python3',root/'scripts/test_settings_web.py','--firmware',root])

run(['python3',root/'scripts/test_wake_setup.py'])
run(['python3',root/'scripts/test_reader_window.py','--firmware',root])
