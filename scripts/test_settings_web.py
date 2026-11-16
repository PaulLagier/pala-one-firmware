#!/usr/bin/env python3
"""Run the production web settings/chrome routes with a native HTTP adapter."""
import argparse,json,hashlib,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--firmware',type=Path,required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[1];fw=a.firmware.resolve();src=fw/'Pala_One_2_1';tests=root/'test/native/pala-reader-locales';lib=fw/'.pio/libdeps/wireless-paper-v1_2-en/U8g2_for_Adafruit_GFX/src'
with tempfile.TemporaryDirectory(prefix='pala-locale-web-') as folder:
    binary=str(Path(folder)/'web')
    production=['web/settings.cpp','web/chrome.cpp','web/web_assets.cpp','web/web_assets_data.cpp','web/web_locale.cpp','lang/locale_core.cpp','lang/locale_data.cpp','lang/locale_settings.cpp','pure/catalog_inflater.cpp','lang/join.cpp','ui/font.cpp','ui/language_fonts.cpp','ui/header_title.cpp','ui/reader_actions.cpp','storage/wifi_creds.cpp']
    subprocess.run(['clang++','-std=c++17','-O1','-g','-DLANG_EN','-fsanitize=address,undefined','-I'+str(tests/'web-stubs'),'-I'+str(root/'test/native/pala-reader-window/stubs'),'-I'+str(fw/'scripts/preview_stubs'),'-I'+str(lib),'-I'+str(src),str(tests/'web.cpp'),*[str(src/'src'/name)for name in production],str(lib/'U8g2_for_Adafruit_GFX.cpp'),'-lz','-o',binary],check=True)
    subprocess.run([binary,folder],check=True)
    manifest=json.loads((src/'src/web/web-assets-manifest.json').read_text())
    for index,asset in enumerate(manifest['assets']):
        raw=(Path(folder)/f'asset-{index}').read_bytes();assert raw==(src/'src/web/assets'/asset['file']).read_bytes()
        assert hashlib.sha256(raw).hexdigest()==asset['rawSHA256'] and asset['rawSHA256'][:12]in asset['url']
        if asset['file'].endswith('.js'):subprocess.run(['node','--check',str(src/'src/web/assets'/asset['file'])],check=True)
    print('PASS: all5 actual gzip route payloads match source bytes/hash URLs, correct MIME/length/cache/encoding, legacy CSS route and JavaScript syntax')

