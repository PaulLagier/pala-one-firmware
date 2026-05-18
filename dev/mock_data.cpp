#include "src/storage/library.h"
#include "src/storage/app_catalog.h"
#include "src/storage/list_items.h"
#include "stubs/fs_stub.h"

// loadListItems() is declared inside #ifdef ARDUINO in list_items.h;
// the host implementation lives in stubs/list_items_glue.cpp.
extern void loadListItems();

void loadMockData() {
  // Prefer an environment variable override; default to dev/fs relative to CWD.
  // Run the binary from the repo root: ./dev/build/pala_web_emu
  const char* env = std::getenv("PALA_EMU_FS_ROOT");
  std::string root = (env && env[0]) ? env : "dev/fs";
  std::printf("FS root: %s\n", root.c_str());

  LittleFS.setRoot(root);

  // Scan the real filesystem so book/folder lists reflect what's on disk.
  loadBooks();
  loadApps();
  loadListItems();
}
