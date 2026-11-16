#!/usr/bin/env python3
"""Compile the actual sketch boot and restore branches in native wake tests."""
from pathlib import Path
import re
import sys

source = Path(sys.argv[1]).read_text()
start = source.index('void setup() {')
mount = source.index('  if (!fsBegin())', start)
restore = source.index('  if (tryRestoreReadingSession())', mount)
end = source.index('  // Drop to 80 MHz', restore)
# The mount block is small, but skip complete comments/literals when counting braces.
brace = source.index('{', mount)
depth = 0
pattern = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]')
for token in pattern.finditer(source, brace):
    if token.group() == '{': depth += 1
    elif token.group() == '}':
        depth -= 1
        if depth == 0:
            guard_end = token.end()
            break
else:
    raise SystemExit('Unclosed storage guard in sketch')
prefix = source[start:mount].replace('void setup()', 'void generatedSetup()', 1)
Path(sys.argv[2]).write_text(prefix + source[mount:guard_end] + '\n' + source[restore:end] + '\n}\n')
