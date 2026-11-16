#!/usr/bin/env python3
"""Generate compact, runtime-selectable reader dictionaries without fallbacks.

Only complete catalogs are accepted. --require-all is the release gate for the
eleven requested languages. --bootstrap imports the existing EN/ES C literals
once. Existing public UI and web functionality retain their canonical key set.
"""
import argparse,ast,html,json,re,struct,unicodedata,zlib
from pathlib import Path

LANGUAGES={'de':'Deutsch','en':'English','fr':'Français','es':'Español','pl':'Polski','it':'Italiano','cs':'Čeština','ru':'Русский','uk':'Українська','nl':'Nederlands','pt':'Português'}
PRINTF=re.compile(r'%(?!%)(?:[-+ #0]*\d*(?:\.\d+)?(?:hh|ll|[hljztL])?[diuoxXfFeEgGaAcspn])|%%')
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--catalogs',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
p.add_argument('--interface-source',type=Path);p.add_argument('--bootstrap',type=Path);p.add_argument('--require-all',action='store_true');p.add_argument('--quiet',action='store_true');a=p.parse_args()
a.catalogs.mkdir(parents=True,exist_ok=True);a.out.mkdir(parents=True,exist_ok=True)
if a.bootstrap:
    for code,name in [('en','en.h'),('es','es_la.h')]:
        dest=a.catalogs/(code+'.json')
        if dest.exists():raise SystemExit(f'Refusing to overwrite catalog: {dest}')
        catalog={}
        for key,literal in re.findall(r'^#define\s+(D_\w+)\s+("(?:\\.|[^"\\])*")\s*$',(a.bootstrap/'Pala_One_2_1/src/lang'/name).read_text(),re.M):
            value=ast.literal_eval(literal)
            if key=='D_MENU_UPLOAD' and key in catalog:catalog['D_MENU_CONNECT']=value
            else:catalog[key]=value
        catalog['D_LANGUAGE_LABEL']='Reader language' if code=='en' else 'Idioma del lector'
        catalog['D_LANGUAGE_INVALID']='Language unavailable' if code=='en' else 'Idioma no disponible'
        dest.write_text(json.dumps(catalog,ensure_ascii=False,indent=2)+'\n')
catalogs={file.stem:json.loads(file.read_text()) for file in sorted(a.catalogs.glob('*.json'))}
if set(catalogs)-set(LANGUAGES):raise SystemExit('Unknown language catalog')
if not {'en','es'}<=set(catalogs):raise SystemExit('Both flash defaults EN/ES are required')
if a.require_all and set(catalogs)!=set(LANGUAGES):raise SystemExit('Incomplete eleven-language release: '+', '.join(sorted(set(LANGUAGES)-set(catalogs))))
keys=sorted(catalogs['en'])
for code,strings in catalogs.items():
    if set(strings)!=set(keys):raise SystemExit(f'{code}: missing/extra keys: {set(keys)^set(strings)}')
    for key,text in strings.items():
        if not re.fullmatch(r'D_[A-Z0-9_]+',key) or not isinstance(text,str) or not text or '\0' in text:raise SystemExit(f'{code}/{key}: invalid text')
        if text!=unicodedata.normalize('NFC',text):raise SystemExit(f'{code}/{key}: not NFC')
        if sorted(re.findall(r'\{[A-Za-z][A-Za-z0-9_]*\}',text))!=sorted(re.findall(r'\{[A-Za-z][A-Za-z0-9_]*\}',catalogs['en'][key])):raise SystemExit(f'{code}/{key}: named placeholders differ')
        is_format=key.endswith('_FMT') or key in ('D_READER_MENU_PAGE','D_READER_MENU_PAGE_TOTAL')
        is_js='_JS_' in key
        if is_js and sorted(re.findall(r'%[1-9][0-9]*',text))!=sorted(re.findall(r'%[1-9][0-9]*',catalogs['en'][key])):raise SystemExit(f'{code}/{key}: JavaScript placeholders differ')
        if is_format and not is_js and PRINTF.findall(text)!=PRINTF.findall(catalogs['en'][key]):raise SystemExit(f'{code}/{key}: printf placeholders differ')
        if is_format and any('%n' in fmt for fmt in PRINTF.findall(text)):raise SystemExit('Unsafe printf format')
        if key.startswith(('D_WEB_CONFIRM_','D_WEB_SS_CONFIRM_')) and any(c in html.unescape(text) for c in "'\\\"<>"):raise SystemExit(f'{code}/{key}: unsafe inline confirmation')
        if not key.endswith('_PLACEHOLDER') and re.findall(r'</?\w+[^>]*>',text)!=re.findall(r'</?\w+[^>]*>',catalogs['en'][key]):raise SystemExit(f'{code}/{key}: HTML markup differs')


