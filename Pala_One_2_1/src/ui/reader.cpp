#include "src/ui/reader.h"

#include "src/hal/display.h"
#include "src/pure/hashing.h"
#include "src/storage/book_metadata.h"
#include "src/storage/library.h"   // g_library — openBookByIndex reads it
#include "src/storage/page_cache.h"
#include "src/storage/preferences_store.h"

#include "src/ui/font.h"                    // currentBodySize/currentLineGap for cache stamping
#include "src/ui/screens/library_screen.h"  // navigateToLibraryRoot — fallback on error
#include "src/ui/text.h"
#include "src/ui/toast.h"                   // Toast::isActive / Toast::draw
#include "src/ui/widgets.h"                 // drawCenter (error fallback)

// The currently open book and where the user is looking. Produced by
// `openBookByIndex()` below, fully torn down by `resetBookView()` on leaving
// the reader.
BookView g_bookview;

// Auto-save throttle bookkeeping. Private to this translation unit — only
// the save-progress functions and `resetSaveThrottle` touch it, and only
// the reader calls them on its hot path (preview never auto-saves).
// Hiding it here keeps the "this is the open book" public type from
// carrying reader-only baggage.
struct SaveThrottle {
  uint32_t lastSaveMs    = 0;
  int      lastSavedPage = -1;
};
static SaveThrottle s_save;

static void resetSaveThrottle() {
  s_save = SaveThrottle{};
}

// ============================================================================
//  OpenBook — file + path + key managed together. Strong invariant:
//  `isOpen()` ⇔ `path()` is non-empty. Public callers never observe
//  the file-closed-with-path-still-set intermediate state.
// ============================================================================
bool OpenBook::open(const String& path) {
  close();
  File f = FS.open(path, "r");
  if (!f || f.isDirectory()) {
    if (f) f.close();
    return false;
  }
  file_ = f;
  path_ = path;
  key_  = prefKeyForBook(path);
  return true;
}

void OpenBook::close() {
  if (file_) file_.close();
  path_ = "";
  key_  = "";
}

void armResumeOnWake() {
  if (!g_bookview.book.isOpen()) return;
  prefs.putString("wake_path", g_bookview.book.path());
}

