#!/usr/bin/env python3
"""Validate the local diagnostic build in one uninterrupted USB session."""
from pathlib import Path
import argparse, time, re, json, statistics
import serial

p=argparse.ArgumentParser()
p.add_argument('--port',default='/dev/cu.usbserial-0001')
p.add_argument('--out',type=Path,required=True)
p.add_argument('--version',required=True)
a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
connection=serial.Serial(port=None,baudrate=115200,timeout=.2)
connection.dtr=False;connection.rts=False;connection.port=a.port;connection.open()
time.sleep(.5);connection.reset_input_buffer()
all_lines=[]
(a.out/"validation.log").write_text("")
def command(name,finished,timeout=20):
    with (a.out/'validation.log').open('a') as log: log.write('[host] send '+name+'\n')
    connection.write(('!'+name+'\n').encode());connection.flush()
    deadline=time.monotonic()+timeout;lines=[]
    while time.monotonic()<deadline:
        line=connection.readline().decode('utf-8',errors='replace').rstrip()
        if not line:continue
        lines.append(line);all_lines.append(line)
        with (a.out/"validation.log").open("a") as log: log.write(line+"\n")
        if finished(line):return lines
    raise RuntimeError('No complete reply to '+name)
try:
    info=command('info',lambda s:s.startswith('[info]'))[-1]
    assert 'firmware='+a.version+' ' in info,info
    samples=[]
    for trial in range(3):
        lines=command('bench',lambda s:s=='[bench] done',60)
        for line in lines:
            m=re.fullmatch(r'\[bench\] mode=(\w+) cpu=(\d+) pages=(\d+) us=(\d+) offsets_hash=([0-9a-f]+)',line)
            if m:samples.append(dict(trial=trial,mode=m[1],cpu=int(m[2]),pages=int(m[3]),us=int(m[4]),offsets_hash=m[5]))
    assert len(samples)==9,samples
    assert {s['pages'] for s in samples}=={24}
    assert len({s['offsets_hash'] for s in samples})==1
    reader_lines=command('readertest',lambda line:line.startswith('[reader-test] done'),120)
    reader_cases=[]
    for line in reader_lines:
        match=re.fullmatch(r'\[reader-test\] family=(\d+) size=(\d+) bionic=(\d+) pages=(\d+) matched=(\d+)',line)
        if match:reader_cases.append(tuple(map(int,match.groups())))
    assert len(reader_cases)==16,reader_cases
    assert {(r[0],r[1],r[2]) for r in reader_cases}=={(f,s,b) for f in [0,1] for s in [8,10,12,14] for b in [0,1]}
    assert all(r[3]>0 and r[4]==1 for r in reader_cases)
    assert '[reader-test] statistics_preserved=1' in reader_lines
    assert 'pass=1 ' in reader_lines[-1] and 'cleaned=1 ' in reader_lines[-1]
    nav_lines=command('navtest',lambda line:line.startswith('[nav-test] empty_apps_bookmarks'),30)
    assert 'pass=1 statistics_preserved=1' in nav_lines[-1],nav_lines
    redraw=[]
    for trial in range(3):
        lines=command('redraw',lambda s:s.startswith('[perf] redraw_us='))
        redraw.append(int(lines[-1].split('=')[-1]))
    redraw_fast=[]
    for trial in range(3):
        lines=command('redrawfast',lambda s:s.startswith('[perf] redrawfast_us='))
        redraw_fast.append(int(lines[-1].split('=')[-1]))
    redraw_legacy=[]
    for trial in range(3):
        lines=command('redrawlegacy',lambda s:s.startswith('[perf] redrawlegacy_us='))
        redraw_legacy.append(int(lines[-1].split('=')[-1]))
    info_after=command('info',lambda s:s.startswith('[info]'))[-1]
    assert 'cpu=80 ' in info_after,info_after
    lines=command('snapshot',lambda s:s=='[/snapshot]')
    start=lines.index('[snapshot] 250 122')
    image=bytes.fromhex(''.join(lines[start+1:-1]));assert len(image)==32*122
    (a.out/'display.pbm').write_bytes(b'P4\n250 122\n'+image)
    # Repeated device discovery must not lose every second RPC.
    for request in range(10):
        packet=b'IMPROV'+bytes([1,3,2,3,0])
        connection.reset_input_buffer()
        connection.write(packet+bytes([sum(packet)&255]));connection.flush()
        prefix=connection.read_until(b'IMPROV')
        assert prefix.endswith(b'IMPROV'),'No Improv reply: '+str(request+1)
        def exact(n):
            data=b'';deadline=time.monotonic()+5
            while len(data)<n and time.monotonic()<deadline:data+=connection.read(n-len(data))
            assert len(data)==n,'Truncated Improv reply'
            return data
        header=exact(3);body=exact(header[2]+1)
        frame=b'IMPROV'+header+body
        assert header[0]==1 and header[1]==4
        assert sum(frame[:-1])&255==frame[-1]
        assert a.version.encode() in body
    final_info=command('info',lambda line:line.startswith('[info]'))[-1]
    result={'version':a.version,'info_before':info,'info_after':info_after,
            'page_layout_samples':samples,'menu_redraw_us':redraw,
            'menu_redraw_median_us':statistics.median(redraw),
            'menu_redraw_fast_us':redraw_fast,
            'menu_redraw_fast_median_us':statistics.median(redraw_fast),
            'menu_redraw_legacy_us':redraw_legacy,
            'menu_redraw_legacy_median_us':statistics.median(redraw_legacy),
            'reader_render_cases':reader_cases,'reader_cache_and_eof':'passed','reader_statistics_preserved':True,
            'empty_menu_navigation':'passed','improv_consecutive_replies':10,'final_info':final_info,
            'page_offsets_identical':True,'improv_discovery_and_diagnostic_switch':True,'status':'passed'}
    (a.out/'validation.json').write_text(json.dumps(result,indent=2))
    print(json.dumps(result,indent=2))
finally:
    connection.close()
    (a.out/'validation.log').write_text('\n'.join(all_lines)+'\n')
