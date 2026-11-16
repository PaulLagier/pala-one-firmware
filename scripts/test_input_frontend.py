#!/usr/bin/env python3
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
out=root/'test/build-local';out.mkdir(exist_ok=True)
subprocess.run(['clang++','-std=c++17','-DARDUINO','-DLANG_EN',
  '-fsanitize=address,undefined','-g','-I'+str(root/'scripts/input_stubs'),
  '-I'+str(root/'Pala_One_2_1'),str(root/'Pala_One_2_1/src/hal/input.cpp'),
  str(root/'Pala_One_2_1/src/pure/hold_gesture.cpp'),
  str(root/'test/input_frontend_probe.cpp'),'-o',str(out/'input_frontend_probe')],check=True)
subprocess.run([str(out/'input_frontend_probe')],check=True)
