#!/usr/bin/env python3
"""Emit deterministic gzip assets, content-hashed URLs and size/hash metadata."""
import argparse,hashlib,json,struct,zlib
from pathlib import Path
ITEMS=[('CoreCss','core.css','text/css; charset=utf-8'),('ReadCss','read.css','text/css; charset=utf-8'),('EditorCss','editor.css','text/css; charset=utf-8'),('EditorJs','editor.js','application/javascript; charset=utf-8')]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--assets',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--quiet',action='store_true');a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
def write(path,text):
    if not path.exists() or path.read_text()!=text:path.write_text(text)
def gzip(raw):
    compressor=zlib.compressobj(9,zlib.DEFLATED,-15)
    return b'\x1f\x8b\x08\x00\x00\x00\x00\x00\x02\xff'+compressor.compress(raw)+compressor.flush()+struct.pack('<II',zlib.crc32(raw),len(raw))
header=['#pragma once','#include "src/pure/arduino_compat.h"','#include <stddef.h>','#include <stdint.h>','namespace WebAssets {','enum class Id : uint8_t {'+','.join(row[0]for row in ITEMS)+',Count};','struct Asset {const char* url;const char* mime;const uint8_t* gzip;size_t gzipSize;size_t rawSize;};','const Asset& get(Id id);','void serve(Id id,bool immutable=true);','void registerRoutes();','void appendStyle(String& out,Id id);','void appendScript(String& out,Id id);','}']
cpp=['#include "src/web/web_assets.h"','namespace WebAssets {'];rows=[];report=[]
for name,file,mime in ITEMS:
    raw=(a.assets/file).read_bytes();raw.decode('utf-8');packed=gzip(raw);digest=hashlib.sha256(raw).hexdigest();stem,suffix=file.rsplit('.',1);url=f'/assets/{stem}.{digest[:12]}.{suffix}';symbol='asset'+name
    cpp+=['static const uint8_t '+symbol+'[]={']
    cpp+=['  '+','.join(str(v)for v in packed[i:i+24])+','for i in range(0,len(packed),24)];cpp+=['};']
    rows.append('{'+json.dumps(url)+','+json.dumps(mime)+','+symbol+',sizeof('+symbol+'),'+str(len(raw))+'}')
    report.append(dict(name=name,file=file,url=url,rawBytes=len(raw),gzipBytes=len(packed),rawSHA256=digest,gzipSHA256=hashlib.sha256(packed).hexdigest()))
cpp+=['const Asset& get(Id id){','  static const Asset assets[]={'+','.join(rows)+'};','  size_t index=static_cast<size_t>(id);','  return assets[index<static_cast<size_t>(Id::Count)?index:0];','}','}']
write(a.out/'web_assets.h','\n'.join(header)+'\n');write(a.out/'web_assets_data.cpp','\n'.join(cpp)+'\n');manifest=dict(assets=report,rawBytes=sum(x['rawBytes']for x in report),gzipBytes=sum(x['gzipBytes']for x in report));write(a.out/'web-assets-manifest.json',json.dumps(manifest,indent=2)+'\n')
if not a.quiet:print(json.dumps(manifest,indent=2))
