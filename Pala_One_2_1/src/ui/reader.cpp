#include "src/ui/bookmark_chrome.h"
#include "src/ui/pala_chrome.h"
#include "src/ui/font_render.h"
#include "src/ui/reader.h"
#include "src/ui/reader_index_feedback.h"
#include "src/hal/packed_file_stream.h"
#include "src/hal/battery.h"
#include "src/ui/screen_settings.h"

#include "src/hal/display.h"
#include "src/hal/cpu_burst.h"
#include "src/pure/hashing.h"
#include "src/storage/book_metadata.h"
#include "src/storage/library.h" // g_library — openBookByIndex reads it
#include "src/storage/page_cache.h"
#include "src/storage/preferences_store.h"
#include "src/storage/statistics.h" // Statistics::onReaderPageTurn

#include "src/ui/font.h"                   // layoutForCache for cache stamping
#include "src/ui/screens/library_screen.h" // navigateToLibraryRoot — fallback on error
#include "src/ui/bookmark_settings.h"
#include "src/ui/statusbar.h"              // Statusbar::mode for the per-mode statusbar render
#include "src/ui/text.h"
#include "src/ui/toast.h"   // Toast::isActive / Toast::draw
#include "src/ui/widgets.h" // drawCenter (error fallback)

// The currently open book and where the user is looking. Produced by
// `openBookByIndex()` below, fully torn down by `resetBookView()` on leaving
// the reader.
BookView g_bookview;

// One-shot flag: force the next renderCurrentPage() to do a full e-ink refresh
// regardless of pageTurnsSinceFull. Set by forceNextRenderFull() (called from
// the unlock path to clear screensaver ghosting).
static bool s_forceNextFull = false;

void forceNextRenderFull() { s_forceNextFull = true; }

// Deferred-repagination flag. Set by `markPagesDirtyForLayoutChange` when a
// mid-session layout change (currently only Statusbar mode cycling) has
// invalidated the in-memory page offset table. Consumed at the top of
// `prepareForRender` so the rebase happens once, on the next reader draw,
// rather than inside the setting cycle.
//
// Two reasons to defer rather than rebase inside `Statusbar::setMode`:
//   1. Batching — cycling Full -> Minimal -> Hidden marks dirty three times
//      but only needs one rebase, on the eventual redraw.
//   2. Decoupling — `setMode` stays a plain setting write with no reader /
//      page-table coupling at call time; the reader owns the table and
//      rebases on its own schedule.
// Rebuilding happens once after the menu closes, using buffered file reads.
static bool s_pagesLayoutDirty = false;
static bool s_prefetchPending = false;
static bool s_indexCancelled = false;
bool readerIndexWasCancelled() { return s_indexCancelled; }
static void cancelIndex() {
  s_indexCancelled = true;
  navigateToLibraryRoot();
}

void markPagesDirtyForLayoutChange() { s_pagesLayoutDirty = true; }

// Auto-save throttle bookkeeping. Private to this translation unit — only
// the save-progress functions and `resetSaveThrottle` touch it, and only
// the reader calls them on its hot path (preview never auto-saves).
// Hiding it here keeps the "this is the open book" public type from
// carrying reader-only baggage.
struct SaveThrottle {
  uint32_t lastSaveMs = 0;
  int lastSavedPage = -1;
};
static SaveThrottle s_save;

static void resetSaveThrottle() { s_save = SaveThrottle{}; }

// ============================================================================
//  OpenBook — file + path + key managed together. Strong invariant:
//  `isOpen()` ⇔ `path()` is non-empty. Public callers never observe
//  the file-closed-with-path-still-set intermediate state.
// ============================================================================
bool OpenBook::open(const String &path) {
  close();
  File f = FS.open(path, "r");
  if (!f || f.isDirectory()) {
    if (f)
      f.close();
    return false;
  }
  file_ = f;
  stream_.reset(new (std::nothrow) BookReadStream(file_));
  if (!stream_ || !stream_->valid()) {
    close();
    return false;
  }
  path_ = path;
  key_ = prefKeyForBook(path);
  return true;
}

void OpenBook::close() {
  clearPagePrefetch();
  stream_.reset();
  if (file_)
    file_.close();
  path_ = "";
  key_ = "";
}

void armResumeOnWake() {
  if (!g_bookview.book.isOpen())
    return;
  prefs.putString("wake_path", g_bookview.book.path());
}

static void clearResumeOnWake() {
  if (prefs.isKey("wake_path"))
    prefs.remove("wake_path");
}

