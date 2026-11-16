#!/usr/bin/env python3
"""Exercise the ordinary sleep path and wait for a physical wake press.
Keeps one serial connection open to avoid turning a port-open reset into
a false wake success. Does not modify sleep/lock/screensaver preferences.
"""
from pathlib import Path
import argparse, json, re, time
import serial

p=argparse.ArgumentParser();p.add_argument('--port',default='/dev/cu.usbserial-0001')
p.add_argument('--version',required=True);p.add_argument('--out',type=Path,required=True)
p.add_argument('--wake-timeout',type=int,default=240)
a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
s=serial.Serial(port=None,baudrate=115200,timeout=.2);s.dtr=False;s.rts=False
s.port=a.port;s.open();time.sleep(.5);s.reset_input_buffer();lines=[]
def receive(until,seconds):
    end=time.monotonic()+seconds
    while time.monotonic()<end:
        line=s.readline().decode('utf-8',errors='replace').rstrip()
        if not line:continue
        lines.append(line)
        with (a.out/'sleep.log').open('a') as f:f.write(line+'\n')
        if until(line):return line
    raise RuntimeError('Timed out waiting for sleep/wake evidence')
def fields(line):return dict(re.findall(r'(\w+)=([^ ]+)',line))
try:
    (a.out/'sleep.log').write_text('')
    s.write(b'!info\n');s.flush()
    before=receive(lambda line:line.startswith('[info]'),20)
    assert fields(before)['firmware']==a.version
    assert fields(before)['book']=='0','Close the current book before the basic sleep test'
    s.write(b'!sleep\n');s.flush()
    sleep_line=receive(lambda line:line.startswith('[sleep]'),20)
    assert 'BTN=1' in sleep_line,'Button was held at sleep entry'
    asleep_at=time.monotonic()
    (a.out/'stage.json').write_text(json.dumps({'stage':'sleeping','version':a.version}))
    print('PALA_ASLEEP_WAITING_FOR_PHYSICAL_WAKE',flush=True)
    wake=receive(lambda line:line.startswith('[boot] wake cause:'),a.wake_timeout)
    wake_cause=int(wake.rsplit(':',1)[1]);assert wake_cause==2,wake
    slept_seconds=time.monotonic()-asleep_at
    assert slept_seconds>1,'Immediate wake was not a useful sleep test'
    s.write(b'!info\n');s.flush()
    after=receive(lambda line:line.startswith('[info]'),20)
    after_fields=fields(after);before_fields=fields(before)
    assert after_fields['firmware']==a.version and after_fields['cpu']=='80'
    expected_lock=before_fields.get('locked')=='1' or before_fields.get('lock_on_sleep')=='1'
    assert (after_fields.get('locked')=='1')==expected_lock
    result={'version':a.version,'status':'passed','wake_cause':wake_cause,
            'slept_seconds':slept_seconds,'info_before':before,'info_after':after,
            'preferences_unchanged':all(before_fields.get(k)==after_fields.get(k)
              for k in ['lock_on_sleep','sleep_s','no_screensaver'])}
    assert result['preferences_unchanged']
    (a.out/'sleep-validation.json').write_text(json.dumps(result,indent=2))
    (a.out/'stage.json').write_text(json.dumps({'stage':'passed','version':a.version}))
    print(json.dumps(result,indent=2),flush=True)
finally:
    s.close()