# Small independently compressed pools avoid a contiguous 20KB allocation while
# Bluetooth owns much of the ESP32 heap. Key IDs and string lifetimes stay stable.
KEYS_PER_POOL=32
# Preload every key reachable from the native UI/RPC; validate browser-only
# pools at selection, then cache them on first use. Stable key IDs are retained.
interface_source=a.interface_source or (a.out.parent if (a.out.parent/'ui').is_dir() else None)
interface_file=a.catalogs.parent/'interface-keys.json'
if interface_source:
    used=set()
    candidates=list(interface_source.rglob('*.cpp'))+list(interface_source.rglob('*.h'))+list(interface_source.parent.glob('*.ino'))+list(interface_source.parent.glob('*.h'))
    for source in candidates:
        if 'web' in source.relative_to(interface_source.parent).parts or source.name in ('runtime_strings.h','locale_data.cpp','en.h','es_la.h','fr.h'):continue
        used.update(re.findall(r'\bD_[A-Z0-9_]+\b',source.read_text()))
    interface_keys=sorted(used&set(keys))
elif interface_file.exists():interface_keys=json.loads(interface_file.read_text())
else:interface_keys=keys
if not interface_keys or not set(interface_keys)<=set(keys):raise SystemExit('Invalid interface key list')
def grouped(selected):
    result=[];group=[];worst=0
    for key in selected:
        size=max(len(catalog[key].encode())+3 for catalog in catalogs.values())
        if size>2048:raise SystemExit('Single catalog string exceeds 2KB budget: '+key)
        if group and (len(group)==KEYS_PER_POOL or worst+size>2048):result.append(group);group=[];worst=0
        group.append(key);worst+=size
    if group:result.append(group)
    return result
interface_groups=grouped(interface_keys)
groups=interface_groups+grouped([key for key in keys if key not in set(interface_keys)])
locations={key:i*KEYS_PER_POOL+j for i,group in enumerate(groups)for j,key in enumerate(group)}
def pool(strings,group):
    result=bytearray(2*len(group));offsets={}
    for index,key in enumerate(group):
        text=strings[key]
        if text not in offsets:
            offsets[text]=len(result);result+=text.encode()+b'\0'
        struct.pack_into('<H',result,index*2,offsets[text])
    assert len(result)<65536
    return bytes(result)
def array(name,data,inline=False):
    lines=[('inline constexpr' if inline else 'static const')+f' uint8_t {name}[] = {{']
    lines+=['  '+','.join(f'0x{x:02x}' for x in data[start:start+24])+',' for start in range(0,len(data),24)]
    return '\n'.join(lines+['};'])
def write(path,text):
    if not path.exists() or path.read_text()!=text:path.write_text(text)
order=[code for code in LANGUAGES if code in catalogs]
pools={code:[pool(strings,group)for group in groups]for code,strings in catalogs.items()}
sequential={code:[b''.join(strings[key].encode()+b'\0'for key in group)for group in groups]for code,strings in catalogs.items()}
header=['#pragma once','#include "src/lang/default_language.h"','#include <stddef.h>','#include <stdint.h>','// Generated. Pointers remain valid until the next successful language switch.','// Getters and switches run on the firmware main thread.','namespace Language {','enum class Key : uint16_t {']
header+=['  K_'+key[2:]+','for key in keys]
header+=['  Count','};',f'inline constexpr size_t keyCount={len(keys)};',f'inline constexpr size_t keysPerPool={KEYS_PER_POOL};',f'inline constexpr size_t poolCount={len(groups)};',f'inline constexpr size_t interfacePoolCount={len(interface_groups)};', 'inline constexpr uint8_t poolKeyCounts[]={'+','.join(str(len(group))for group in groups)+'};', 'inline constexpr size_t keysInPool(size_t i){return poolKeyCounts[i];}', 'inline constexpr uint16_t keyLocations[]={'+','.join(str(locations[key])for key in keys)+'};', 'bool ensurePool(size_t index);']
for code,condition in [('es','#if defined(LANG_ES_LA)'),('fr','#elif defined(LANG_FR)'),('en','#else')]:
    header+=[condition,f'inline constexpr const char* defaultCode="{code}";']
    for i,data in enumerate(pools[code]):header.append(array('defaultPool'+str(i),data,True))
