#include "src/hal/diagnostics.h"
#include "src/hal/ipc_stack.h"
#include "src/ui/screens/upload_screen.h"
#include "src/hal/companion_ble.h"
#include <WiFi.h>
#if PALA_DIAGNOSTICS && defined(PALA_COMPANION_TEST)
void packedBookDeviceTest();
void companionUploadDeviceTest();
void companionLanguageDeviceTest();
void readerWindowDeviceTest();
void readerIndexCancelDeviceTest();
void readerTestCleanup();
void companionResumeTestBegin();
void companionResumeTestCheck(bool cancel);
#endif
#if PALA_DIAGNOSTICS
#include "src/hal/display.h"
#include "src/hal/wifi_provisioning.h"
#include "src/ui/screen.h"
#include "src/ui/lock.h"
#include "src/ui/reader.h"
#include <esp_heap_caps.h>
#include "src/storage/library.h"
#include "src/storage/fs_util.h"
#include "src/storage/statistics.h"
#include "src/ui/font.h"
#include "src/hal/file_stream.h"
#include "src/hal/cpu_burst.h"
#include "src/ui/text.h"
#include "src/ui/sleep.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/screens/statistics_screen.h"
#include "src/ui/screens/settings_screen.h"
#include "src/ui/screens/update_screen.h"
#include "src/ui/screens/about_screen.h"
#include "src/ui/screens/storage_recovery_screen.h"
#include "src/ui/reader_actions.h"
#include "src/ui/screens/reader_screen.h"
#include "src/ui/reader_menu.h"
#include "src/ui/statusbar.h"
#include "src/ui/toast.h"
#include "src/ui/widgets.h"
#include "src/storage/book_metadata.h"
#include "src/storage/preferences_store.h"
#include "src/ui/screens/bookmarks/book_select_screen.h"
#include "src/ui/screens/bookmarks/session.h"

