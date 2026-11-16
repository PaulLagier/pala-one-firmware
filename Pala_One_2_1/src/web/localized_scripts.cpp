#include "src/web/localized_scripts.h"
#include "src/lang/lang.h"
#include "src/pure/web_script.h"
#include "src/web/web_assets.h"

// Retain main #128 typography folding and UTF-8 offsets with runtime-localized labels.
void appendReadScript(String &out) {
  out += R"PALA(<script>(function(){
var bid=parseInt(document.getElementById('bvData').dataset.id,10);
var txt=document.getElementById('bvText'),qry=document.getElementById('bvFind'),all=document.getElementById('bvFindBtn'),next=document.getElementById('bvFindNext'),prev=document.getElementById('bvFindPrev'),stat=document.getElementById('bvFindStat'),jump=document.getElementById('bvJumpBtn'),jstat=document.getElementById('bvJumpStat');
var raw='',hits=[],cur=-1,cacheN=null;
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
if(!highlight){txt.textContent=raw;return;}
var html='',last=0;
for(var i=0;i<hits.length;i++){
var h=hits[i];html+=esc(raw.slice(last,h.start));
html+='<span class="find-hit'+(i===cur?' find-cur':'')+'" data-i="'+i+'">'+esc(raw.slice(h.start,h.end))+'</span>';
last=h.end;
}
html+=esc(raw.slice(last));txt.innerHTML=html;
}
function gotoHit(i){
if(hits.length===0){stat.textContent=)PALA";
  out += webJsString(D_WEB_READ_JS_NO_MATCHES);
  out += R"PALA(;return;}
cur=((i%hits.length)+hits.length)%hits.length;render(true);
var n=txt.querySelector('.find-cur');if(n)n.scrollIntoView({block:'center',behavior:'smooth'});
stat.textContent=palaFmt()PALA";
  out += webJsString(D_WEB_READ_JS_MATCH_FMT);
  out += R"PALA(,cur+1,hits.length,utf8Len(raw.slice(0,hits[cur].start)));
}
function search(){
var r=findMatches(raw,qry.value,cacheN);cacheN=r.cache||cacheN;
if(r.empty){hits=[];cur=-1;render(false);stat.textContent=)PALA";
  out += webJsString(D_WEB_READ_JS_ENTER_PHRASE);
  out += R"PALA(;return;}
hits=r.hits;cur=hits.length>0?0:-1;render(true);
if(hits.length===0)stat.textContent=)PALA";
  out += webJsString(D_WEB_READ_JS_NO_MATCHES);
  out += R"PALA(;else gotoHit(0);
}
all.addEventListener('click',search);
qry.addEventListener('keydown',function(e){if(e.key==='Enter'){e.preventDefault();search();}});
next.addEventListener('click',function(){if(hits.length)gotoHit(cur+1);});
prev.addEventListener('click',function(){if(hits.length)gotoHit(cur-1);});
txt.addEventListener('click',function(e){var t=e.target;if(t.classList&&t.classList.contains('find-hit')){var idx=parseInt(t.dataset.i,10);if(!isNaN(idx))gotoHit(idx);}});
jump.addEventListener('click',function(){
if(cur<0){jstat.textContent=)PALA";
  out += webJsString(D_WEB_READ_JS_FIND_FIRST);
  out += R"PALA(;return;}
var off=utf8Len(raw.slice(0,hits[cur].start)),fd=new FormData();fd.append('id',String(bid));fd.append('offset',String(off));
jstat.textContent=)PALA";
  out += webJsString(D_WEB_READ_JS_SAVING);
  out += R"PALA(;jump.disabled=true;
fetch('/jumpoffset',{method:'POST',body:fd,redirect:'follow'}).then(function(r){
jstat.textContent=r.ok?palaFmt()PALA";
  out += webJsString(D_WEB_READ_JS_SAVED_FMT);
  out += R"PALA(,off):palaFmt()PALA";
  out += webJsString(D_WEB_READ_JS_SAVE_HTTP_FMT);
  out += R"PALA(,r.status);
}).catch(function(e){jstat.textContent=palaFmt()PALA";
  out += webJsString(D_WEB_READ_JS_SAVE_FAIL_FMT);
  out += R"PALA(,e&&e.message?e.message:)PALA";
  out += webJsString(D_WEB_READ_JS_ERROR);
  out += R"PALA();
}).finally(function(){jump.disabled=false;});
});
fetch('/readbook-text?id='+encodeURIComponent(bid)).then(function(r){return r.text();}).then(function(t){
raw=t;cacheN=null;render(false);stat.textContent=palaFmt()PALA";
  out += webJsString(D_WEB_READ_JS_LOADED_FMT);
  out += R"PALA(,utf8Len(raw));
}).catch(function(){stat.textContent=)PALA";
  out += webJsString(D_WEB_READ_JS_LOAD_FAILED);
  out += R"PALA(;});
})();</script>)PALA";
}

void appendEditorScript(String &out) {
  out += "<script>window.palaEditorLabels=[";
  out += webJsString(D_WEB_SS_JS_PX_SUFFIX);
  out += ",";
  out += webJsString(D_WEB_SS_NO_IMAGE);
  out += ",";
  out += webJsString(D_WEB_SS_JS_PREVIEW_FMT);
  out += ",";
  out += webJsString(D_WEB_SS_JS_DECODE_FAILED);
  out += ",";
  out += webJsString(D_WEB_SS_JS_READ_FAILED);
  out += ",";
  out += webJsString(D_WEB_SS_ERR_CHOOSE_IMAGE);
  out += ",";
  out += webJsString(D_WEB_SS_JS_PREVIEW_NOT_READY);
  out += ",";
  out += webJsString(D_WEB_SS_JS_UPLOADING);
  out += ",";
  out += webJsString(D_WEB_SS_JS_HTTP_FMT);
  out += ",";
  out += webJsString(D_WEB_SS_JS_UPLOAD_COMPLETE);
  out += ",";
  out += webJsString(D_WEB_SS_JS_UPLOAD_FAIL_FMT);
  out += ",";
  out += webJsString(D_WEB_SS_JS_ERROR);
  out += "];</script>";
  WebAssets::appendScript(out, WebAssets::Id::EditorJs);
}
