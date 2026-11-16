#include "src/lang/locale_core.h"
#include "src/pure/catalog_inflater.h"
#include "src/pure/web_script.h"
#include "src/web/localized_scripts.h"
#include <iostream>
#include <iterator>
#include <string>
static bool inflate(const uint8_t *in, size_t n, uint8_t *out, size_t m) {
  CatalogInflater decoder;
  return decoder.inflate(in, n, out, m);
}
int main(int argc, char **argv) {
  if (argc == 1) {
    const std::string text((std::istreambuf_iterator<char>(std::cin)), {});
    std::cout << webJsString(text.c_str()).c_str();
    return 0;
  }
  if (!Language::select(argv[1], inflate))
    return 2;
  String script;
  appendReadScript(script);
  appendEditorScript(script);
  std::cout << script.c_str();
}