namespace {
uint32_t s_lastDiagnosticMs = 0;
// Exact old byte adapter, retained ONLY for a controlled read-only benchmark.
class LegacyStream : public IReadStream {
public:
  explicit LegacyStream(File& f) : f_(f) {}
  int read() override { if (!f_ || !f_.available()) return -1; return f_.read(); }
  bool seek(uint32_t p) override { return f_.seek(p); }
  uint32_t position() override { return f_.position(); }
  size_t size() override { return f_.size(); }
  bool available() override { return f_.available() > 0; }
private: File& f_;
};
#ifdef PALA_COMPANION_TEST
bool connectionHttpProbe(){
  WiFiClient client;client.setTimeout(1500);
  const IPAddress address=WiFi.getMode()==WIFI_AP?WiFi.softAPIP():WiFi.localIP();
  if(!client.connect(address,80,1500) && !client.connect(IPAddress(127,0,0,1),80,1500)){
    Serial.println("[http-probe] connect=0");return false;
  }
  // Deliberately invalid ID: exercises a real browser route without reading a book.
  client.print("GET /readbook-text?id=-1 HTTP/1.1\r\nHost: pala.local\r\nConnection: close\r\n\r\n");
  const uint32_t started=millis();
  while(!client.available() && uint32_t(millis()-started)<2500){server.handleClient();delay(1);}
  const String status=client.readStringUntil('\n');client.stop();
  Serial.printf("[http-probe] connected=1 bytes=%u status400=%d\n",status.length(),status.startsWith("HTTP/1.1 400") || status.startsWith("HTTP/1.0 400"));
  return status.startsWith("HTTP/1.1 400") || status.startsWith("HTTP/1.0 400");
}
void connectionTest(){
  if(g_bookview.book.isOpen() || companionBleConnected()){Serial.println("[connect-test] busy");return;}
  auto& screen=g_uploadScreen;bool chooser=true,bluetooth=true,browser=true,http=true,exclusive=true,exit=true;
  for(int round=0;round<2;++round){
    screen.nextScreen=nullptr;screen.onEnter();chooser=chooser&&screen.diagnosticChoosing();
    screen.onButton(ButtonEvent{ButtonEvent::Long});screen.onIdleTick();
    bluetooth=bluetooth&&companionBleRunning();exclusive=exclusive&&WiFi.getMode()==WIFI_OFF;
    screen.onButton(ButtonEvent{ButtonEvent::Long});exit=exit&&screen.nextScreen==&g_libraryScreen&&!companionBleRunning();
    screen.nextScreen=nullptr;screen.onEnter();screen.onButton(ButtonEvent{ButtonEvent::Short});screen.onButton(ButtonEvent{ButtonEvent::Long});screen.onIdleTick();
    const uint32_t started=millis();
    while(!screen.diagnosticBrowserReady()&&uint32_t(millis()-started)<8000){screen.onIdleTick();delay(10);}
    browser=browser&&screen.diagnosticBrowserReady()&&WiFi.getMode()!=WIFI_OFF;
    if(screen.diagnosticBrowserReady())http=http&&connectionHttpProbe();else http=false;
    exclusive=exclusive&&!companionBleRunning();
    screen.onButton(ButtonEvent{ButtonEvent::Triple});exit=exit&&screen.nextScreen==&g_libraryScreen&&WiFi.getMode()==WIFI_OFF;
    Serial.printf("[ipc-stack] round=%d ipc0_bytes=%lu ipc1_bytes=%lu ipc0_free=%lu ipc1_free=%lu\n",round,(unsigned long)RuntimeStack::ipcBytes(0),(unsigned long)RuntimeStack::ipcBytes(1),(unsigned long)RuntimeStack::ipcFreeBytes(0),(unsigned long)RuntimeStack::ipcFreeBytes(1));
  }
  screen.nextScreen=nullptr;g_currentScreen=&g_libraryScreen;g_libraryScreen.onEnter();
  Serial.printf("[connect-test] chooser=%d bluetooth=%d browser=%d http=%d exclusive=%d exit=%d rounds=2\n",chooser,bluetooth,browser,http,exclusive,exit);
}
#endif
void benchmark() {
  const char* path = bookPath(0);
  static const char* fixture = "/.pala_bench_local.tmp";
  bool temporary = false;
  if (!path) {
    if (FS.exists(fixture) || fsFreeBytesSafe() < 65536) {
      Serial.println("[bench] fixture unavailable"); return;
    }
    File out = FS.open(fixture, "w");
    if (!out) { Serial.println("[bench] fixture open failed"); return; }
    const char sample[] =
      "Pala One reads books. Klarer Text, ruhige Seiten und eine flüssige Bedienung.\n"
      "Words wrap at the same position. café, Größe, mañana, punctuation; and long URLs.\n";
    bool ok = true;
    for (int i = 0; i < 256 && ok; ++i)
      ok = out.write(reinterpret_cast<const uint8_t*>(sample), sizeof(sample) - 1) == sizeof(sample) - 1;
    out.close();
    if (!ok) { FS.remove(fixture); Serial.println("[bench] fixture write failed"); return; }
    path = fixture; temporary = true;
  }
  File file = FS.open(path, "r");
  if (!file) {
    if (temporary) FS.remove(fixture);
    Serial.println("[bench] open failed"); return;
  }
  uint32_t previousCpu = getCpuFrequencyMhz();
  for (int mode = 0; mode < 3; ++mode) {
    setCpuFrequencyMhz(mode == 2 ? 240 : 80);
    LegacyStream legacy(file); FileReadStream buffered(file);
    IReadStream& stream = mode == 0 ? static_cast<IReadStream&>(legacy)
                                   : static_cast<IReadStream&>(buffered);
    Font::useBody(); const auto& metrics = Font::bodyLayout();
    uint32_t pos = 0, hash = 2166136261UL;
    int pages = 0; const uint32_t started = micros();
    while (pos < file.size() && pages < 24) {
      uint32_t next = paginatePage(stream, pos, metrics,
        [](const char* text) { return Font::measureBionicLine(text); }, nullptr);
      if (next <= pos) break;
      pos = next; hash = (hash ^ pos) * 16777619UL; ++pages;
    }
    Serial.printf("[bench] mode=%s cpu=%d pages=%d us=%lu offsets_hash=%08lx\n",
      mode == 0 ? "legacy" : "buffered", mode == 2 ? 240 : 80, pages,
      (unsigned long)(micros() - started), (unsigned long)hash);
  }
  setCpuFrequencyMhz(previousCpu);
  file.close();
  if (temporary) FS.remove(fixture);
  Serial.println("[bench] done");
}
struct RecoveryMockState {int retries=0,erases=0,restarts=0;bool retryResult=false,eraseResult=false;};
RecoveryMockState s_recoveryMock;
bool recoveryRetryMock(){++s_recoveryMock.retries;return s_recoveryMock.retryResult;}
bool recoveryEraseMock(){++s_recoveryMock.erases;return s_recoveryMock.eraseResult;}
void recoveryRestartMock(){++s_recoveryMock.restarts;}
// UI simulation only. Never call the real erase backend on a healthy reader.
void storageRecoveryTest(){
  if(g_currentScreen!=&g_libraryScreen||g_bookview.book.isOpen()||Lock::isLocked()||companionBleRunning()){
    Serial.println("[recovery-test] busy");return;
  }
  const auto before=Statistics::snapshot();const auto total=fsTotalBytesSafe(),used=fsUsedBytesSafe();
  const int books=g_library.bookCount;const bool locked=Lock::isLocked();
  StorageRecoveryScreen screen({recoveryRetryMock,recoveryEraseMock,recoveryRestartMock});
  auto tap=[&](ButtonEvent::Kind kind){screen.onButton({kind});};
  auto shortN=[&](int n){while(n--)tap(ButtonEvent::Short);};
  auto fresh=[&](){s_recoveryMock={};screen.onEnter();};
  bool ok=true;
  fresh();tap(ButtonEvent::Double);ok=ok&&s_recoveryMock.retries==1&&!s_recoveryMock.erases&&!s_recoveryMock.restarts;
  tap(ButtonEvent::Double);tap(ButtonEvent::Short);tap(ButtonEvent::Long);shortN(2);tap(ButtonEvent::Double);
  ok=ok&&!s_recoveryMock.erases; // Help and its visible Back.
  fresh();shortN(2);tap(ButtonEvent::Double);tap(ButtonEvent::Double);ok=ok&&!s_recoveryMock.erases; // Default Cancel.
  fresh();shortN(2);tap(ButtonEvent::Long);tap(ButtonEvent::Short);tap(ButtonEvent::Double);tap(ButtonEvent::Double);ok=ok&&!s_recoveryMock.erases; // Final Cancel.
  fresh();shortN(2);tap(ButtonEvent::Long);tap(ButtonEvent::Short);tap(ButtonEvent::Double);tap(ButtonEvent::Short);
  tap(ButtonEvent::Double);tap(ButtonEvent::VeryLong);tap(ButtonEvent::ClickHold);ok=ok&&!s_recoveryMock.erases;
  tap(ButtonEvent::Long);ok=ok&&s_recoveryMock.erases==1&&!s_recoveryMock.restarts;tap(ButtonEvent::Double);ok=ok&&s_recoveryMock.erases==1;
  fresh();shortN(2);tap(ButtonEvent::Long);tap(ButtonEvent::Short);tap(ButtonEvent::Long);tap(ButtonEvent::Short);
  s_recoveryMock.eraseResult=true;tap(ButtonEvent::Long);tap(ButtonEvent::Long);ok=ok&&s_recoveryMock.erases==1&&s_recoveryMock.restarts==1;
  fresh();s_recoveryMock.retryResult=true;tap(ButtonEvent::Long);ok=ok&&s_recoveryMock.retries==1&&s_recoveryMock.restarts==1&&!s_recoveryMock.erases;
  const auto after=Statistics::snapshot();const bool counters=before.pagesRead==after.pagesRead&&before.buttonPresses==after.buttonPresses&&before.totalSessions==after.totalSessions;
  const bool storage=total==fsTotalBytesSafe()&&used==fsUsedBytesSafe()&&books==g_library.bookCount&&locked==Lock::isLocked();
  resetInputFrontend();forceNextMenuFrameFull();g_libraryScreen.onEnter();
  Serial.printf("[recovery-test] simulated=1 confirmation=%d storage=%d counters=%d pass=%d\n",ok,storage,counters,ok&&storage&&counters);
}

// Read-only visual check: leave the corrected Sleep page visible for the user.
// Exercises real screen actions and NVS owners, always restoring original values.
void statisticsTest() {
  if(g_currentScreen!=&g_libraryScreen||g_bookview.book.isOpen()||Lock::isLocked()||companionBleRunning()){
    Serial.println("[statistics-test] busy");return;
  }
  const auto before=Statistics::snapshot();
  g_statsScreen.onEnter();
  g_statsScreen.onButton({ButtonEvent::Long});
  g_statsScreen.onButton({ButtonEvent::Short});
  g_statsScreen.onButton({ButtonEvent::Double});
  g_statsScreen.onButton({ButtonEvent::Short});
  g_statsScreen.onButton({ButtonEvent::Long});
  bool returned=g_statsScreen.nextScreen==&g_libraryScreen;
  g_statsScreen.onEnter();
  g_statsScreen.onButton({ButtonEvent::Short});
  g_statsScreen.onButton({ButtonEvent::Double});
  g_statsScreen.onButton({ButtonEvent::Long});
  g_statsScreen.onButton({ButtonEvent::Triple});
  returned=returned&&g_statsScreen.nextScreen==&g_libraryScreen;
  const auto after=Statistics::snapshot();
  const bool counters=before.pagesRead==after.pagesRead&&before.buttonPresses==after.buttonPresses&&before.totalSessions==after.totalSessions&&before.currentStreak==after.currentStreak&&before.longestStreak==after.longestStreak&&before.bitmap==after.bitmap;
  g_statsScreen.nextScreen=nullptr;g_libraryScreen.onEnter();
  Serial.printf("[statistics-test] return=%d counters=%d pass=%d\n",returned,counters,returned&&counters);
}

void navigationTest() {
  if (g_currentScreen != &g_libraryScreen || g_bookview.book.isOpen() || Lock::isLocked()) {
    Serial.println("[nav-test] unavailable: return to unlocked library"); return;
  }
  if (g_library.bookCount) {
    Serial.println("[nav-test] unavailable: empty catalog required"); return;
  }
  const auto before = Statistics::snapshot();
  bool ok = true;
  g_bmBookSelectScreen.nextScreen = nullptr;
  g_bmBookSelectScreen.onEnter();
  g_bmBookSelectScreen.onButton(ButtonEvent{ButtonEvent::Short});
  ok = ok && g_bookmarkSession.bookIndex == 0 && g_bmBookSelectScreen.nextScreen == nullptr;
  g_bmBookSelectScreen.onButton(ButtonEvent{ButtonEvent::Long});
  ok = ok && g_bmBookSelectScreen.nextScreen == &g_libraryScreen;
  g_bmBookSelectScreen.nextScreen = nullptr;
  g_bmBookSelectScreen.onButton(ButtonEvent{ButtonEvent::Double});
  ok = ok && g_bmBookSelectScreen.nextScreen == &g_libraryScreen;
  g_bmBookSelectScreen.nextScreen = nullptr;
  const auto after = Statistics::snapshot();
  ok = ok && before.pagesRead == after.pagesRead && before.buttonPresses == after.buttonPresses;
  g_libraryScreen.onEnter();
  Serial.printf("[nav-test] empty_apps_bookmarks pass=%d statistics_preserved=%d\n", ok,
    before.pagesRead == after.pagesRead && before.buttonPresses == after.buttonPresses);
}

void readerTest() {
  if (g_currentScreen != &g_libraryScreen || g_bookview.book.isOpen()) {
    Serial.println("[reader-test] unavailable: return to library first"); return;
  }
  const StatisticsSnapshot statsBefore = Statistics::snapshot();
  const char* path = "/.pala_reader_local.tmp";
  if (FS.exists(path) || fsFreeBytesSafe() < 32768) {
    Serial.println("[reader-test] unavailable: fixture"); return;
  }
  File fixture = FS.open(path, "w");
  if (!fixture) { Serial.println("[reader-test] fixture open failed"); return; }
  const char sample[] = "Pala One reader test. café, Größe, mañana.\n"
    "The next page must preserve every line, word, accent and page boundary.\n"
    "https://example.invalid/abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789\n";
  bool ok = true;
  for (int i = 0; i < 8 && ok; ++i)
    ok = fixture.write(reinterpret_cast<const uint8_t*>(sample), sizeof(sample) - 1) == sizeof(sample) - 1;
  fixture.close();
  File file = FS.open(path, "r");
  ok = ok && (bool)file;
  unsigned totalPages = 0;
  for (int family = 0; family < 2 && ok; ++family) {
    for (int size : {8, 10, 12, 14}) {
      for (int bionic = 0; bionic < 2 && ok; ++bionic) {
        CpuBurst cpu;
        Font::setDiagnosticLayout(size, (Font::Family)family, 0, bionic != 0);
        uint32_t pos = 0; unsigned pages = 0;
        while (ok && pos < file.size() && pages < 128) {
          clearPagePrefetch(); beginPageCanvas();
          const uint32_t uncached = drawPageAt(file, pos);
          const uint32_t hash = gfx.captureHash();
          ok = !lastDrawUsedPrefetch();
          prefetchPage(file, pos); beginPageCanvas();
          const uint32_t cached = drawPageAt(file, pos);
          const uint32_t cachedHash = gfx.captureHash();
          ok = ok && lastDrawUsedPrefetch() && uncached == cached
               && cachedHash == hash && cached > pos && cached <= file.size();
          if (!ok) Serial.printf("[reader-test] mismatch start=%lu direct_next=%lu cached_next=%lu direct_hash=%08lx cached_hash=%08lx cache_hit=%d\n",
            (unsigned long)pos, (unsigned long)uncached, (unsigned long)cached,
            (unsigned long)hash, (unsigned long)cachedHash, lastDrawUsedPrefetch());
          pos = cached; ++pages; ++totalPages;
          delay(1); // let the RTOS run during the multi-layout diagnostic
        }
        ok = ok && pos == file.size();
        cpu.restore();
        Serial.printf("[reader-test] family=%d size=%d bionic=%d pages=%u matched=%d\n",
                      family, size, bionic, pages, ok);
      }
    }
  }
  // A ready cache must be rejected when any typography field changes.
  for (int changed = 0; changed < 4 && ok; ++changed) {
    Font::setDiagnosticLayout(10, Font::Family::Helvetica, 0, false);
    clearPagePrefetch(); prefetchPage(file, 0);
    Font::setDiagnosticLayout(changed == 0 ? 12 : 10,
      changed == 1 ? Font::Family::OpenDyslexic : Font::Family::Helvetica,
      changed == 2 ? 4 : 0, changed == 3);
    beginPageCanvas(); drawPageAt(file, 0);
    ok = !lastDrawUsedPrefetch();
  }
  file.close(); clearPagePrefetch(); Font::loadSettings();
  resetBookView();
  if (ok && g_bookview.book.open(path)) {
    g_bookview.pages.reset();
    forceNextRenderFull(); renderCurrentPage();
    ok = g_bookview.pages.count > 1;
    if (ok) {
      prefetchNextReaderPage(); g_bookview.cursor.pageIndex = 1;
      renderCurrentPage(); ok = lastDrawUsedPrefetch();
    }
    if (ok) {
      const int last = findPageForOffset(g_bookview.book.size() - 1);
      g_bookview.cursor.pageIndex = last; renderCurrentPage();
      // Prove the no-op EOF branch before calling it: no diagnostic page
      // turns are recorded in the user's lifetime reading statistics.
      ok = g_bookview.pages.eofReached && last + 1 == g_bookview.pages.endPage()
           && g_bookview.pages.offsetAt(last) < g_bookview.book.size();
      if (ok) ok = !advancePage() && g_bookview.cursor.pageIndex == last;
    }
  } else ok = false;
  resetBookView(); Font::loadSettings();
  const bool removed = FS.remove(path);
  const StatisticsSnapshot statsAfter = Statistics::snapshot();
  const bool statsPreserved = statsBefore.pagesRead == statsAfter.pagesRead
    && statsBefore.buttonPresses == statsAfter.buttonPresses
    && statsBefore.totalSessions == statsAfter.totalSessions
    && statsBefore.currentStreak == statsAfter.currentStreak;
  ok = ok && statsPreserved;
  Serial.printf("[reader-test] statistics_preserved=%d\n", statsPreserved);
  g_libraryScreen.draw();
  Serial.printf("[reader-test] done pass=%d pages=%u cleaned=%d cpu=%lu\n",
                ok && removed, totalPages, removed, (unsigned long)getCpuFrequencyMhz());
}
} // namespace

