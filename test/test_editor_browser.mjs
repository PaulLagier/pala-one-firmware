// Optional real-Canvas regression: node test/test_editor_browser.mjs /path/to/chrome
// Uses an isolated Chrome profile, synthetic files and Node's built-in WebSocket.
import assert from 'node:assert/strict';
import {spawn} from 'node:child_process';
import {mkdtemp, readFile, writeFile, rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {fileURLToPath, pathToFileURL} from 'node:url';
import {setTimeout as delay} from 'node:timers/promises';
const root=fileURLToPath(new URL('../',import.meta.url));
const folder=await mkdtemp(join(tmpdir(),'pala-editor-browser-'));
const page=join(folder,'test.html');
const fixture=await readFile(join(root,'test/editor-browser-fixture.html'),'utf8');
const source=await readFile(join(root,'Pala_One_2_1/src/web/assets/editor.js'),'utf8');
const style=await readFile(join(root,'Pala_One_2_1/src/web/assets/editor.css'),'utf8');
assert.equal(fixture.split('__PALA_EDITOR_SOURCE__').length,2);
await writeFile(page,fixture.replace('__PALA_EDITOR_SOURCE__',()=>source).replace('__PALA_EDITOR_STYLE__',()=>style));
const chrome=spawn(process.argv[2],[`--user-data-dir=${folder}/profile`,'--headless=new',
 '--remote-debugging-address=127.0.0.1','--remote-debugging-port=0','--no-first-run',
 '--no-default-browser-check','--disable-background-networking','--disable-extensions','about:blank'],
 {stdio:'ignore'});
let socket;
const deadline=Date.now()+20000;
try{
 let active;
 while(Date.now()<deadline){
  try{active=(await readFile(join(folder,'profile/DevToolsActivePort'),'utf8')).trim().split('\n');break}catch{}
  assert.equal(chrome.exitCode,null,'Chrome exited before debugger initialization');await delay(50);
 }
 assert(active,'Chrome debugger initialization timed out');
 socket=new WebSocket(`ws://127.0.0.1:${active[0]}${active[1]}`);
 await new Promise((resolve,reject)=>{socket.addEventListener('open',resolve,{once:true});socket.addEventListener('error',reject,{once:true})});
 let sequence=0;const pending=new Map();
 socket.addEventListener('message',event=>{const message=JSON.parse(event.data);if(pending.has(message.id)){pending.get(message.id)(message);pending.delete(message.id)}});
 async function command(method,params={},sessionId){
  const id=++sequence;
  let timer;
  try{
   const response=await Promise.race([new Promise(resolve=>{pending.set(id,resolve);socket.send(JSON.stringify({id,method,params,sessionId}))}),new Promise((_,reject)=>{timer=setTimeout(()=>reject(Error('Debugger command timed out: '+method)),5000)})]);
   assert(!response.error,JSON.stringify(response.error));return response.result;
  }finally{clearTimeout(timer);pending.delete(id)}
 }
 const target=await command('Target.createTarget',{url:'about:blank'});
 const {sessionId}=await command('Target.attachToTarget',{targetId:target.targetId,flatten:true});
 await command('Page.navigate',{url:pathToFileURL(page).href},sessionId);
 let report;
 while(Date.now()<deadline){
  const response=await command('Runtime.evaluate',{expression:"document.getElementById('result')?.textContent",returnByValue:true},sessionId);
  const value=response.result?.value;
  if(value&&value!=='RUNNING'){assert(!value.startsWith('FAIL'),value);report=JSON.parse(value);break}
  await delay(50);
 }
 assert.equal(report?.success,true,'Browser regression timed out');console.log(JSON.stringify(report));
}finally{
 socket?.close();chrome.kill('SIGTERM');
 if(chrome.exitCode===null)await Promise.race([new Promise(resolve=>chrome.once('exit',resolve)),delay(3000)]);
 if(chrome.exitCode===null)chrome.kill('SIGKILL');
 await rm(folder,{recursive:true,force:true,maxRetries:5,retryDelay:100});
}