// ============================================================================
//  Active-book page table — extending g_bookview.pages forward.
//
//  Lives here (not in text.cpp) because every helper mutates `g_bookview`.
//  Layered on `nextPageOffset` (a pure file/offset primitive in text.cpp).
//
//  Cursor clamping is intentionally NOT done here. The single canonical clamp
//  lives in `prepareForRender` — every render path flows through it, so any
//  out-of-range cursor that this code produces (e.g. EOF reached before the
//  requested page) is normalized before it's observed.
// ============================================================================

// Append a known next-page offset onto the table. Used by both
// `tryExtendPageTable` (which computes the offset via paginate) and
// `renderCurrentPage` (which already learned it as drawPageAt's return
// value — recording it avoids a redundant paginate on the next advance).
// Returns true on append, false if the offset doesn't extend the table
// (EOF or `next` not strictly after the last known offset).
static bool appendPageOffset(uint32_t next) {
  if (g_bookview.pages.count == MAX_PAGES &&
      next > g_bookview.pages.offsets[g_bookview.pages.count - 1] &&
      next < g_bookview.book.size()) {
    // Keep a previous window for fast backwards crossings. A failed cache
    // write is harmless: navigation can rebuild from the beginning.
    savePageOffsetCacheForBook(g_bookview.book.path(), g_bookview.book.size(),
                               Font::layoutForCache(), g_bookview.pages);
  }
  return g_bookview.pages.append(next, g_bookview.book.size());
}

// Extend `g_bookview.pages` by one entry. Returns true if a page was appended
// (or the empty seed entry was created), false if no further progress is
// possible (EOF reached).
static bool tryExtendPageTable(IReadStream &stream) {
  // Defensive seed — book just opened or anyone left the table empty.
  if (g_bookview.pages.count < 1) {
    g_bookview.pages.reset();
    return true;
  }
  if (g_bookview.pages.eofReached)
    return false;

  uint32_t start = g_bookview.pages.offsets[g_bookview.pages.count - 1];
  uint32_t next = nextPageOffset(stream, start);
  const bool appended = appendPageOffset(next);
  if (appended && (g_bookview.pages.endPage() & 31) == 0)
    delay(1);
  return appended;
}

// Extend the page table forward until `targetPage` is reachable, or no more
// progress is possible (EOF). A full window is cached before it rolls forward.
static bool ensureOffsetsUpTo(int targetPage) {
  s_indexCancelled = false;
  if (targetPage < 0)
    targetPage = 0;
  if (targetPage < g_bookview.pages.firstPage) {
    g_bookview.pages.reset();
    loadPageOffsetCacheForBook(g_bookview.book.path(), g_bookview.book.size(),
                               Font::layoutForCache(), g_bookview.pages);
    if (targetPage < g_bookview.pages.firstPage)
      g_bookview.pages.reset();
  }
  if (g_bookview.pages.endPage() <= targetPage) {
    // Long legacy-position and backwards-window scans are CPU work, just
    // like page rendering. Do not leave them at the idle frequency.
    CpuBurst cpu;
    IReadStream &stream = g_bookview.book.stream();
    ReaderIndexFeedback feedback;
    while (g_bookview.pages.endPage() <= targetPage) {
      if (!feedback.step(uint32_t(g_bookview.pages.endPage()), uint32_t(targetPage))) {
        cancelIndex();
        return false;
      }
      if (!tryExtendPageTable(stream))
        break;
    }
  }
  return true;
}

int findPageForOffset(uint32_t targetOffset, bool showProgress) {
  s_indexCancelled = false;
  if (g_bookview.pages.count && targetOffset < g_bookview.pages.offsets[0]) {
    g_bookview.pages.reset();
    loadPageOffsetCacheForBook(g_bookview.book.path(), g_bookview.book.size(),
                               Font::layoutForCache(), g_bookview.pages);
    if (targetOffset < g_bookview.pages.offsets[0])
      g_bookview.pages.reset();
  }
  // Extend forward until the last known page starts at or past the target,
  // or we can't extend further.
  if (g_bookview.pages.count == 0 ||
      g_bookview.pages.offsets[g_bookview.pages.count - 1] < targetOffset) {
    // Restore the caller's clock on every exit, including nested render
    // bursts; cached lookups do not need a frequency switch.
    CpuBurst cpu;
    IReadStream &stream = g_bookview.book.stream();
    ReaderIndexFeedback feedback(showProgress);
    while (g_bookview.pages.count == 0 ||
           g_bookview.pages.offsets[g_bookview.pages.count - 1] < targetOffset) {
      const uint32_t current =
          g_bookview.pages.count ? g_bookview.pages.offsets[g_bookview.pages.count - 1] : 0;
      if (!feedback.step(current, targetOffset)) {
        cancelIndex();
        return -1;
      }
      if (!tryExtendPageTable(stream))
        break;
    }
  }
  // The page containing `targetOffset` is the largest N with
  // offsets[N] <= targetOffset.
  for (int i = g_bookview.pages.count - 1; i >= 0; i--) {
    if (g_bookview.pages.offsets[i] <= targetOffset)
      return g_bookview.pages.firstPage + i;
  }
  return 0;
}