// Development-only protocol, deliberately disjoint from Improv's 'I' prefix.
// No settings, credentials or raw flash commands. Benchmark uses a temporary
// fixture outside the library only if no user book exists, then removes it.
bool Diagnostics::isActive() {
  return s_lastDiagnosticMs != 0 && (uint32_t)(millis() - s_lastDiagnosticMs) < 30000;
}

bool Diagnostics::poll() {
  static char command[32];
  static size_t used = 0;
  static bool receiving = false;
  static uint32_t started = 0;
  if (receiving && millis() - started > 1000) { receiving = false; used = 0; }
  if (!receiving && WifiProvisioning::serialFrameInProgress()) return false;
  if (!receiving && (!Serial.available() || Serial.peek() != '!')) return false;
  if (!receiving) { receiving = true; started = millis(); }
  s_lastDiagnosticMs = millis();
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (used < sizeof(command) - 1) command[used++] = c;
      continue;
    }
    command[used] = 0; used = 0; receiving = false;
    markUserActivity();
    if (strcmp(command, "!info") == 0) {
      Serial.printf("[info] firmware=%s board=v1_2 cpu=%lu heap=%lu largest=%lu stack=%lu book=%d page=%d locked=%d lock_on_sleep=%d sleep_s=%d no_screensaver=%d ipc0_bytes=%lu ipc1_bytes=%lu ipc0_free=%lu ipc1_free=%lu\n",
        FW_VERSION, (unsigned long)getCpuFrequencyMhz(),
        (unsigned long)ESP.getFreeHeap(),
        (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
        (unsigned long)uxTaskGetStackHighWaterMark(nullptr),
        g_bookview.book.isOpen(), g_bookview.cursor.pageIndex, Lock::isLocked(),
        Sleep::lockOnSleep(), Sleep::idleTimeoutSecs(), Sleep::noScreensaver(),
        (unsigned long)RuntimeStack::ipcBytes(0),(unsigned long)RuntimeStack::ipcBytes(1),(unsigned long)RuntimeStack::ipcFreeBytes(0),(unsigned long)RuntimeStack::ipcFreeBytes(1));
    } else if (strcmp(command, "!snapshot") == 0) {
      gfx.dumpCapture();
    } else if (strcmp(command, "!bench") == 0) {
      benchmark();
#ifdef PALA_COMPANION_TEST
    } else if (strcmp(command, "!connecttest") == 0) {
      connectionTest();
    } else if (strcmp(command, "!resumetest") == 0) {
      companionResumeTestBegin();
    } else if (strcmp(command, "!resumecheck") == 0 || strcmp(command, "!resumecancel") == 0) {
      companionResumeTestCheck(strcmp(command, "!resumecancel") == 0);
    } else if (strcmp(command, "!testcleanup") == 0) {
      readerTestCleanup();
    } else if (strcmp(command, "!indexcanceltest") == 0) {
      readerIndexCancelDeviceTest();
    } else if (strcmp(command, "!windowtest") == 0) {
      readerWindowDeviceTest();
    } else if (strcmp(command, "!languagetest") == 0) {
      companionLanguageDeviceTest();
    } else if (strcmp(command, "!uploadtest") == 0) {
      companionUploadDeviceTest();
    } else if (strcmp(command, "!packedtest") == 0) {
      packedBookDeviceTest();
#endif
    } else if (strcmp(command, "!readermenutest") == 0) {
      Serial.println("[ui-test] run scripts/test_major_update.py; use snapshot for physical display checks");
    } else if (strcmp(command, "!recoverytest") == 0) {
      storageRecoveryTest();
    } else if (strcmp(command, "!settingsrows") == 0) {
      Serial.println("[ui-test] run scripts/test_major_update.py; use snapshot for physical display checks");
    } else if (strcmp(command, "!settingstest") == 0) {
      Serial.println("[ui-test] run scripts/test_major_update.py; use snapshot for physical display checks");
    } else if (strcmp(command, "!statstest") == 0) {
      statisticsTest();
    } else if (strcmp(command, "!navtest") == 0) {
      navigationTest();
    } else if (strcmp(command, "!readertest") == 0) {
      readerTest();
    } else if (strcmp(command, "!sleep") == 0) {
      Serial.println("[sleep-test] entering normal device sleep"); Serial.flush(); Sleep::enter();
    } else if (strcmp(command, "!redraw") == 0 || strcmp(command, "!redrawfast") == 0 || strcmp(command, "!redrawlegacy") == 0 || strcmp(command, "!redrawfull") == 0) {
      if (!Lock::isLocked()) {
        const bool full = strcmp(command, "!redrawfull") == 0;
        if(full){forceNextMenuFrameFull();forceNextRenderFull();}
        const bool fast = strcmp(command, "!redrawfast") == 0;
        const bool legacy = strcmp(command, "!redrawlegacy") == 0;
        forceDisplayModeSetupForTest(legacy);
        const uint32_t previousCpu = getCpuFrequencyMhz();
        if (fast) setCpuFrequencyMhz(240);
        const uint32_t started = micros();
        g_currentScreen->draw();
        const uint32_t elapsed = micros() - started;
        forceDisplayModeSetupForTest(false);
        setCpuFrequencyMhz(previousCpu);
        Serial.printf(full ? "[perf] redrawfull_us=%lu\n" : legacy ? "[perf] redrawlegacy_us=%lu\n" : fast ? "[perf] redrawfast_us=%lu\n" : "[perf] redraw_us=%lu\n",
                      (unsigned long)elapsed);
      }
    } else {
      Serial.println("[diagnostic] unknown command");
    }
    s_lastDiagnosticMs = millis();
    return true;
  }
  return true;
}
#endif
