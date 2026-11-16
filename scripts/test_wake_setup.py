#!/usr/bin/env python3
"""Compile actual setup branches against hardware adapters and the real input frontend."""
from pathlib import Path
import subprocess
r=Path(__file__).resolve().parents[1];src=r/'Pala_One_2_1';out=r/'test/build-local/wake';out.mkdir(parents=True,exist_ok=True)
sketch=(src/'Pala_One_2_1.ino').read_text()
def block(needle):
 start=sketch.index(needle);brace=sketch.index('{',start);depth=1;end=brace+1
 while depth:
  if sketch[end]=='{':depth+=1
  if sketch[end]=='}':depth-=1
  end+=1
 return sketch[start:end]
start=sketch.index('void setup() {');mount=sketch.index('  if (!fsBegin())',start)
prefix=sketch[start:mount].replace('void setup()','void generatedSetup()',1)
restore_start='const bool restoredReadingSession=' if 'const bool restoredReadingSession=' in sketch else 'if (tryRestoreReadingSession())'
(out/'wake-setup.inc').write_text(prefix+block('if (!fsBegin())')+'\n'+sketch[sketch.index(restore_start):sketch.index('  // Drop to 80 MHz',sketch.index(restore_start))]+'\n}\n')
for board in ['DISPLAY_V1_2','DISPLAY_V1_1']:
 subprocess.run(['clang++','-std=c++17','-DARDUINO','-DLANG_EN','-D'+board,'-fsanitize=address,undefined','-g','-I'+str(r/'scripts/input_stubs'),'-I'+str(src),'-I'+str(out),str(r/'test/native/wake/setup.cpp'),str(src/'src/hal/input.cpp'),str(src/'src/pure/hold_gesture.cpp'),'-o',str(out/'setup')],check=True)
 subprocess.run([str(out/'setup')],check=True)