// ============================================================================
//  Reader operations
// ============================================================================
bool openBookByIndex(int idx) {
  s_indexCancelled = false;
  // Invariant: callers reset the view before calling here. `OpenBook::open`
  // is also atomic about the file-handle swap, so a stale handle would be
  // overwritten cleanly even if the invariant were violated.
  const char *p = bookPath(idx);
  if (!p)
    return false;

  String path(p);
  if (!g_bookview.book.open(path))
    return false;

  g_bookview.pages.reset();
  loadPageOffsetCacheForBook(path, g_bookview.book.size(), Font::layoutForCache(),
                             g_bookview.pages);

  // Resolve the reading position. The byte offset (`_off`) is canonical and
  // survives font changes; if it's set, find which page contains it under
  // the current layout. If absent (legacy data from older firmware), fall
  // back to the saved page number — it'll get rewritten as an offset on
  // the next save.
  // A cancelled offset lookup returns -1 and leaves saved progress untouched.
  // Successful lookup and the legacy page hint are nonnegative.
  // Any out-of-range value vs. the page table gets normalized in
  // prepareForRender on the first draw.
  PreferencesStore kv(prefs);
  uint32_t savedOffset = loadSavedOffset(kv, g_bookview.book.key());
  if (savedOffset != kOffsetUnset) {
    g_bookview.cursor.pageIndex = findPageForOffset(savedOffset, true);
    if (g_bookview.cursor.pageIndex < 0) {
      g_bookview.book.close();
      return false;
    }
  } else {
    g_bookview.cursor.pageIndex = loadSavedPage(kv, g_bookview.book.key());
  }
  g_bookview.cursor.pageTurnsSinceFull = 0;
  resetSaveThrottle();
  return true;
}

static void drawStatusBar(uint32_t startOffset) {
  if (Statusbar::mode() == Statusbar::Hidden)
    return;

  size_t total = g_bookview.book.size();
  if (total == 0)
    total = 1;

  if (Statusbar::mode() == Statusbar::Minimal) {
    int w = SCREEN_W - 2 * MARGIN_X;
    int filled = (int)((startOffset * (uint32_t)w) / (uint32_t)total);
    if (filled < 0)
      filled = 0;
    if (filled > w)
      filled = w;
    if (filled > 0)
      gfx.drawFastHLine(MARGIN_X, SCREEN_H - 1, filled, 1);
    return;
  }

  int pageTextW = 0;
  if (SHOW_PAGE_NUMBER) {
    Font::useUiTiny();
    char buf[20];
    snprintf(buf, sizeof(buf), "%d", g_bookview.cursor.pageIndex + 1);
    pageTextW = u8g2.getUTF8Width(buf);
    u8g2.setCursor(SCREEN_W - MARGIN_X - pageTextW, SCREEN_H - 1);
    u8g2.print(buf);
    Font::useBody();
  }

  if (SHOW_PROGRESS_BAR) {
    const int padR = SHOW_PAGE_NUMBER ? (pageTextW + 8) : 0;
    int x0 = MARGIN_X;
    bool showBatteryWarning = false;
#if HAS_BATTERY
    if (ScreenSettings::batteryIndicatorsEnabled()) {
      // Battery status usually isn't updated mid-session, so poll it here to see if we're low.
      // Entries are valid for BAT_CACHE_MS so this won't be too burdonsome.
      updateBatteryBackground();
      showBatteryWarning = batteryLow();
    }
#endif
    if (showBatteryWarning) {
      drawBatteryBottomLeft();
      x0 += 24;
    }
    int w = (SCREEN_W - MARGIN_X - x0) - padR;
    if (w < 40)
      w = 40;
    int yTop = SCREEN_H - 7;
    int barH = 4;
    int filled = (int)((startOffset * (uint32_t)w) / (uint32_t)total);
    if (filled < 0)
      filled = 0;
    if (filled > w)
      filled = w;

    gfx.drawRect(x0, yTop, w, barH, 1);
    if (filled > 0)
      gfx.fillRect(x0 + 1, yTop + 1, max(0, filled - 2), barH - 2, 1);
  }
}

