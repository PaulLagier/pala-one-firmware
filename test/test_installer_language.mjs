import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
const source = await readFile(new URL('../install/one-language.js', import.meta.url), 'utf8');
const {LANGUAGES, browserLanguage, languagePacket, languageAcknowledged, configureLanguage, readingMenuPacket, readingMenuAcknowledged} =
  await import('data:text/javascript;base64,' + Buffer.from(source).toString('base64'));
assert.equal(browserLanguage(['ja-JP', 'fr-CA', 'de-DE']), 'fr');
assert.equal(browserLanguage(['PT_br']), 'pt');
assert.equal(browserLanguage(['ja-JP']), 'en');
function ack(code) {
  const bytes = [73,77,80,82,79,86,1,4,5,0x81,3,2,...new TextEncoder().encode(code)];
  return new Uint8Array([...bytes, bytes.reduce((n,v)=>(n+v)&255,0)]);
}
for (const code of LANGUAGES) {
  const packet = languagePacket(code);
  assert.equal(packet.length, 14);
  assert.equal(packet[8], 4); assert.equal(packet[9], 0x81);
  assert.equal(packet[10], 2);
  assert.equal(packet.at(-1), packet.slice(0,-1).reduce((n,v)=>(n+v)&255,0));
  const response = ack(code);
  assert(languageAcknowledged(response, code));
  for (let length=0;length<response.length;++length)
    assert(!languageAcknowledged(response.slice(0,length), code));
  for (let i=0;i<response.length;++i) {
    const corrupt = response.slice(); corrupt[i] ^= 1;
    assert(!languageAcknowledged(corrupt, code));
  }
  assert(!languageAcknowledged(response, code==='de'?'fr':'de'));
  assert(languageAcknowledged(new Uint8Array([0,13,10,...response]), code));
}
for (const invalid of ['DE','xx','de\0','de-DE','',undefined])
  assert.throws(()=>languagePacket(invalid));
function portMock(code, reply=true) {
  let controller;
  const state = {writes:[], signals:[], opened:false, closed:false, cancelled:false};
  const port = {
    readable:new ReadableStream({start(c){controller=c},cancel(){state.cancelled=true}}),
    writable:new WritableStream({write(bytes){
      state.writes.push([...bytes]);
      if (reply) {
        const response=ack(code);
        controller.enqueue(response.slice(0,7));
        controller.enqueue(response.slice(7));
      }
    }}),
    async open(options){assert.equal(options.baudRate,115200);state.opened=true},
    async setSignals(signals){state.signals.push(signals)},
    async close(){state.closed=true},
  };
  return {port,state};
}
{
  const {port,state}=portMock('uk');
  await configureLanguage(port,'uk',1000);
  assert.deepEqual(state.writes,[[...languagePacket('uk')]]);
  assert(state.opened && state.closed && state.cancelled);
  assert(!port.readable.locked && !port.writable.locked);
  assert(state.signals.every(s=>s.dataTerminalReady===false));
  assert.equal(state.signals.at(-1).requestToSend,false);
}
{
  const {port,state}=portMock('de',false);
  await assert.rejects(configureLanguage(port,'de',15), /not confirmed/);
  assert(state.closed && state.cancelled);
  assert(!port.readable.locked && !port.writable.locked);
}
{
  const {port,state}=portMock('de');
  await assert.rejects(configureLanguage(port,'xx'), /Unsupported/);
  assert(!state.opened); assert.equal(state.writes.length,0);
}
console.log('Installer language: eleven packets, corrupt/truncated/wrong replies, preference and serial cleanup passed');

for(const enabled of [false,true]){
 const packet=readingMenuPacket(enabled);assert.equal(packet[9],0x82);assert.equal(packet[10],1);
 assert.equal(packet.at(-1),packet.slice(0,-1).reduce((n,v)=>(n+v)&255,0));
 const bytes=[73,77,80,82,79,86,1,4,4,0x82,2,1,enabled?49:48];
 const response=new Uint8Array([...bytes,bytes.reduce((n,v)=>(n+v)&255,0)]);
 assert(readingMenuAcknowledged(response,enabled));
 assert(!readingMenuAcknowledged(response,!enabled));
 for(let n=0;n<response.length;n++)assert(!readingMenuAcknowledged(response.slice(0,n),enabled));
 response[13]^=1;assert(!readingMenuAcknowledged(response,enabled));
}
assert.throws(()=>readingMenuPacket(1),/boolean/);