static void clearResumeOnWake() {
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
// (EOF, MAX_PAGES, or `next` not strictly after the last known offset).
static bool appendPageOffset(uint32_t next) {
  if (g_bookview.pages.count < 1) return false;
  if (g_bookview.pages.eofReached) return false;
  if (g_bookview.pages.count >= MAX_PAGES) return false;

  uint32_t start = g_bookview.pages.offsets[g_bookview.pages.count - 1];
  if (next <= start) {
    // Paginator stuck (zero-byte page) — treat as EOF.
    g_bookview.pages.eofReached = true;
    return false;
  }

  g_bookview.pages.offsets[g_bookview.pages.count] = next;
  g_bookview.pages.count++;

  // At or past file end — no more pages to add. Use file size (not stream
  // `available()`) because the paginator's internal seeks make `available()`
  // unreliable.
  if (next >= (uint32_t)g_bookview.book.size()) {
    g_bookview.pages.eofReached = true;
  }
  return true;
}

// Extend `g_bookview.pages` by one entry. Returns true if a page was appended
// (or the empty seed entry was created), false if no further progress is
// possible (EOF reached or MAX_PAGES hit).
static bool tryExtendPageTable() {
  // Defensive seed — book just opened or anyone left the table empty.
  if (g_bookview.pages.count < 1) {
    g_bookview.pages.count = 1;
    g_bookview.pages.offsets[0] = 0;
    return true;
  }
  if (g_bookview.pages.eofReached) return false;
  if (g_bookview.pages.count >= MAX_PAGES) return false;

  uint32_t start = g_bookview.pages.offsets[g_bookview.pages.count - 1];
  uint32_t next = nextPageOffset(g_bookview.book.file(), start);
  return appendPageOffset(next);
}

// Extend the page table forward until `targetPage` is reachable, or no more
// progress is possible (EOF / MAX_PAGES). Pure extension — no persistence
// side effects (the explicit save points handle that — see persistReaderState).
static void ensureOffsetsUpTo(int targetPage) {
  while (g_bookview.pages.count <= targetPage) {
    if (!tryExtendPageTable()) break;
  }
}

int findPageForOffset(uint32_t targetOffset) {
  // Extend forward until the last known page starts at or past the target,
  // or we can't extend further.
  while (g_bookview.pages.count == 0
      || g_bookview.pages.offsets[g_bookview.pages.count - 1] < targetOffset) {
    if (!tryExtendPageTable()) break;
  }
  // The page containing `targetOffset` is the largest N with
  // offsets[N] <= targetOffset.
  for (int i = g_bookview.pages.count - 1; i >= 0; i--) {
    if (g_bookview.pages.offsets[i] <= targetOffset) return i;
  }
  return 0;
}

// ============================================================================
//  Reader operations
// ============================================================================
bool openBookByIndex(int idx) {
  // Invariant: callers reset the view before calling here. `OpenBook::open`
  // is also atomic about the file-handle swap, so a stale handle would be
  // overwritten cleanly even if the invariant were violated.
  const char* p = bookPath(idx);
  if (!p) return false;

  String path(p);
  if (!g_bookview.book.open(path)) return false;

  g_bookview.pages.count = 1;
  g_bookview.pages.offsets[0] = 0;
  g_bookview.pages.eofReached = false;
  loadPageOffsetCacheForBook(path, g_bookview.book.size(),
                             Font::layoutForCache(),
                             g_bookview.pages);

  // Resolve the reading position. The byte offset (`_off`) is canonical and
  // survives font changes; if it's set, find which page contains it under
  // the current layout. If absent (legacy data from older firmware), fall
  // back to the saved page number — it'll get rewritten as an offset on
  // the next save.
  // Both branches below produce a >= 0 page index (findPageForOffset's
  // floor is 0; loadSavedPage clamps internally), so no extra guard needed.
  // Any out-of-range value vs. the page table gets normalized in
  // prepareForRender on the first draw.
  PreferencesStore kv(prefs);
  uint32_t savedOffset = loadSavedOffset(kv, g_bookview.book.key());
  if (savedOffset != kOffsetUnset) {
    g_bookview.cursor.pageIndex = findPageForOffset(savedOffset);
  } else {
    g_bookview.cursor.pageIndex = loadSavedPage(kv, g_bookview.book.key());
  }
  g_bookview.cursor.pageTurnsSinceFull = 0;
  resetSaveThrottle();
  return true;
}

static void drawStatusBar(uint32_t startOffset) {
  size_t total = g_bookview.book.size();
  if (total == 0) total = 1;

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
    int w = (SCREEN_W - 2 * MARGIN_X) - padR;
    if (w < 40) w = 40;

    int x0 = MARGIN_X;
    int yTop = SCREEN_H - 7;
    int barH = 4;
    int filled = (int)((startOffset * (uint32_t)w) / (uint32_t)total);
    if (filled < 0) filled = 0;
    if (filled > w) filled = w;

    gfx.drawRect(x0, yTop, w, barH, 1);
    if (filled > 0) gfx.fillRect(x0 + 1, yTop + 1, max(0, filled - 2), barH - 2, 1);
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
  if (g_bookview.book.size() == 0) return false;
  ensureOffsetsUpTo(g_bookview.cursor.pageIndex);
  if (g_bookview.pages.count <= 0) return false;

  if (g_bookview.cursor.pageIndex < 0) g_bookview.cursor.pageIndex = 0;
  if (g_bookview.cursor.pageIndex >= g_bookview.pages.count)
    g_bookview.cursor.pageIndex = g_bookview.pages.count - 1;

  if (g_bookview.pages.offsets[g_bookview.cursor.pageIndex] >= g_bookview.book.size()) {
    g_bookview.cursor.pageIndex = 0;
    g_bookview.pages.count = 1;
    g_bookview.pages.offsets[0] = 0;
    g_bookview.pages.eofReached = false;
  }
  return true;
}

void renderCurrentPage() {
  // Strong invariant: if we got here, a book is open. The reader screen
  // is never active otherwise.
  if (!prepareForRender()) {
    drawCenter(D_READER_BOOK_EMPTY, D_READER_BACK_LIBRARY);
    navigateToLibraryRoot();
    return;
  }

  uint32_t start = g_bookview.pages.offsets[g_bookview.cursor.pageIndex];

  bool doFull = (g_bookview.cursor.pageTurnsSinceFull >= FULL_REFRESH_EVERY_N_PAGES);
  if (doFull) {
    display.fastmodeOff();
    g_bookview.cursor.pageTurnsSinceFull = 0;
  } else {
    display.fastmodeOn();
  }

  beginPageCanvas();
  Font::useBody();

  uint32_t nextOff = drawPageAt(g_bookview.book.file(), start);
  // Record what render learned about the next page so a later advance
  // doesn't re-paginate this same page. Only meaningful when the cursor is
  // at the table's current last page; appendPageOffset is a no-op otherwise.
  if (g_bookview.cursor.pageIndex + 1 == g_bookview.pages.count) {
    appendPageOffset(nextOff);
  }

  if (Toast::isActive()) Toast::draw();
  else                   drawStatusBar(start);

  display.update();
}

bool tryRestoreReadingSession() {
  // Single-shot: read wake intent, then clear it. From this point on
  // (during this runtime session) wake state is empty unless the reader
  // re-arms it on its way into deep sleep.
  String wp = prefs.getString("wake_path", "");
  clearResumeOnWake();
  if (wp.length() == 0) return false;

  int idx = bookIndexForPath(wp);
  if (idx < 0) return false;
  if (!openBookByIndex(idx)) return false;
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
  ensureOffsetsUpTo(targetPage);
  // Couldn't extend to the requested page (EOF / MAX_PAGES) — don't move.
  if (targetPage >= g_bookview.pages.count) return false;
  g_bookview.cursor.pageIndex = targetPage;
  g_bookview.cursor.pageTurnsSinceFull++;
  return true;
}

bool retreatPage() {
  if (g_bookview.cursor.pageIndex <= 0) return false;
  g_bookview.cursor.pageIndex--;
  g_bookview.cursor.pageTurnsSinceFull++;
  return true;
}

// ============================================================================
//  Full view reset — used when leaving the reader entirely.
// ============================================================================
void resetBookView() {
  g_bookview.book.close();
  g_bookview.cursor = ReaderCursor{};
  g_bookview.pages  = PageOffsetTable{};
  resetSaveThrottle();
}

// ============================================================================
//  Progress + bookmark glue. Reads/writes g_bookview, delegates the actual
//  persistence to the pure storage API. Lives here (not in storage/) because
//  the throttle decision and the "is a book open" guard are view-state
//  concerns.
// ============================================================================
void saveProgress() {
  if (!g_bookview.book.isOpen()) return;

  PreferencesStore kv(prefs);
  // Canonical: the byte offset where the user currently is. Invariant
  // under font/layout changes.
  saveSavedOffset(kv, g_bookview.book.key(),
                  g_bookview.pages.offsets[g_bookview.cursor.pageIndex]);
  // Hint: the page number at the current layout. Used by the web file
  // list for display and as a legacy fallback for openBookByIndex when
  // no offset has been saved yet.
  saveSavedPage(kv, g_bookview.book.key(), g_bookview.cursor.pageIndex);
  s_save.lastSaveMs    = millis();
  s_save.lastSavedPage = g_bookview.cursor.pageIndex;
}

void saveProgressThrottled() {
  if (!g_bookview.book.isOpen()) return;
  if (g_bookview.cursor.pageIndex == s_save.lastSavedPage) return;
  uint32_t now = millis();
  if (s_save.lastSaveMs != 0 && (now - s_save.lastSaveMs) < SAVE_EVERY_MS) return;
  saveProgress();
}

const char* addBookmarkForCurrentBook() {
  if (!g_bookview.book.isOpen()) return nullptr;

  PreferencesStore kv(prefs);
  Bookmarks bm = loadBookmarks(kv, g_bookview.book.key());

  uint32_t pageOff = g_bookview.pages.offsets[g_bookview.cursor.pageIndex];
  BookmarkAddResult r = addBookmark(bm, (uint16_t)g_bookview.cursor.pageIndex, pageOff);
  if (r.added) saveBookmarks(kv, g_bookview.book.key(), bm);
  // Page table is NOT saved here — every realistic flow that adds a bookmark
  // is followed by a sleep or explicit exit, both of which call
  // persistReaderState(). The bookmark's offset is byte-exact and survives
  // a re-paginate on next open if the disk cache is stale.
  return r.message;
}

// Commit everything the active reader has accumulated to durable storage.
// Used at every "leaving" moment (sleep, exit-to-home, preview commit) so a
// later boot or open finds both the latest cursor and a fresh page table.
// Mid-session uses `saveProgressThrottled` for cheaper, more frequent writes.
void persistReaderState() {
  if (!g_bookview.book.isOpen()) return;
  saveProgress();
  savePageOffsetCacheForBook(g_bookview.book.path(), g_bookview.book.size(),
                             Font::layoutForCache(),
                             g_bookview.pages);
}
