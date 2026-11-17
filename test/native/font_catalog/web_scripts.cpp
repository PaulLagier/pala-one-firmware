#include "src/lang/locale_core.h"
#include "src/pure/catalog_inflater.h"
#include "src/pure/web_script.h"
#include "src/web/localized_scripts.h"
#include <iostream>
#include <fstream>
#include "src/web/web_assets.h"
#include <iterator>
#include <string>
namespace WebAssets {
void appendScript(String &out, Id id) {
  if (id != Id::EditorJs && id != Id::ReaderJs)
    std::abort();
  std::ifstream file(id == Id::EditorJs ? WEB_EDITOR_ASSET : WEB_READER_ASSET);
  if (!file)
    std::abort();
  const std::string text((std::istreambuf_iterator<char>(file)), {});
  out += "<script>";
  out += text.c_str();
  out += "</script>";
}
} // namespace WebAssets
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