header+=['#endif','inline constexpr const uint8_t* defaultPools[]={'+','.join('defaultPool'+str(i)for i in range(len(groups)))+'};','inline const uint8_t* activePools[]={'+','.join('defaultPool'+str(i)for i in range(len(groups)))+'};','inline const uint8_t* activePool=defaultPool0;','inline const char* activeCode=defaultCode;','#if defined(_MSC_VER)','#define PALA_LOCALE_NOINLINE __declspec(noinline)','#else','#define PALA_LOCALE_NOINLINE __attribute__((noinline))','#endif','PALA_LOCALE_NOINLINE inline const char* text(Key key) {','  const size_t index=static_cast<size_t>(key);','  if(index>=keyCount)return "";','  const size_t location=keyLocations[index],poolIndex=location/keysPerPool;','  if(!activePools[poolIndex]&&!ensurePool(poolIndex))return "";','  const auto* pool=activePools[poolIndex];','  const auto* offset=pool+(location%keysPerPool)*2;','  return reinterpret_cast<const char*>(pool+(uint16_t(offset[0])|(uint16_t(offset[1])<<8)));','}','#undef PALA_LOCALE_NOINLINE','inline const char* currentCode(){return activeCode;}']
header+=['inline constexpr const char* codes[]={'+','.join(json.dumps(code)for code in order)+'};','inline constexpr const char* names[]={'+','.join(json.dumps(LANGUAGES[code],ensure_ascii=False)for code in order)+'};',f'inline constexpr size_t count(){{return {len(order)};}}','inline const char* codeAt(size_t i){return i<count()?codes[i]:nullptr;}','inline const char* nameAt(size_t i){return i<count()?names[i]:nullptr;}','}']
for key in keys:
    header.append(f'#define {key} Language::text(Language::Key::K_{key[2:]})')
write(a.out/'runtime_strings.h','\n'.join(header)+'\n')
data=['#include "src/lang/locale_core.h"','namespace Language {']
default_flags={'en':'LANG_EN','es':'LANG_ES_LA','fr':'LANG_FR'}
for code in order:
    flag=default_flags.get(code)
    if flag:data+=['#if !defined('+flag+')']
    for i,raw in enumerate(sequential[code]):data.append(array(f'packed_{code}_{i}',zlib.compress(raw,9)))
    data+=['static const PackedPool pools_'+code+'[]={']
    for i,raw in enumerate(sequential[code]):data.append('  {packed_'+code+'_'+str(i)+',sizeof(packed_'+code+'_'+str(i)+'),'+str(len(raw))+'},')
    data+=['};']
    if flag:data+=['#endif']
data+=['const Catalog* catalogAt(size_t index) {','  static const Catalog catalogs[]={']
for code in order:
    flag=default_flags.get(code)
    if flag:data+=['#if defined('+flag+')','    {"'+code+'",nullptr},','#else']
    data+=['    {"'+code+'",pools_'+code+'},']
    if flag:data+=['#endif']
data+=['  };','  return index<count()?&catalogs[index]:nullptr;','}','}']
write(a.out/'locale_data.cpp','\n'.join(data)+'\n')
report={'requestedLanguages':list(LANGUAGES),'implementedCatalogs':order,'keyCount':len(keys),'complete':len(order)==11,'keysPerPool':KEYS_PER_POOL,'poolsPerCatalog':len(groups),'interfacePoolCount':len(interface_groups),'interfaceKeyCount':len(interface_keys),'largestAllocation':max(len(raw)+2*len(group)for segments in sequential.values()for raw,group in zip(segments,groups)),'catalogs':{code:{'defaultPoolBytes':sum(map(len,pools[code])),'rawStringsBytes':sum(map(len,segments)),'compressedBytes':sum(len(zlib.compress(raw,9))for raw in segments),'preloadedInterfaceBytes':sum(len(raw)+2*len(group)for raw,group in zip(segments[:len(interface_groups)],interface_groups))}for code,segments in sequential.items()}}
write(a.out/'interface-keys.json',json.dumps(interface_keys,indent=2)+'\n')
write(a.out/'locale-manifest.json',json.dumps(report,indent=2)+'\n')
if not a.quiet:print(json.dumps(report,indent=2))
