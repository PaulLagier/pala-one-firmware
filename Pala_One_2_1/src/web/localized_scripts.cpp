#include "src/web/localized_scripts.h"
#include "src/lang/lang.h"
#include "src/pure/web_script.h"

// Retain main #128 typography folding and UTF-8 offsets with runtime-localized labels.
void appendReadScript(String &out) {
  out += R"PALA(<script>(function(){
var bookId=parseInt(document.getElementById('bvData').dataset.id,10);
var elText=document.getElementById('bvText'),elQ=document.getElementById('bvFind'),elBtnAll=document.getElementById('bvFindBtn'),elNext=document.getElementById('bvFindNext'),elPrev=document.getElementById('bvFindPrev'),elStat=document.getElementById('bvFindStat'),elJump=document.getElementById('bvJumpBtn'),elJumpStat=document.getElementById('bvJumpStat');
var rawText='',hits=[],curHit=-1,normCache=null;
function esc(s){return s.replace(/[&<>]/g,function(c){return c==='&'?'&amp;':c==='<'?'&lt;':'&gt;'});}
var TYPO={'‘':"'",'’':"'",'‚':"'",'‛':"'",'“':'"','”':'"','„':'"','‟':'"','‹':'"','›':'"','«':'"','»':'"','–':'-','—':'-','―':'-','…':'...','\u00A0':' '};
function normTextMap(s){
 var n='',map=[],pend=false;
 for(var i=0;i<s.length;i++){
  var t=TYPO[s[i]]||s[i];
  for(var k=0;k<t.length;k++){
   var ch=t[k];
   if(ch===' '||ch==='\n'||ch==='\r'||ch==='\t'){if(n.length)pend=true;continue;}
   if(pend){n+=' ';map.push(i);pend=false;}
   var lower=ch.toLowerCase();n+=lower;
   for(var j=0;j<lower.length;j++)map.push(i);
  }
 }
 return{n:n,map:map};
}
function findMatches(raw,q,cache){
 var qn=normTextMap(q).n;
 if(!qn)return{hits:[],cache:cache,empty:true};
 if(!cache)cache=normTextMap(raw);
 var hits=[],i=0;
 while(true){
  var p=cache.n.indexOf(qn,i);if(p<0)break;
  var start=cache.map[p],end=cache.map[p+qn.length-1]+1;
  if(!hits.length||start>=hits[hits.length-1].end)hits.push({start:start,end:end});
  i=p+qn.length;
 }
 return{hits:hits,cache:cache,empty:false};
}
function utf8Len(s){return new TextEncoder().encode(s).length;}
function render(highlight){
 if(!highlight){elText.textContent=rawText;return;}
 var html='',last=0;
 for(var i=0;i<hits.length;i++){
  var h=hits[i];html+=esc(rawText.slice(last,h.start));
  html+='<span class="find-hit'+(i===curHit?' find-cur':'')+'" data-i="'+i+'">'+esc(rawText.slice(h.start,h.end))+'</span>';
  last=h.end;
 }
 html+=esc(rawText.slice(last));elText.innerHTML=html;
}
function gotoHit(i){
 if(hits.length===0){elStat.textContent=)PALA";
  out += webJsString(D_WEB_READ_JS_NO_MATCHES);
  out += R"PALA(;return;}
 curHit=((i%hits.length)+hits.length)%hits.length;render(true);
 var n=elText.querySelector('.find-cur');if(n)n.scrollIntoView({block:'center',behavior:'smooth'});
 elStat.textContent=palaFmt()PALA";
  out += webJsString(D_WEB_READ_JS_MATCH_FMT);
  out += R"PALA(,curHit+1,hits.length,utf8Len(rawText.slice(0,hits[curHit].start)));
}
function search(){
 var r=findMatches(rawText,elQ.value,normCache);normCache=r.cache||normCache;
 if(r.empty){hits=[];curHit=-1;render(false);elStat.textContent=)PALA";
  out += webJsString(D_WEB_READ_JS_ENTER_PHRASE);
  out += R"PALA(;return;}
 hits=r.hits;curHit=hits.length>0?0:-1;render(true);
 if(hits.length===0)elStat.textContent=)PALA";
  out += webJsString(D_WEB_READ_JS_NO_MATCHES);
  out += R"PALA(;else gotoHit(0);
}
elBtnAll.addEventListener('click',search);
elQ.addEventListener('keydown',function(e){if(e.key==='Enter'){e.preventDefault();search();}});
elNext.addEventListener('click',function(){if(hits.length)gotoHit(curHit+1);});
elPrev.addEventListener('click',function(){if(hits.length)gotoHit(curHit-1);});
elText.addEventListener('click',function(e){var t=e.target;if(t.classList&&t.classList.contains('find-hit')){var idx=parseInt(t.dataset.i,10);if(!isNaN(idx))gotoHit(idx);}});
elJump.addEventListener('click',function(){
 if(curHit<0){elJumpStat.textContent=)PALA";
  out += webJsString(D_WEB_READ_JS_FIND_FIRST);
  out += R"PALA(;return;}
 var off=utf8Len(rawText.slice(0,hits[curHit].start)),fd=new FormData();fd.append('id',String(bookId));fd.append('offset',String(off));
 elJumpStat.textContent=)PALA";
  out += webJsString(D_WEB_READ_JS_SAVING);
  out += R"PALA(;elJump.disabled=true;
 fetch('/jumpoffset',{method:'POST',body:fd,redirect:'follow'}).then(function(r){
  elJumpStat.textContent=r.ok?palaFmt()PALA";
  out += webJsString(D_WEB_READ_JS_SAVED_FMT);
  out += R"PALA(,off):palaFmt()PALA";
  out += webJsString(D_WEB_READ_JS_SAVE_HTTP_FMT);
  out += R"PALA(,r.status);
 }).catch(function(e){elJumpStat.textContent=palaFmt()PALA";
  out += webJsString(D_WEB_READ_JS_SAVE_FAIL_FMT);
  out += R"PALA(,e&&e.message?e.message:)PALA";
  out += webJsString(D_WEB_READ_JS_ERROR);
  out += R"PALA();
 }).finally(function(){elJump.disabled=false;});
});
fetch('/readbook-text?id='+encodeURIComponent(bookId)).then(function(r){return r.text();}).then(function(t){
 rawText=t;normCache=null;render(false);elStat.textContent=palaFmt()PALA";
  out += webJsString(D_WEB_READ_JS_LOADED_FMT);
  out += R"PALA(,utf8Len(rawText));
}).catch(function(){elStat.textContent=)PALA";
  out += webJsString(D_WEB_READ_JS_LOAD_FAILED);
  out += R"PALA(;});
})();</script>)PALA";
}