// State normalization for rendering: lazily paginates up to the cursor,
// clamps the cursor to a valid range, and recovers from out-of-file
// offsets. Returns false if the book has nothing renderable (empty file
// or no pages) — caller should show an error and bail.
//
// All g_bookview mutations triggered by rendering are concentrated here;
// the actual draw in renderCurrentPage is read-only on view state
// (modulo pageTurnsSinceFull, which is render-side bookkeeping).
static bool prepareForRender() {
  if (g_bookview.book.size() == 0)
    return false;
  // Consume any deferred layout-change repagination *before* extending the
  // page table — otherwise `ensureOffsetsUpTo` would happily extend the
  // stale table with new-layout offsets glued onto old-layout offsets.
  if (s_pagesLayoutDirty) {
    s_pagesLayoutDirty = false;
    repaginateForLayoutChange();
    if (s_indexCancelled)
      return false;
  }
  if (!ensureOffsetsUpTo(g_bookview.cursor.pageIndex))
    return false;
  if (g_bookview.pages.count <= 0)
    return false;

  if (g_bookview.cursor.pageIndex < 0)
    g_bookview.cursor.pageIndex = 0;
  if (g_bookview.cursor.pageIndex >= g_bookview.pages.endPage())
    g_bookview.cursor.pageIndex = g_bookview.pages.endPage() - 1;

  if (g_bookview.pages.offsetAt(g_bookview.cursor.pageIndex) >= g_bookview.book.size()) {
    g_bookview.cursor.pageIndex = 0;
    g_bookview.pages.reset();
    g_bookview.pages.eofReached = false;
  }
  return true;
}

bool renderBookmarkPreview(bool readSelected) {
  CpuBurst cpu;
  if (!g_bookview.book.isOpen() || !prepareForRender() ||
      !g_bookview.pages.contains(g_bookview.cursor.pageIndex))
    return false;
  const uint32_t offset = g_bookview.pages.offsetAt(g_bookview.cursor.pageIndex);
  const uint32_t position = g_bookview.book.stream().position();
  prepareMenuFrame();
  drawSectionHeader("");
  BookmarkChrome::header(u8g2, D_BOOKMARKS_PREVIEW);
  PalaChrome::panel(gfx);
  Font::useToast();
  u8g2.setFontMode(1);
  // Preview uses its own compact layout. Never put these boundaries in the
  // book's page table or change typography, saved progress or reading counters.
  const LayoutMetrics metrics{14, -4, 18, 216, 2};
  {
    int y = 48;
    IReadStream &stream = g_bookview.book.stream();
    paginatePage(
        stream, offset, metrics,
        [](const char *text) { return Font::renderText(u8g2, text, strlen(text)); },
        [&](const char *text, size_t bytes) {
          const int width = Font::renderText(u8g2, text, bytes);
          Font::renderText(u8g2, text, bytes, 125 - width / 2, y, true);
          y += 18;
        });
  }
  g_bookview.book.stream().seek(position);
  const unsigned percent =
      g_bookview.book.size() ? unsigned(uint64_t(offset) * 100 / g_bookview.book.size()) : 0;
  char page[96];
  snprintf(page, sizeof(page), D_READER_MENU_PAGE_PCT_FMT, g_bookview.cursor.pageIndex + 1,
           percent);
  u8g2.setFont(pala_font_sans_r08);
  u8g2.setFontMode(1);
  BookmarkChrome::centered(u8g2, page, 80);
  BookmarkChrome::action(gfx, u8g2, 5, D_BOOKMARKS_READ, readSelected);
  BookmarkChrome::action(gfx, u8g2, 129, D_ABOUT_BACK, !readSelected);
  Font::useBody();
  cpu.restore();
  display.update();
  return true;
}

