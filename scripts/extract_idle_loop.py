#!/usr/bin/env python3
"""Compile the actual sketch loop against deterministic input/screen adapters."""
from pathlib import Path
import re,sys
source=Path(sys.argv[1]).read_text();start=source.index('void loop() {');brace=source.index('{',start);depth=0
pattern=re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]')
for token in pattern.finditer(source,brace):
 if token.group()=='{':depth+=1
 elif token.group()=='}':
  depth-=1
  if depth==0:
   Path(sys.argv[2]).write_text(source[start:token.end()].replace('void loop()', 'void generatedLoop()',1)+'\n')
   break
else:raise SystemExit('Unclosed sketch loop')
