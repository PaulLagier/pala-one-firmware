"""Verify release archives, provenance, hashes, determinism and layout guards."""
import hashlib,json,struct,subprocess,sys,tempfile,zipfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as directory:
    temp=Path(directory);site=temp/'site';site.mkdir();out=temp/'out'
    app=bytearray(128);app[0]=0xe9;app[12]=9;app[32:39]=b'v3.1.0\0'
    for board in ['v1_1','v1_2']:
        for language in ['en','es','fr']:(site/f'firmware-{board}-{language}.bin').write_bytes(app)
    table=b''.join(struct.pack('<HBBII16sI',0x50aa,t,s,o,n,b'test',0) for t,s,o,n in [(0,0x10,0x10000,0x1d0000),(0,0x11,0x1e0000,0x1d0000),(1,0x82,0x3b0000,0x450000)])
    (site/'partitions.bin').write_bytes(table)
    for name in ['bootloader.bin','boot_app0.bin']:(site/name).write_bytes(b'fixture')
    command=[sys.executable,str(root/'scripts/package_release.py'),'--site',str(site),'--out',str(out),'--version','v3.1.0','--commit','a'*40]
    subprocess.run(command,check=True,capture_output=True)
    index=json.loads((out/'release.json').read_text());assert len(index['artifacts'])==6
    before={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in out.iterdir()}
    for item in index['artifacts']:
        archive=out/item['file'];assert hashlib.sha256(archive.read_bytes()).hexdigest()==item['sha256']
        with zipfile.ZipFile(archive) as z:
            assert len(z.namelist())==6
            sums=next(n for n in z.namelist() if n.endswith('/SHA256SUMS'))
            prefix=sums.rsplit('/',1)[0]+'/'
            for line in z.read(sums).decode().splitlines():
                digest,name=line.split('  ');assert hashlib.sha256(z.read(prefix+name)).hexdigest()==digest
            assert b'0x10000' in z.read(prefix+'README.txt')
    subprocess.run(command,check=True,capture_output=True)
    assert before=={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in out.iterdir()}
    (site/'partitions.bin').write_bytes(b'bad')
    assert subprocess.run(command,capture_output=True).returncode!=0
print('Release archives: all six variants, source provenance, checksums, deterministic output and partition rejection passed')
