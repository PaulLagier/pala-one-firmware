#include <array>
#include <memory>
#include <new>
#include <fstream>
#include <iterator>
#include <cstring>
#include <zlib.h>
#include <ArduinoJson.h>
#include "test_framework.h"
#include "src/lang/locale_core.h"
#include "src/pure/catalog_inflater.h"

namespace {
long failArrayAfter = -1;
}
void *operator new[](std::size_t size, const std::nothrow_t &) noexcept {
  if (failArrayAfter == 0)
    return nullptr;
  if (failArrayAfter > 0)
    --failArrayAfter;
  try {
    return ::operator new[](size);
  } catch (...) {
    return nullptr;
  }
}
void operator delete[](void *memory, const std::nothrow_t &) noexcept {
  ::operator delete[](memory);
}
namespace {
CatalogInflater decoder;
unsigned decodeCalls = 0, persistCalls = 0;
int failDecodeAfter = -1;
bool persistResult = true;
std::string saved;
bool inflate(const uint8_t *input, size_t inputSize, uint8_t *output, size_t outputSize) {
  ++decodeCalls;
  if (failDecodeAfter == 0)
    return false;
  if (failDecodeAfter > 0)
    --failDecodeAfter;
  return decoder.inflate(input, inputSize, output, outputSize);
}
bool persist(const char *code) {
  ++persistCalls;
  if (persistResult)
    saved = code;
  return persistResult;
}
JsonDocument catalog(const char *code) {
  std::ifstream input(std::string(CATALOG_DIRECTORY) + "/" + code + ".json", std::ios::binary);
  if (!input)
    throw std::runtime_error("catalog fixture missing");
  const std::string text{std::istreambuf_iterator<char>(input), {}};
  JsonDocument doc;
  if (deserializeJson(doc, text))
    throw std::runtime_error("invalid catalog fixture");
  return doc;
}
void reset() {
  failArrayAfter = -1;
  failDecodeAfter = -1;
  persistResult = true;
  if (!Language::select(Language::defaultCode, inflate))
    throw std::runtime_error("default language failed");
  decodeCalls = persistCalls = 0;
  saved.clear();
}
} // namespace
TEST_CASE("Runtime defaults and all eleven dictionaries decode to their canonical text") {
  reset();
  CHECK(std::string(Language::currentCode()) == Language::defaultCode);
  CHECK_EQ(Language::count(), 11u);
  REQUIRE(decoder.ready());
  CHECK(CatalogInflater::workspaceBytes() <= 2048u);
  CHECK(Language::interfacePoolCount < Language::poolCount);
  for (size_t code = 0; code < Language::count(); ++code) {
    const char *selected = Language::codeAt(code);
    auto reference = catalog(selected);
    REQUIRE(Language::select(selected, inflate, persist));
    CHECK(saved == selected);
    CHECK(std::string(Language::currentCode()) == selected);
    CHECK_EQ(reference.size(), Language::keyCount);
    size_t index = 0;
    for (JsonPairConst entry : reference.as<JsonObjectConst>()) {
      const char *text = Language::text(static_cast<Language::Key>(index++));
      CHECK(std::string(text) == entry.value().as<std::string>());
    }
  }
  CHECK(std::string(Language::text(Language::Key::Count)).empty());
  CHECK(Language::codeAt(Language::count()) == nullptr);
}
TEST_CASE("Runtime allocation failures keep old pointers selection and persistence intact") {
  reset();
  const char *old = D_BOOT_STORAGE_ERROR;
  const std::string text = old;
  const size_t allocations =
      Language::interfacePoolCount + (Language::poolCount > Language::interfacePoolCount ? 1u : 0u);
  for (size_t fault = 0; fault < allocations; ++fault) {
    failArrayAfter = long(fault);
    CHECK(!Language::select("de", inflate, persist));
    failArrayAfter = -1;
    CHECK(std::string(Language::currentCode()) == Language::defaultCode);
    CHECK(D_BOOT_STORAGE_ERROR == old);
    CHECK(std::string(old) == text);
    CHECK_EQ(persistCalls, 0u);
  }
  REQUIRE(Language::select("de", inflate, persist));
  CHECK_EQ(persistCalls, 1u);
}
TEST_CASE("Runtime decode and persistence failures never commit partial dictionaries") {
  reset();
  const auto oldCode = std::string(Language::currentCode());
  const char *old = D_BOOT_STORAGE_ERROR;
  for (size_t pool = 0; pool < Language::poolCount; ++pool) {
    failDecodeAfter = int(pool);
    persistCalls = 0;
    CHECK(!Language::select("de", inflate, persist));
    CHECK(D_BOOT_STORAGE_ERROR == old);
    CHECK(std::string(Language::currentCode()) == oldCode);
    CHECK_EQ(persistCalls, 0u);
  }
  failDecodeAfter = -1;
  persistResult = false;
  CHECK(!Language::select("de", inflate, persist));
  CHECK(D_BOOT_STORAGE_ERROR == old);
  CHECK(std::string(Language::currentCode()) == oldCode);
  CHECK_EQ(persistCalls, 1u);
  CHECK(!Language::select(nullptr, inflate));
  CHECK(!Language::select("xx", inflate));
  CHECK(!Language::select("de", nullptr));
  persistResult = true;
  REQUIRE(Language::select("de", inflate, persist));
}
TEST_CASE("Lazy browser pools retain earlier strings within the same language") {
  reset();
  REQUIRE(Language::select("uk", inflate));
  const char *old = D_WEB_UPLOAD_BOOK_DESC;
  const std::string copy = old;
  for (size_t key = 0; key < Language::keyCount; ++key)
    REQUIRE(*Language::text(static_cast<Language::Key>(key)));
  CHECK(old == D_WEB_UPLOAD_BOOK_DESC);
  CHECK(std::string(old) == copy);
  const auto calls = decodeCalls;
  for (size_t key = 0; key < Language::keyCount; ++key)
    (void)Language::text(static_cast<Language::Key>(key));
  CHECK_EQ(decodeCalls, calls);
}
TEST_CASE("Catalog inflater rejects corruption truncation trailing input and output overrun") {
  const std::string text(1800, 'a');
  uLongf size = compressBound(text.size());
  std::vector<uint8_t> packed(size);
  REQUIRE(compress2(packed.data(), &size, reinterpret_cast<const uint8_t *>(text.data()),
                    text.size(), 9) == Z_OK);
  packed.resize(size);
  std::vector<uint8_t> output(text.size() + 1);
  const size_t capacity = text.size();
  REQUIRE(decoder.inflate(packed.data(), packed.size(), output.data(), capacity));
  CHECK(std::string(output.begin(), output.begin() + capacity) == text);
  for (size_t length = 0; length < packed.size(); ++length)
    CHECK(!decoder.inflate(packed.data(), length, output.data(), capacity));
  for (size_t byte = 0; byte < packed.size(); ++byte) {
    auto corrupt = packed;
    corrupt[byte] ^= 0x40;
    const bool accepted = decoder.inflate(corrupt.data(), corrupt.size(), output.data(), capacity);
    // Changing unused DEFLATE padding may leave a valid identical stream.
    if (accepted)
      CHECK(std::string(output.begin(), output.begin() + capacity) == text);
  }
  CHECK(!decoder.inflate(packed.data(), packed.size(), output.data(), capacity - 1));
  CHECK(!decoder.inflate(packed.data(), packed.size(), output.data(), capacity + 1));
  auto trailing = packed;
  trailing.push_back(0);
  CHECK(!decoder.inflate(trailing.data(), trailing.size(), output.data(), capacity));
  CHECK(!decoder.inflate(nullptr, packed.size(), output.data(), capacity));
}