void renderCurrentPage(PrintState::State &state) {
  CpuBurst cpu;
#if PALA_DIAGNOSTICS
  const uint32_t renderStarted = micros();
#endif
  // Strong invariant: if we got here, a book is open. The reader screen
  // is never active otherwise.
  if (!prepareForRender()) {
    if (!s_indexCancelled)
      drawCenter(D_READER_BOOK_EMPTY, D_READER_BACK_LIBRARY);
    navigateToLibraryRoot();
    return;
  }

  uint32_t start = g_bookview.pages.offsetAt(g_bookview.cursor.pageIndex);

  bool doFull =
      s_forceNextFull || (g_bookview.cursor.pageTurnsSinceFull >= FULL_REFRESH_EVERY_N_PAGES);
  s_forceNextFull = false;
  if (doFull) {
    display.fastmodeOff();
    g_bookview.cursor.pageTurnsSinceFull = 0;
  } else {
    display.fastmodeOn();
  }

  beginPageCanvas();
  state.isBold ? Font::useBold() : Font::useBody();

  uint32_t nextOff = drawPageAt(g_bookview.book.stream(), start, state);
  // Record what render learned about the next page so a later advance
  // doesn't re-paginate this same page. Only meaningful when the cursor is
  // at the table's current last page; appendPageOffset is a no-op otherwise.
  if (g_bookview.cursor.pageIndex + 1 == g_bookview.pages.endPage()) {
    appendPageOffset(nextOff);
  }

  if (Toast::isActive())
    Toast::draw();
  else
    drawStatusBar(start);

  s_prefetchPending = true;
#if PALA_DIAGNOSTICS
  const uint32_t layoutDone = micros();
#endif
  cpu.restore(); // panel busy time does not benefit from a faster CPU
  display.update();
#if PALA_DIAGNOSTICS
  Serial.printf("[perf] reader layout_us=%lu display_us=%lu full=%d\n",
                (unsigned long)(layoutDone - renderStarted), (unsigned long)(micros() - layoutDone),
                doFull);
#endif
}

void prefetchNextReaderPage() {
  if (!s_prefetchPending || !g_bookview.book.isOpen() || s_pagesLayoutDirty)
    return;
  s_prefetchPending = false;
  int next = g_bookview.cursor.pageIndex + 1;
  if (g_bookview.pages.contains(next))
    prefetchPage(g_bookview.book.stream(), g_bookview.pages.offsetAt(next));
}

bool tryRestoreReadingSession() {
  // Single-shot: read wake intent, then clear it. From this point on
  // (during this runtime session) wake state is empty unless the reader
  // re-arms it on its way into deep sleep.
  String wp = prefs.isKey("wake_path") ? prefs.getString("wake_path", "") : String("");
  clearResumeOnWake();
  if (wp.length() == 0)
    return false;

  int idx = bookIndexForPath(wp);
  if (idx < 0)
    return false;
  if (!openBookByIndex(idx))
    return false;
  return true;
}

// ============================================================================
//  Cursor navigation — shared between ReaderScreen and BookmarkPreviewScreen.
//  Both move the cursor and bump `pageTurnsSinceFull` on a real change; the
//  caller-side post-move work (save progress, render) varies between the
//  screens and stays at the call site.
// ============================================================================
bool advancePage() {
  int targetPage = g_bookview.cursor.pageIndex + 1;
  if (!ensureOffsetsUpTo(targetPage))
    return false;
  // Couldn't extend to the requested page (EOF) — don't move.
  if (!g_bookview.pages.contains(targetPage))
    return false;
  g_bookview.cursor.pageIndex = targetPage;
  g_bookview.cursor.pageTurnsSinceFull++;
  Statistics::onReaderPageTurn();
  return true;
}

bool retreatPage() {
  if (g_bookview.cursor.pageIndex <= 0)
    return false;
  if (!ensureOffsetsUpTo(g_bookview.cursor.pageIndex - 1))
    return false;
  if (!g_bookview.pages.contains(g_bookview.cursor.pageIndex - 1))
    return false;
  g_bookview.cursor.pageIndex--;
  g_bookview.cursor.pageTurnsSinceFull++;
  Statistics::onReaderPageTurn();
  return true;
}

// ============================================================================
//  Layout-change response — keeps `g_bookview.pages` consistent with the
//  current Font::layoutForCache(). See header comment for the contract.
// ============================================================================
void repaginateForLayoutChange() {
  if (!g_bookview.book.isOpen())
    return;

  // Rebuild from the beginning: mixing layouts can skip text when going
  // backwards, and must never be persisted as a valid new-layout cache.
  uint32_t anchor = 0;
  int cur = g_bookview.cursor.pageIndex;
  if (g_bookview.pages.contains(cur))
    anchor = g_bookview.pages.offsetAt(cur);
  g_bookview.pages.reset();
  g_bookview.cursor.pageIndex = findPageForOffset(anchor, true);
  resetSaveThrottle();
}

