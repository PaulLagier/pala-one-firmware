#include "src/lang/locale_core.h"
#include "src/pure/catalog_inflater.h"
#include "src/state.h"
#if PALA_DIAGNOSTICS
#include <esp_heap_caps.h>
#endif
namespace Language {
namespace {
constexpr const char* key="cfg_lang";
bool persist(const char* code){return prefs.putString(key,code)>=strlen(code);}
// Synchronous main-thread callback; the scoped decoder outlives every invocation.
CatalogInflater* activeDecoder=nullptr;
bool inflateCatalog(const uint8_t* in,size_t inSize,uint8_t* out,size_t outSize){
  if(activeDecoder)return activeDecoder->inflate(in,inSize,out,outSize);
  // Lazy browser-pool reads happen after the scoped selection decoder is gone.
  CatalogInflater decoder;return decoder.inflate(in,inSize,out,outSize);
}
bool selectWithDecoder(const char* code,Persist save=nullptr){
  if(!code)return false;
  if(!strcmp(code,currentCode())||!strcmp(code,defaultCode))return select(code,nullptr,save);
  // Allocate the bounded catalog decoder first. Allocating it between output blocks can fail on a
  // fragmented BLE heap even when the total free space is sufficient.
  CatalogInflater decoder;if(!decoder.ready()){
#if PALA_DIAGNOSTICS
    Serial.printf("[locale-failure] reason=decoder-allocation bytes=%u heap=%u largest=%u\n",unsigned(CatalogInflater::workspaceBytes()),unsigned(ESP.getFreeHeap()),unsigned(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)));
#endif
    return false;
  }
  auto* previous=activeDecoder;activeDecoder=&decoder;
  const bool result=select(code,inflateCatalog,save);activeDecoder=previous;
  return result;
}
}
void loadSettings(){const String saved=prefs.getString(key,defaultCode);(void)selectWithDecoder(saved.c_str());}
bool setLanguage(const char* code){return selectWithDecoder(code,persist);}
}
