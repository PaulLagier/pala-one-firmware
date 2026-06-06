#include "src/hal/app_loader.h"

#include <esp_heap_caps.h>
#include <soc/soc.h>           // MAP_IRAM_TO_DRAM, SOC_I_D_OFFSET

#include "src/config.h"            // MAX_APP_BINARY
#include "src/hal/input.h"         // resetInputFrontend
#include "pala_app.h"          // PalaAppHeader, PALA_APP_MAGIC, pala_app_entry_t
#include "src/pure/app_header.h"   // validateAppHeader, validateRelocEntries
#include "src/state.h"             // FS macro

// File-local working buffer. Held only between malloc and free inside
// loadAndRunApp; not externally visible. Reset to nullptr on the way out
// of every code path so an OOM on a later call doesn't leak.
static void* s_appExecBuf  = nullptr;
static size_t s_appExecSize = 0;

static void freeExecBuf() {
  if (s_appExecBuf) {
    heap_caps_free(s_appExecBuf);
    s_appExecBuf  = nullptr;
    s_appExecSize = 0;
  }
}

// Translate a pure/app_header status into a LoadResult, threading
// `outFileApiVersion` through on ApiMismatch.
static LoadResult fromHeaderStatus(AppHeaderStatus s) {
  switch (s) {
    case AppHeaderStatus::Ok:             return LoadResult::Ok;
    case AppHeaderStatus::TooSmall:       return LoadResult::FileTooSmall;
    case AppHeaderStatus::BadMagic:       return LoadResult::BadMagic;
    case AppHeaderStatus::ApiMismatch:    return LoadResult::ApiMismatch;
    case AppHeaderStatus::BadEntryOffset: return LoadResult::BadEntryOffset;
    case AppHeaderStatus::BadRelocTable:  return LoadResult::BadRelocTable;
  }
  return LoadResult::BadEntryOffset;  // unreachable; keeps the compiler quiet
}

LoadResult loadAndRunApp(const char* path, const PalaAPI* api,
                         uint32_t* outFileApiVersion) {
  freeExecBuf();  // defensive — a prior bail-out should have done this already

  File f = FS.open(path, "r");
  if (!f) return LoadResult::FileNotFound;

  size_t fileSize = f.size();
  if (fileSize < sizeof(PalaAppHeader) + 4) { f.close(); return LoadResult::FileTooSmall; }
  if (fileSize > MAX_APP_BINARY)             { f.close(); return LoadResult::FileTooLarge; }

  // Exec-capable, 32-bit-aligned. On ESP32-S3 this returns an
  // instruction-bus address (0x403xxxxx). That address is NOT writable
  // from the data bus — we have to access the same physical SRAM through
  // its DRAM alias for the read + relocation passes below.
  s_appExecBuf = heap_caps_malloc(fileSize, MALLOC_CAP_EXEC | MALLOC_CAP_32BIT);
  if (!s_appExecBuf) { f.close(); return LoadResult::OutOfMemory; }
  s_appExecSize = fileSize;

  uint8_t* dataBuf = reinterpret_cast<uint8_t*>(MAP_IRAM_TO_DRAM(reinterpret_cast<uint32_t>(s_appExecBuf)));

  size_t bytesRead = f.read(dataBuf, fileSize);
  f.close();
  if (bytesRead != fileSize) { freeExecBuf(); return LoadResult::ReadError; }

  // Header validation — pure, host-testable. Returns specific failure
  // codes so we can give the caller a useful error string.
  AppHeaderStatus hs = validateAppHeader(dataBuf, fileSize, outFileApiVersion);
  if (hs != AppHeaderStatus::Ok) { freeExecBuf(); return fromHeaderStatus(hs); }

  if (!validateRelocEntries(dataBuf, fileSize)) {
    freeExecBuf();
    return LoadResult::BadRelocEntry;
  }

  // Header is valid; perform the relocation pass.
  //
  // For each entry off in the reloc table, the 32-bit word at dataBuf+off
  // is a file-relative address that needs to be biased into the run-time
  // memory location of the buffer. Add `base` (the DRAM view's address)
  // so the patched values resolve via the data bus — string literals,
  // .rodata pointers, etc. are all data-bus reads. l32r literal-pool
  // accesses go through the instruction bus but reach the same physical
  // SRAM, so adding the DRAM base is correct for both consumers.
  PalaAppHeader hdr;
  memcpy(&hdr, dataBuf, sizeof(hdr));
  uint32_t base = reinterpret_cast<uint32_t>(dataBuf);
  if (hdr.reloc_count > 0) {
    const uint32_t* relocs = reinterpret_cast<const uint32_t*>(dataBuf + hdr.reloc_offset);
    for (uint32_t i = 0; i < hdr.reloc_count; i++) {
      uint32_t off = relocs[i];
      *reinterpret_cast<uint32_t*>(dataBuf + off) += base;
    }
  }

  // The entry pointer must use the instruction-bus address (the exec
  // buffer we got from heap_caps_malloc); call-and-return goes through
  // the I-bus, so we hand the app the original `s_appExecBuf` cast.
  pala_app_entry_t entry =
      reinterpret_cast<pala_app_entry_t>(reinterpret_cast<uint8_t*>(s_appExecBuf) + hdr.entry_offset);

  // Bracket the app call with input-frontend resets so the click that
  // launched the app doesn't leak into the app's first frame, and any
  // press during the app doesn't leak back into the screen we return to.
  resetInputFrontend();
  entry(api);
  resetInputFrontend();

  freeExecBuf();
  return LoadResult::Ok;
}