// ============================================================================
//  Full view reset — used when leaving the reader entirely.
// ============================================================================
void resetBookView() {
  s_indexCancelled = false;
  clearPagePrefetch();
  s_prefetchPending = false;
  g_bookview.book.close();
  g_bookview.cursor = ReaderCursor{};
  g_bookview.pages = PageOffsetTable{};
  s_pagesLayoutDirty = false;
  resetSaveThrottle();
}

// ============================================================================
//  Progress + bookmark glue. Reads/writes g_bookview, delegates the actual
//  persistence to the pure storage API. Lives here (not in storage/) because
//  the throttle decision and the "is a book open" guard are view-state
//  concerns.
// ============================================================================
void saveProgress() {
  if (!g_bookview.book.isOpen() || s_indexCancelled ||
      !g_bookview.pages.contains(g_bookview.cursor.pageIndex))
    return;

  PreferencesStore kv(prefs);
  // Canonical: the byte offset where the user currently is. Invariant
  // under font/layout changes.
  saveSavedOffset(kv, g_bookview.book.key(),
                  g_bookview.pages.offsetAt(g_bookview.cursor.pageIndex));
  // Hint: the page number at the current layout. Used by the web file
  // list for display and as a legacy fallback for openBookByIndex when
  // no offset has been saved yet.
  saveSavedPage(kv, g_bookview.book.key(), g_bookview.cursor.pageIndex);
  s_save.lastSaveMs = millis();
  s_save.lastSavedPage = g_bookview.cursor.pageIndex;
}

void saveProgressThrottled() {
  if (!g_bookview.book.isOpen())
    return;
  if (g_bookview.cursor.pageIndex == s_save.lastSavedPage)
    return;
  uint32_t now = millis();
  if (s_save.lastSaveMs != 0 && (now - s_save.lastSaveMs) < SAVE_EVERY_MS)
    return;
  saveProgress();
}

const char *addBookmarkForCurrentBook() {
  if (!g_bookview.book.isOpen() || s_indexCancelled ||
      !g_bookview.pages.contains(g_bookview.cursor.pageIndex))
    return nullptr;

  PreferencesStore kv(prefs);
  const uint32_t start = g_bookview.pages.offsetAt(g_bookview.cursor.pageIndex);
  uint32_t end = start;
  const bool toggle = BookmarkSettings::toggleEnabled();
  if (toggle) {
    const int next = g_bookview.cursor.pageIndex + 1;
    if (g_bookview.pages.contains(next))
      end = g_bookview.pages.offsetAt(next);
    else if (g_bookview.pages.eofReached)
      end = uint32_t(g_bookview.book.size());
    else {
      if (s_pagesLayoutDirty)
        return D_WEB_ERR_WRITE_FAILED;
      // Usually render already recorded this boundary. Resolve only one page
      // when needed; never treat the rest of the book as the current page.
      auto &stream = g_bookview.book.stream();
      const uint32_t position = stream.position();
      end = nextPageOffset(stream, start);
      if (!stream.seek(position))
        return D_WEB_ERR_WRITE_FAILED;
    }
    if (end <= start || end > g_bookview.book.size())
      return D_WEB_ERR_WRITE_FAILED;
  }
  return applyReaderBookmarkAction(kv, g_bookview.book.key(), uint32_t(g_bookview.cursor.pageIndex),
                                   start, end, toggle);
}

// Commit everything the active reader has accumulated to durable storage.
// Used at every "leaving" moment (sleep, exit-to-home, preview commit) so a
// later boot or open finds both the latest cursor and a fresh page table.
// Mid-session uses `saveProgressThrottled` for cheaper, more frequent writes.
void persistReaderState() {
  if (!g_bookview.book.isOpen() || s_indexCancelled ||
      !g_bookview.pages.contains(g_bookview.cursor.pageIndex))
    return;
  saveProgress();
  // Font/status settings may have changed while the menu still holds the
  // old layout's table. Its canonical byte offset is valid; its page cache
  // must not be stamped as the new layout on close or sleep.
  if (s_pagesLayoutDirty)
    return;
  savePageOffsetCacheForBook(g_bookview.book.path(), g_bookview.book.size(), Font::layoutForCache(),
                             g_bookview.pages);
}
