#include "test_framework.h"
#include "lang_config.h"

#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static std::set<std::string> extractKeys(const fs::path& path) {
  std::set<std::string> keys;
  std::ifstream f(path);
  if (!f.is_open()) return keys;

  std::string line;
  while (std::getline(f, line)) {
    // Match "#define" followed by whitespace and "D_".
    if (line.size() < 10) continue;
    if (line.compare(0, 7, "#define") != 0) continue;
    size_t pos = 7;
    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
    if (pos >= line.size() || line.compare(pos, 2, "D_") != 0) continue;
    size_t keyStart = pos;
    size_t keyEnd = line.find_first_of(" \t", keyStart);
    if (keyEnd == std::string::npos) keyEnd = line.size();
    keys.insert(line.substr(keyStart, keyEnd - keyStart));
  }
  return keys;
}

static std::vector<fs::path> findLangFiles() {
  std::vector<fs::path> files;
  for (auto& entry : fs::directory_iterator(LANG_DIR)) {
    if (!entry.is_regular_file()) continue;
    auto name = entry.path().filename().string();
    if (name == "lang.h") continue;
    if (entry.path().extension() == ".h") files.push_back(entry.path());
  }
  std::sort(files.begin(), files.end());
  return files;
}

TEST_CASE("lang: all language files define the same D_* keys as en.h") {
  fs::path enPath = fs::path(LANG_DIR) / "en.h";
  auto en = extractKeys(enPath);
  REQUIRE(!en.empty());

  auto langFiles = findLangFiles();
  for (auto& langFile : langFiles) {
    if (langFile.filename() == "en.h") continue;
    auto label = langFile.filename().string();
    auto keys = extractKeys(langFile);
    REQUIRE(!keys.empty());

    std::vector<std::string> missing;
    for (auto& k : en) {
      if (keys.find(k) == keys.end()) missing.push_back(k);
    }

    std::vector<std::string> extra;
    for (auto& k : keys) {
      if (en.find(k) == en.end()) extra.push_back(k);
    }

    if (!missing.empty()) {
      std::fprintf(stderr, "  Keys in en.h but missing from %s:\n", label.c_str());
      for (auto& k : missing)
        std::fprintf(stderr, "    %s\n", k.c_str());
    }
    if (!extra.empty()) {
      std::fprintf(stderr, "  Keys in %s but missing from en.h:\n", label.c_str());
      for (auto& k : extra)
        std::fprintf(stderr, "    %s\n", k.c_str());
    }

    CHECK(missing.empty());
    CHECK(extra.empty());
  }
}

TEST_CASE("lang: no D_* key has an empty or whitespace-only value") {
  auto checkFile = [](const fs::path& path) {
    auto label = path.filename().string();
    std::ifstream f(path);
    REQUIRE(f.is_open());

    std::string line;
    int bad = 0;
    while (std::getline(f, line)) {
      if (line.size() < 10) continue;
      if (line.compare(0, 7, "#define") != 0) continue;
      size_t pos = 7;
      while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
      if (pos >= line.size() || line.compare(pos, 2, "D_") != 0) continue;
      size_t keyStart = pos;
      size_t keyEnd = line.find_first_of(" \t", keyStart);
      if (keyEnd == std::string::npos) continue;

      size_t valStart = line.find_first_not_of(" \t", keyEnd);
      if (valStart == std::string::npos) continue;
      if (line[valStart] != '"') continue;

      // Scan the quoted string for any non-whitespace content.
      bool hasContent = false;
      for (size_t i = valStart + 1; i < line.size() && line[i] != '"'; ++i) {
        if (line[i] != ' ' && line[i] != '\t') { hasContent = true; break; }
      }
      if (!hasContent) {
        std::fprintf(stderr, "  Empty/whitespace-only value in %s: %s\n",
                     label.c_str(), line.substr(keyStart, keyEnd - keyStart).c_str());
        ++bad;
      }
    }
    CHECK(bad == 0);
  };

  auto langFiles = findLangFiles();
  for (auto& langFile : langFiles)
    checkFile(langFile);
}
