#include "src/lang/locale_core.h"
#include "src/pure/catalog_inflater.h"
// Host persistence boundary: the core is real; NVS rollback is exercised by
// runtime_locales_* and hardware persistence remains the firmware adapter.
namespace Language {
bool setLanguage(const char* code) {
  return select(code, [](const uint8_t* input, size_t n, uint8_t* output, size_t m) {
    CatalogInflater decoder;
    return decoder.inflate(input,n,output,m);
  });
}
}
