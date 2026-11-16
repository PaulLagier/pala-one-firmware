#!/usr/bin/env python3
"""Run the CMake-listed host tests with Clang sanitizers, without CMake."""
import pathlib, re, subprocess
root=pathlib.Path(__file__).resolve().parents[1]
src=root/'Pala_One_2_1/src'
cm=(root/'test/CMakeLists.txt').read_text()
files=[]
for block in ['SUT_SRC','TEST_SRC']:
    body=re.search(r'set\('+block+r'\s+(.*?)\n\)',cm,re.S).group(1)
    for line in body.splitlines():
        line=line.strip()
        if line:
            files.append(line.replace('${FW_SRC}',str(src)).replace('${CMAKE_CURRENT_SOURCE_DIR}',str(root/'test')))
out=root/'test/build-local'
out.mkdir(exist_ok=True)
subprocess.run(['clang++','-std=c++17','-DLANG_EN','-Wall','-Wextra','-Wpedantic','-Wno-unused-function','-fsanitize=address,undefined','-g','-I'+str(src),'-I'+str(root/'Pala_One_2_1'),'-I'+str(root/'test'),*files,'-o',str(out/'host_tests')],check=True)
subprocess.run([str(out/'host_tests')],check=True)

subprocess.run(["python3", str(root/"scripts/test_input_frontend.py")], check=True)

subprocess.run(["python3", str(root/"scripts/test_improv_parser.py")], check=True)

subprocess.run(["python3", str(root/"scripts/test_font_render.py")], check=True)