void appendEditorScript(String &out) {
  out +=
      "<script>(function(){if(window.__palaSleepEditorInit)return;window.__palaSleepEditorInit=1;"
      "var W=250,H=122,ROW=32,TOTAL=H*ROW;var fileInput=document.getElementById('ssEditFile');var "
      "tol=document.getElementById('ssTolerance');var "
      "tolLbl=document.getElementById('ssToleranceLabel');var "
      "zoom=document.getElementById('ssZoom');var "
      "zoomLbl=document.getElementById('ssZoomLabel');var "
      "panX=document.getElementById('ssPanX');var panY=document.getElementById('ssPanY');var "
      "panXLbl=document.getElementById('ssPanXLabel');var "
      "panYLbl=document.getElementById('ssPanYLabel');var "
      "inv=document.getElementById('ssInvert');var canvas=document.getElementById('ssPreview');var "
      "meta=document.getElementById('ssMeta');var "
      "status=document.getElementById('ssUploadStatus');var "
      "resetBtn=document.getElementById('ssResetBtn');var "
      "uploadBtn=document.getElementById('ssUploadBtn');var "
      "dstSel=document.getElementById('ssDestination');if(!fileInput||!tol||!zoom||!panX||!panY||!"
      "inv||!canvas||!meta||!status||!resetBtn||!uploadBtn)return;var "
      "ctx=canvas.getContext('2d',{willReadFrequently:true});var "
      "work=document.createElement('canvas');work.width=W;work.height=H;var "
      "workCtx=work.getContext('2d',{willReadFrequently:true});var sourceImage=null;var "
      "isDragging=false,dragStartX=0,dragStartY=0,dragPanX=0,dragPanY=0;var pointers={};var "
      "pinch={active:false,startDist:0,startZoom:100,startPanX:0,startPanY:0,startMidX:0,startMidY:"
      "0};function clamp(v,a,b){return v<a?a:(v>b?b:v)}function "
      "thr(o){o=clamp(parseInt(o||0,10)||0,-100,100);return "
      "clamp(128+Math.round(o*(255-128)/100),0,255)}function setLbls(){var "
      "t=parseInt(tol.value,10)||0;tolLbl.textContent=(t>0?'+':'')+t+'%';zoomLbl.textContent=zoom."
      "value+'%';panXLbl.textContent=panX.value+";
  out += webJsString(D_WEB_SS_JS_PX_SUFFIX);
  out += ";panYLbl.textContent=panY.value+";
  out += webJsString(D_WEB_SS_JS_PX_SUFFIX);
  out += ";}function "
         "fit(){if(!sourceImage)return;zoom.value='100';panX.value='0';panY.value='0';setLbls();"
         "render();}function "
         "drawToWork(){workCtx.fillStyle='#fff';workCtx.fillRect(0,0,W,H);if(!sourceImage)return;"
         "var base=Math.min(W/sourceImage.width,H/sourceImage.height);var "
         "s=base*((parseInt(zoom.value,10)||100)/100);if(!isFinite(s)||s<=0)s=base;var "
         "dw=Math.max(1,Math.round(sourceImage.width*s));var "
         "dh=Math.max(1,Math.round(sourceImage.height*s));var "
         "x=((W-dw)/2)+(parseInt(panX.value,10)||0);var "
         "y=((H-dh)/"
         "2)+(parseInt(panY.value,10)||0);workCtx.drawImage(sourceImage,x,y,dw,dh);}function "
         "toOneBit(){var "
         "img=workCtx.getImageData(0,0,W,H),d=img.data,t=thr(tol.value),iv=!!inv.checked;for(var "
         "i=0;i<d.length;i+=4){var L=((d[i]*299)+(d[i+1]*587)+(d[i+2]*114))/1000;var "
         "w=(L>=t);if(iv)w=!w;var "
         "c=w?255:0;d[i]=c;d[i+1]=c;d[i+2]=c;d[i+3]=255;}ctx.putImageData(img,0,0);return "
         "img;}function pack(img){var d=img.data,o=new Uint8Array(TOTAL);for(var "
         "y=0;y<H;y++)for(var x=0;x<W;x++){var i=(y*W+x)*4;if(d[i]>=128){var "
         "b=(y*ROW)+(x>>3);o[b]=o[b]|(1<<(x&7));}}return o;}function "
         "render(){setLbls();drawToWork();var i=toOneBit();if(!sourceImage){meta.textContent=";
  out += webJsString(D_WEB_SS_NO_IMAGE);
  out += ";return null;}meta.textContent=palaFmt(";
  out += webJsString(D_WEB_SS_JS_PREVIEW_FMT);
  out += ",W,H,thr(tol.value),TOTAL);return i;}function pts(){var a=[];for(var k in "
         "pointers){a.push(pointers[k]);}return a;}function dist(a,b){var "
         "dx=a.x-b.x,dy=a.y-b.y;return Math.sqrt(dx*dx+dy*dy);}function "
         "mid(a,b){return{x:(a.x+b.x)/2,y:(a.y+b.y)/2};}function startPinch(){var "
         "p=pts();if(p.length===2){pinch.active=true;pinch.startDist=Math.max(8,dist(p[0],p[1]));"
         "pinch.startZoom=parseInt(zoom.value,10)||100;pinch.startPanX=parseInt(panX.value,10)||0;"
         "pinch.startPanY=parseInt(panY.value,10)||0;var "
         "m=mid(p[0],p[1]);pinch.startMidX=m.x;pinch.startMidY=m.y;}else "
         "pinch.active=false;}fileInput.addEventListener('change',function(){var "
         "f=fileInput.files&&fileInput.files[0];if(!f){sourceImage=null;render();return;}status."
         "textContent='';var r=new FileReader();r.onload=function(){var im=new "
         "Image();im.onload=function(){sourceImage=im;fit();};im.onerror=function(){status."
         "textContent=";
  out += webJsString(D_WEB_SS_JS_DECODE_FAILED);
  out += ";};im.src=r.result;};r.onerror=function(){status.textContent=";
  out += webJsString(D_WEB_SS_JS_READ_FAILED);
  out +=
      ";};r.readAsDataURL(f);});[tol,zoom,panX,panY,inv].forEach(function(el){el.addEventListener('"
      "input',render);el.addEventListener('change',render);});resetBtn.addEventListener('click',"
      "function(){fit();status.textContent='';});canvas.addEventListener('pointerdown',function(e){"
      "pointers[e.pointerId]={x:e.clientX,y:e.clientY};canvas.setPointerCapture(e.pointerId);var "
      "p=pts();if(p.length===1){isDragging=true;dragStartX=e.clientX;dragStartY=e.clientY;dragPanX="
      "parseInt(panX.value,10)||0;dragPanY=parseInt(panY.value,10)||0;}startPinch();});canvas."
      "addEventListener('pointermove',function(e){if(!pointers[e.pointerId])return;pointers[e."
      "pointerId].x=e.clientX;pointers[e.pointerId].y=e.clientY;var "
      "p=pts();if(pinch.active&&p.length===2){var "
      "d=Math.max(8,dist(p[0],p[1])),r=d/"
      "pinch.startDist;zoom.value=String(clamp(Math.round(pinch.startZoom*r),10,400));var "
      "m=mid(p[0],p[1]);panX.value=String(clamp(pinch.startPanX+Math.round(m.x-pinch.startMidX),-"
      "250,250));panY.value=String(clamp(pinch.startPanY+Math.round(m.y-pinch.startMidY),-180,180))"
      ";render();return;}if(isDragging&&p.length===1){panX.value=String(clamp(dragPanX+Math.round("
      "e.clientX-dragStartX),-250,250));panY.value=String(clamp(dragPanY+Math.round(e.clientY-"
      "dragStartY),-180,180));render();}});function endP(e){delete pointers[e.pointerId];var "
      "p=pts();if(p.length===1){isDragging=true;dragStartX=p[0].x;dragStartY=p[0].y;dragPanX="
      "parseInt(panX.value,10)||0;dragPanY=parseInt(panY.value,10)||0;}else "
      "isDragging=false;startPinch();}canvas.addEventListener('pointerup',endP);canvas."
      "addEventListener('pointercancel',endP);canvas.addEventListener('wheel',function(e){if(!"
      "sourceImage)return;e.preventDefault();var "
      "z=parseInt(zoom.value,10)||100,s=Math.max(2,Math.round(Math.abs(e.deltaY)/"
      "25));zoom.value=String(clamp(z-(e.deltaY>0?s:-s),10,400));render();},{passive:false});"
      "uploadBtn.addEventListener('click',function(){if(!sourceImage){alert(";
  out += webJsString(D_WEB_SS_ERR_CHOOSE_IMAGE);
  out += ");return;}var img=render();if(!img){status.textContent=";
  out += webJsString(D_WEB_SS_JS_PREVIEW_NOT_READY);
  out += ";return;}var bytes=pack(img);var fd=new FormData();fd.append('file',new "
         "Blob([bytes],{type:'application/octet-stream'}),'sleep-editor.bin');var "
         "url='/screensavers/upload';var "
         "sel=dstSel?dstSel.value:'auto';if(sel==='single')url+='?single=1';else "
         "if(sel!=='auto')url+='?slot='+encodeURIComponent(sel);status.textContent=";
  out += webJsString(D_WEB_SS_JS_UPLOADING);
  out += ";uploadBtn.disabled=true;fetch(url,{method:'POST',body:fd}).then(function(r){if(!r.ok)"
         "return r.text().then(function(t){throw new Error(t||palaFmt(";
  out += webJsString(D_WEB_SS_JS_HTTP_FMT);
  out += ",r.status));});status.textContent=";
  out += webJsString(D_WEB_SS_JS_UPLOAD_COMPLETE);
  out += ";setTimeout(function(){window.location.href='/"
         "screensavers';},600);}).catch(function(e){status.textContent=palaFmt(";
  out += webJsString(D_WEB_SS_JS_UPLOAD_FAIL_FMT);
  out += ",(e&&e.message?e.message:";
  out += webJsString(D_WEB_SS_JS_ERROR);
  out +=
      "));}).finally(function(){uploadBtn.disabled=false;});});setLbls();render();})();</script>";
}
