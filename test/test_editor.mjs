import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import vm from 'node:vm';
const probe=process.argv[2];
for(const locale of ['de','en','fr','es','it','pl','cs','ru','uk','nl','pt']){
 const html=execFileSync(probe,[locale],{encoding:'utf8'});
 const scripts=[...html.matchAll(/<script>([\s\S]*?)<\/script>/g)].map(x=>x[1]);
 assert.equal(scripts.length,4);
 const nodes={},posted=[],alerts=[],draws=[],rotations=[];
 const pixels=new Uint8ClampedArray(250*122*4).fill(255);
 const ctx={save(){},restore(){},translate(){},rotate(angle){rotations.push(angle)},createImageData(w,h){return{data:new Uint8ClampedArray(w*h*4)}},fillRect(){},drawImage(...a){draws.push(a)},getImageData(){return{data:pixels.slice()}},putImageData(){}};
 const get=id=>nodes[id]??={value:id==='ssZoom'?'100':id==='ssDestination'?'auto':'0',checked:false,files:[],textContent:'',events:{},
  addEventListener(name,fn){this.events[name]=fn},getContext(){return ctx},setPointerCapture(){}};
 class Fields{constructor(){this.values={}}append(k,v){this.values[k]=v}}
 class Reader{readAsDataURL(){this.result='synthetic:image';this.onload()}readAsArrayBuffer(file){this.result=file.bytes;this.onload()}}
 class Image{constructor(){this.width=500;this.height=244}set src(value){this.onload()}}
 const context={document:{getElementById:get,createElement(){return{getContext(){return ctx}}}},
  FormData:Fields,FileReader:Reader,Image,Blob,Uint8Array,alert:x=>alerts.push(x),setTimeout(){},
  palaFmt(format,...args){return format.replace(/%(\d+)/g,(_,n)=>String(args[Number(n)-1]))},
  fetch(url,options){posted.push({url,fields:options.body.values});return Promise.resolve({ok:true})}};
 context.window=context;vm.runInNewContext(scripts.slice(2).join('\n'),context);
 get('ssUploadBtn').events.click();assert.equal(alerts.length,1);assert.equal(posted.length,0);
 get('ssEditFile').files=[{}];get('ssEditFile').events.change();
 assert(draws.length>0);assert.equal(draws.at(-1)[3],250);assert.equal(draws.at(-1)[4],122);
 get('ssDestination').value='single';get('ssUploadBtn').events.click();await new Promise(setImmediate);
 assert.equal(posted.length,1);assert.equal(posted[0].url,'/screensavers/upload?single=1');
 const bytes=new Uint8Array(await posted[0].fields.file.arrayBuffer());
 assert.equal(bytes.length,3904);assert.equal(bytes[0],255);assert.equal(bytes[31],3);
 assert.equal(get('ssUploadBtn').disabled,false);assert(get('ssUploadStatus').textContent.length>0);
 get('ssRotateBtn').events.click();assert.equal(rotations.at(-1),Math.PI/2);
 assert.equal(draws.at(-1)[3],122);assert.equal(draws.at(-1)[4],60);
 for(let turn=0;turn<3;turn++)get('ssRotateBtn').events.click();
 assert.equal(rotations.at(-1),0);assert.equal(draws.at(-1)[3],250);
 get('ssEditFile').files=[{name:'bad.BIN',bytes:new Uint8Array(3903).buffer}];get('ssEditFile').events.change();
 assert(get('ssUploadStatus').textContent.length>0);
 const before=posted.length;get('ssUploadBtn').events.click();assert.equal(posted.length,before);
 get('ssEditFile').files=[{name:'saved.bin',bytes:new Uint8Array(3904).buffer}];get('ssEditFile').events.change();
 assert.equal(draws.at(-1)[0].width,250);assert.equal(draws.at(-1)[0].height,122);
 get('ssResetBtn').events.click();assert.equal(get('ssZoom').value,'100');
 assert.equal(get('ssPanX').value,'0');assert.equal(get('ssPanY').value,'0');
}
console.log('All eleven actual editor scripts preserve image fit, empty-image validation, bitmap packing, upload destination and reset.');
