#!/usr/bin/env python3
"""Use only with the local diagnostic firmware. Never flashes or erases.
Requires pyserial (available in the PlatformIO Python environment).
"""
import argparse, time
from pathlib import Path
import serial

p=argparse.ArgumentParser()
p.add_argument('command',choices=['info','snapshot','redraw','redrawfast','redrawlegacy','bench','readertest','navtest'])
p.add_argument('--port',default='/dev/cu.usbserial-0001')
p.add_argument('--out',type=Path,required=True)
a=p.parse_args()
s=serial.Serial(port=None,baudrate=115200,timeout=.2)
s.dtr=False;s.rts=False;s.port=a.port;s.open()
time.sleep(.5)
s.reset_input_buffer()
s.write(('!'+a.command+'\n').encode());s.flush()
lines=[];deadline=time.monotonic()+(120 if a.command=='readertest' else 60 if a.command=='bench' else 15)
bench_count=0
while time.monotonic()<deadline:
    raw=s.readline()
    if not raw:continue
    line=raw.decode('utf-8',errors='replace').rstrip()
    lines.append(line)
    if a.command=='info' and line.startswith('[info]'):break
    if a.command=='snapshot' and line=='[/snapshot]':break
    if a.command=='bench' and line.startswith('[bench]'):
        bench_count+=1
        if line in ['[bench] fixture unavailable','[bench] open failed','[bench] fixture open failed','[bench] fixture write failed']:break
    if a.command=='bench' and line=='[bench] done':break
    if a.command=='navtest' and line.startswith('[nav-test]'):break
    if a.command=='readertest' and line.startswith('[reader-test] done'):break
    if a.command in ['redraw','redrawfast','redrawlegacy'] and line.startswith('[perf]'):break
s.close()
a.out.parent.mkdir(parents=True,exist_ok=True)
if a.command=='snapshot':
    try:
        start=lines.index('[snapshot] 250 122');end=lines.index('[/snapshot]')
        data=bytes.fromhex(''.join(lines[start+1:end]))
        if len(data)!=32*122:raise ValueError('Wrong capture size')
        a.out.write_bytes(b'P4\n250 122\n'+data)
        print('Saved complete 250 x 122 capture:',a.out)
    except (ValueError,IndexError):
        a.out.with_suffix('.log').write_text('\n'.join(lines))
        raise SystemExit('Incomplete capture; log saved, no image claimed')
else:
    a.out.write_text('\n'.join(lines)+'\n')
    print('\n'.join(lines))
    if not lines:raise SystemExit('No device reply; check wake state and diagnostic build')
