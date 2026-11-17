import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import vm from 'node:vm';
const probe=process.argv[2];
for(const locale of ['de','en','fr','es','it','pl','cs','ru','uk','nl','pt']){
 const html=execFileSync(probe,[locale],{encoding:'utf8'});
 let code=[...html.matchAll(/<script>([\s\S]*?)<\/script>/g)].slice(0,2).map(x=>x[1]).join('\n');
 const end=code.lastIndexOf('})();');assert(end>=0);
 code=code.slice(0,end)+'globalThis.readerTest={normTextMap,findMatches,utf8Len};'+code.slice(end);
 const nodes={};const posted=[];
 const raw='Größe 📚\n"Grüße" - all... now.\nNext paragraph';
 const query='“GRÜẞE”\n—\tall…';
 const get=id=>nodes[id]??=( {dataset:{id:'0'},value:'',textContent:'',innerHTML:'',events:{},addEventListener(kind,fn){this.events[kind]=fn},querySelector(){return null}} );
 class Fields{constructor(){this.values={}}append(key,value){this.values[key]=value}get(key){return this.values[key]}}
 const context={document:{getElementById:get},TextEncoder,FormData:Fields,
  palaFmt(format,...args){return format.replace(/%(\d+)/g,(_,n)=>String(args[Number(n)-1]))},
  fetch(url,options){if(url.startsWith('/readbook-text'))return Promise.resolve({text:async()=>raw});posted.push(options.body.get('offset'));return Promise.resolve({ok:true,status:200})}};
 context.window=context;vm.runInNewContext(code,context);await new Promise(setImmediate);
 assert(get('bvFindStat').textContent.includes(String(Buffer.byteLength(raw,'utf8'))));
 const engine=context.readerTest;
 const found=engine.findMatches(raw,query,null);assert.equal(found.hits.length,1);
 assert.equal(found.hits[0].start,raw.indexOf('"'));
 assert.equal(engine.normTextMap('İstanbul').map.length,engine.normTextMap('İstanbul').n.length);
 assert.equal(engine.findMatches('İstanbul','i\u0307stanbul',null).hits[0].end,8);
 assert.equal(engine.findMatches('…','.',null).hits.length,1);
 assert.equal(engine.findMatches('😀 one 😀','😀',null).hits.length,2);
 assert(engine.findMatches(raw,' \n\t',null).empty);
 get('bvFind').value=query;get('bvFindBtn').events.click();
 const expected=String(Buffer.byteLength(raw.slice(0,raw.indexOf('"')),'utf8'));
 assert(get('bvFindStat').textContent.includes(expected));
 get('bvJumpBtn').events.click();await new Promise(setImmediate);
 assert.deepEqual(posted,[expected]);assert.equal(get('bvJumpBtn').disabled,false);
 assert(get('bvText').innerHTML.includes('find-hit find-cur'));
}
console.log('All eleven actual reader scripts preserve folded matching, Unicode index maps and UTF-8 jump/status offsets.');
