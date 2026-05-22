#include "src/ui/font.h"

#include <string.h>             // memcpy
#include <U8g2_for_Adafruit_GFX.h>

#include "src/config.h"
#include "src/hal/display.h"  // u8g2 instance
#include "src/state.h"        // prefs

// OpenDyslexic u8g2 font tables. Vendored alongside the sketch (see
// Pala_One_2_1/opendyslexic_u8g2_fonts.h). Only referenced from this file.
#include "opendyslexic_u8g2_fonts.h"

namespace Font {

// File-private font tables. The rest of the codebase only ever sees the
// role accessors (useBody, useBold, ...).
static const uint8_t* s_body    = u8g2_font_helvR08_te;
static const uint8_t* s_bold    = u8g2_font_helvB08_te;
// _tf = ASCII only. Translated strings must NOT use these — see font.h.
static const uint8_t* s_uiSmall = u8g2_font_6x10_tf;
static const uint8_t* s_uiTiny  = u8g2_font_5x8_tf;
// _te = Latin Extended. Used by toasts so translations with accents render.
static const uint8_t* s_toast   = u8g2_font_helvR08_te;

// Owned settings.
//   s_size    — body font size (8/10/12/14). Out-of-set values fall back to 10.
//   s_lineGap — extra px between body lines, [0, 4].
//   s_family  — body face family. Drives which table set applyBodySize picks.
//   s_bionic  — render body text in "bionic reading" mode (bold prefix of
//               each word). Hooked into text.cpp's draw path; the paginator
//               picks it up via measureBionicWord.
static int    s_size    = 10;
static int    s_lineGap = 0;
static Family s_family  = Family::Helvetica;
static bool   s_bionic  = false;

// Layout-metrics cache. Invalid after the mutators below; recomputed on the
// next bodyLayout() call.
static LayoutMetrics s_layout;
static bool          s_layoutValid = false;

// NVS keys — file-private; nothing outside font.cpp should reference them.
static constexpr const char* kKeyBodySize   = "cfg_font";
static constexpr const char* kKeyLineGap    = "cfg_lgap";
static constexpr const char* kKeyFamily     = "cfg_font_fam";
static constexpr const char* kKeyBionic     = "cfg_bionic";

// Pick the (regular, bold) pair for a given size + family. Centralized so
// applyBodySize and applyFamily share one switch. Returns true if `sz` was
// in the supported set; out params are always set (10pt fallback otherwise).
static bool pickFaces(int sz, Family fam,
                      const uint8_t*& outBody, const uint8_t*& outBold) {
  if (fam == Family::OpenDyslexic) {
    switch (sz) {
      case 8:  outBody = u8g2_font_open_dys_r08_te; outBold = u8g2_font_open_dys_b08_te; return true;
      case 10: outBody = u8g2_font_open_dys_r10_te; outBold = u8g2_font_open_dys_b10_te; return true;
      case 12: outBody = u8g2_font_open_dys_r12_te; outBold = u8g2_font_open_dys_b12_te; return true;
      case 14: outBody = u8g2_font_open_dys_r14_te; outBold = u8g2_font_open_dys_b14_te; return true;
      default: outBody = u8g2_font_open_dys_r10_te; outBold = u8g2_font_open_dys_b10_te; return false;
    }
  }
  switch (sz) {
    case 8:  outBody = u8g2_font_helvR08_te; outBold = u8g2_font_helvB08_te; return true;
    case 10: outBody = u8g2_font_helvR10_te; outBold = u8g2_font_helvB10_te; return true;
    case 12: outBody = u8g2_font_helvR12_te; outBold = u8g2_font_helvB12_te; return true;
    case 14: outBody = u8g2_font_helvR14_te; outBold = u8g2_font_helvB14_te; return true;
    default: outBody = u8g2_font_helvR10_te; outBold = u8g2_font_helvB10_te; return false;
  }
}

// In-memory body-size apply: select font tables + invalidate the layout
// cache. Any out-of-set size falls back to 10. Does NOT persist — public
// callers go through setBodySize() / loadSettings().
static void applyBodySize(int sz) {
  if (!pickFaces(sz, s_family, s_body, s_bold)) sz = 10;
  s_size = sz;
  s_layoutValid = false;
}

// In-memory family apply: refresh the (body, bold) pair for the current
// size under the new family. Does NOT persist.
static void applyFamily(Family fam) {
  s_family = fam;
  // pickFaces always writes the out params; current s_size determines the
  // size half of the (size, family) lookup.
  (void)pickFaces(s_size, s_family, s_body, s_bold);
  s_layoutValid = false;
}

// In-memory line-gap apply: clamp + invalidate the layout cache. Does NOT
// persist — public callers go through setLineGap() / loadSettings().
static void applyLineGap(int gap) {
  if (gap < 0) gap = 0;
  if (gap > 4) gap = 4;
  s_lineGap = gap;
  s_layoutValid = false;
}

void useBody()     { u8g2.setFont(s_body); }
void useBold()     { u8g2.setFont(s_bold); }
void useToast()    { u8g2.setFont(s_toast); }
void useUiSmall()  { u8g2.setFont(s_uiSmall); }
void useUiTiny()   { u8g2.setFont(s_uiTiny); }
void useAppLarge() { u8g2.setFont(u8g2_font_helvB14_te); }

const LayoutMetrics& bodyLayout() {
  if (!s_layoutValid) {
    useBody();
    s_layout.ascent  = u8g2.getFontAscent();
    s_layout.descent = u8g2.getFontDescent();
    s_layout.lineH   = (s_layout.ascent - s_layout.descent) + s_lineGap;

    int w = SCREEN_W - (MARGIN_X * 2);
    if (w < 50) w = 50;
    s_layout.maxWidth = w;

    int maxHeight = SCREEN_H - TOP_PAD - BOT_PAD;
    if (SHOW_PROGRESS_BAR || SHOW_PAGE_NUMBER) maxHeight -= STATUS_H;

    s_layout.maxLines = maxHeight / s_layout.lineH;
    if (s_layout.maxLines < 1) s_layout.maxLines = 1;
    s_layoutValid = true;
  }
  return s_layout;
}

void loadSettings() {
  int famVal = prefs.getInt(kKeyFamily, 0);
  applyFamily(famVal == 1 ? Family::OpenDyslexic : Family::Helvetica);
  applyBodySize(prefs.getInt(kKeyBodySize, 10));
  applyLineGap(prefs.getInt(kKeyLineGap, 0));
  s_bionic = (prefs.getInt(kKeyBionic, 0) != 0);
}

void setBodySize(int sz) {
  applyBodySize(sz);
  prefs.putInt(kKeyBodySize, s_size);  // s_size reflects validation fallback
}

void setLineGap(int gap) {
  applyLineGap(gap);
  prefs.putInt(kKeyLineGap, s_lineGap);  // s_lineGap reflects clamp
}

void setFamily(Family fam) {
  applyFamily(fam);
  prefs.putInt(kKeyFamily, fam == Family::OpenDyslexic ? 1 : 0);
}

void setBionic(bool on) {
  if (s_bionic == on) return;
  s_bionic = on;
  // Layout cache is unchanged (per-line height/width-budget don't move), but
  // the on-disk page cache is stamped with the bionic flag so it'll reject
  // any stale entries on the next load — no explicit invalidation needed.
  prefs.putInt(kKeyBionic, on ? 1 : 0);
}

int    currentBodySize() { return s_size; }
int    currentLineGap()  { return s_lineGap; }
Family currentFamily()   { return s_family; }
bool   bionicEnabled()   { return s_bionic; }

PageCacheLayout layoutForCache() {
  return PageCacheLayout{
    /*bodySize=*/s_size,
    /*lineGap =*/s_lineGap,
    /*family  =*/(uint8_t)s_family,
    /*bionic  =*/(uint8_t)(s_bionic ? 1 : 0),
  };
}

// ----------------------------------------------------------------------------
//  Bionic helpers — UTF-8-aware. Same prefix policy as Kevin's fork:
//    chars ≤ 3 → 1 strong char; otherwise chars/2 clamped to [1, 4].
//  Punctuation-only words (no ASCII alnum / non-ASCII lead byte) get 0 so
//  we don't bold things like "—" or "...".
// ----------------------------------------------------------------------------

static bool isBionicLeadByte(uint8_t b) {
  return (b >= '0' && b <= '9') ||
         (b >= 'A' && b <= 'Z') ||
         (b >= 'a' && b <= 'z') ||
         (b >= 128);   // any UTF-8 multibyte lead byte
}

// UTF-8 char length at byte `i` in `s` (assumes well-formed; degenerate
// 0xF8+ falls through to 1). Local copy so this file doesn't pull in
// text_util.h — the algorithm is trivial.
static int utf8Len(uint8_t b) {
  if (b < 0x80) return 1;
  if ((b & 0xE0) == 0xC0) return 2;
  if ((b & 0xF0) == 0xE0) return 3;
  if ((b & 0xF8) == 0xF0) return 4;
  return 1;
}

static int bionicStrongCharCount(int charCount) {
  if (charCount <= 0) return 0;
  if (charCount <= 3) return 1;
  int n = charCount / 2;
  if (n < 1) n = 1;
  if (n > 4) n = 4;
  return n;
}

int bionicPrefixBytes(const char* word, int byteLen) {
  if (!word || byteLen <= 0) return 0;
  if (!isBionicLeadByte((uint8_t)word[0])) return 0;

  // Count UTF-8 chars, then convert "strong char count" back to a byte
  // offset by walking again. Two passes keep the logic obvious; words are
  // short so cost is negligible.
  int chars = 0;
  for (int i = 0; i < byteLen; ) {
    int clen = utf8Len((uint8_t)word[i]);
    if (i + clen > byteLen) break;
    i += clen;
    chars++;
  }

  int strong = bionicStrongCharCount(chars);
  if (strong <= 0) return 0;

  int byteOff = 0;
  int seen = 0;
  while (byteOff < byteLen && seen < strong) {
    int clen = utf8Len((uint8_t)word[byteOff]);
    if (byteOff + clen > byteLen) break;
    byteOff += clen;
    seen++;
  }
  // Reject if the split is degenerate: the whole word would be bolded
  // (no tail) or nothing would be (no head). Both look worse than
  // unstyled.
  if (byteOff <= 0 || byteOff >= byteLen) return 0;
  return byteOff;
}

// Largest line the paginator emits in one shot — see kLineMax in
// paginator.cpp. Word-level slicing fits in the same envelope.
static constexpr int kBionicScratchMax = 320;

// Walk a body-text line and call `onPart` for each maximal stretch of
// either non-space bytes (one word) or space bytes (one space run). For
// non-space stretches, `isWord` is true. Used by both the measure and the
// draw entry points so the segmentation logic only lives here.
template <typename Fn>
static void forEachLineSegment(const char* line, Fn onPart) {
  if (!line) return;
  int i = 0;
  while (line[i]) {
    int start = i;
    bool space = (line[i] == ' ');
    while (line[i] && ((line[i] == ' ') == space)) i++;
    int len = i - start;
    onPart(line + start, len, !space);
  }
}

// Measure helper — copies bytes [src, src+len) into a NUL-terminated stack
// buffer and returns u8g2.getUTF8Width on it. Caller has already set the
// active font.
static int measureSlice(const char* src, int len) {
  if (len <= 0) return 0;
  if (len >= kBionicScratchMax) len = kBionicScratchMax - 1;
  char scratch[kBionicScratchMax];
  memcpy(scratch, src, (size_t)len);
  scratch[len] = 0;
  return u8g2.getUTF8Width(scratch);
}

// Print helper — same idea, but prints at (x, y).
static void printSlice(int x, int y, const char* src, int len) {
  if (len <= 0) return;
  if (len >= kBionicScratchMax) len = kBionicScratchMax - 1;
  char scratch[kBionicScratchMax];
  memcpy(scratch, src, (size_t)len);
  scratch[len] = 0;
  u8g2.setCursor(x, y);
  u8g2.print(scratch);
}

// Width of one rendered word — measures the bold prefix in the Bold face
// and the tail in the Body face, plus kBionicRestGapPx between them.
// Falls back to a single Body measurement if the word doesn't qualify.
static int measureBionicWord(const char* word, int len) {
  if (len <= 0) return 0;
  int strong = bionicPrefixBytes(word, len);
  if (strong <= 0) {
    useBody();
    return measureSlice(word, len);
  }
  useBold();
  int wStrong = measureSlice(word, strong);
  useBody();
  int wTail = measureSlice(word + strong, len - strong);
  return wStrong + kBionicRestGapPx + wTail;
}

int measureBionicLine(const char* line) {
  if (!line) return 0;
  if (!s_bionic) {
    useBody();
    return u8g2.getUTF8Width(line);
  }
  int total = 0;
  forEachLineSegment(line, [&](const char* seg, int segLen, bool isWord) {
    if (!isWord) {
      useBody();
      total += measureSlice(seg, segLen);
      return;
    }
    total += measureBionicWord(seg, segLen);
  });
  useBody();
  return total;
}

void drawBionicLine(int x, int y, const char* line) {
  if (!line) return;
  if (!s_bionic) {
    useBody();
    u8g2.setCursor(x, y);
    u8g2.print(line);
    return;
  }
  int cursorX = x;
  forEachLineSegment(line, [&](const char* seg, int segLen, bool isWord) {
    if (segLen <= 0) return;
    if (!isWord) {
      useBody();
      printSlice(cursorX, y, seg, segLen);
      cursorX += measureSlice(seg, segLen);
      return;
    }
    int strong = bionicPrefixBytes(seg, segLen);
    if (strong <= 0) {
      useBody();
      printSlice(cursorX, y, seg, segLen);
      cursorX += measureSlice(seg, segLen);
      return;
    }
    useBold();
    printSlice(cursorX, y, seg, strong);
    cursorX += measureSlice(seg, strong);
    cursorX += kBionicRestGapPx;
    useBody();
    printSlice(cursorX, y, seg + strong, segLen - strong);
    cursorX += measureSlice(seg + strong, segLen - strong);
  });
  useBody();
}

}  // namespace Font
