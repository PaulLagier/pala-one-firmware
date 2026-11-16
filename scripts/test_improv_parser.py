#!/usr/bin/env python3
from pathlib import Path
import argparse, subprocess
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--library',type=Path,default=root/'lib/improv-wifi')
a=p.parse_args();out=root/'test/build-local';out.mkdir(exist_ok=True)
subprocess.run(['clang++','-std=c++17','-DARDUINO','-DARDUINO_ARCH_ESP32',
  '-fsanitize=address,undefined','-g','-Wno-c99-designator',
  '-I'+str(root/'scripts/improv_stubs'),'-I'+str(a.library/'src'),
  str(a.library/'src/ImprovWiFiLibrary.cpp'),str(root/'test/improv_parser_probe.cpp'),
  '-o',str(out/'improv_parser_probe')],check=True)
subprocess.run([str(out/'improv_parser_probe')],check=True)
