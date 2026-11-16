export const LANGUAGES=['de','en','fr','es','it','pl','cs','ru','uk','nl','pt'];
export function browserLanguage(languages){for(const value of languages){const code=value.toLowerCase().split(/[-_]/)[0];if(LANGUAGES.includes(code))return code}return 'en'}
export function languagePacket(code){
 if(!LANGUAGES.includes(code))throw new Error('Unsupported language');
 const bytes=[73,77,80,82,79,86,1,3,4,0x81,2,...new TextEncoder().encode(code)];
 return new Uint8Array([...bytes,bytes.reduce((n,v)=>(n+v)&255,0)]);
}
export function languageAcknowledged(bytes,code){
 for(let i=0;i+10<=bytes.length;i++){
  if(![73,77,80,82,79,86,1,4].every((v,j)=>bytes[i+j]===v))continue;
  const size=bytes[i+8],end=i+9+size;if(end>=bytes.length)continue;
  let sum=0;for(let j=i;j<end;j++)sum=(sum+bytes[j])&255;
  if(sum!==bytes[end])continue;
  // Vendor RPC response: command, argument-byte count, string length, code.
  if(size===5&&bytes[i+9]===0x81&&bytes[i+10]===3&&bytes[i+11]===2&&bytes[i+12]===code.charCodeAt(0)&&bytes[i+13]===code.charCodeAt(1))return true;
 }
 return false;
}
async function configureSetting(port,packet,acknowledged,message,timeoutMs){let reader,writer,timer,retry;const bytes=[];
 await port.open({baudRate:115200,bufferSize:8192});
 try{
  reader=port.readable.getReader();writer=port.writable.getWriter();
  let expired=false;timer=setTimeout(()=>{expired=true;reader.cancel().catch(()=>{})},timeoutMs);
  const send=()=>writer.write(packet).catch(()=>{});await send();retry=setInterval(send,1000);
  while(!expired){const {value,done}=await reader.read();if(done)break;
   if(value){bytes.push(...value);if(bytes.length>4096)bytes.splice(0,bytes.length-4096)}
   if(acknowledged(bytes))return;
  }
  throw new Error(message);
 }finally{
  clearTimeout(timer);clearInterval(retry);
  if(reader){try{await reader.cancel()}catch{}reader.releaseLock()}
  if(writer)writer.releaseLock();
  // Restart in the persisted language; USB bridge GPIO0 stays released.
  try{await port.setSignals({dataTerminalReady:false,requestToSend:true});await new Promise(r=>setTimeout(r,220));await port.setSignals({dataTerminalReady:false,requestToSend:false})}catch{}
  await port.close();
 }
}

export function readingMenuPacket(enabled){
 if(typeof enabled!=='boolean')throw new Error('Expected a boolean menu selection');
 const bytes=[73,77,80,82,79,86,1,3,3,0x82,1,enabled?49:48];
 return new Uint8Array([...bytes,bytes.reduce((n,v)=>(n+v)&255,0)]);
}
export function readingMenuAcknowledged(bytes,enabled){
 for(let i=0;i+14<=bytes.length;i++){
  if(![73,77,80,82,79,86,1,4,4,0x82,2,1,enabled?49:48].every((v,j)=>bytes[i+j]===v))continue;
  let sum=0;for(let j=i;j<i+13;j++)sum=(sum+bytes[j])&255;
  if(sum===bytes[i+13])return true;
 }
 return false;
}
export async function configureLanguage(port,code,timeoutMs=15000){
 return configureSetting(port,languagePacket(code),bytes=>languageAcknowledged(bytes,code),
  'Firmware installed; language was not confirmed. Retry or select it in the Pala app.',timeoutMs);
}
export async function configureReadingMenu(port,enabled,timeoutMs=15000){
 return configureSetting(port,readingMenuPacket(enabled),bytes=>readingMenuAcknowledged(bytes,enabled),
  'Firmware installed; menu selection was not confirmed. Retry or select it in the Pala app.',timeoutMs);
}
