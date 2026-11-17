#include "src/ui/font.h"
#include "src/ui/font_render.h"

#include <string.h> // memcpy
#include <U8g2_for_Adafruit_GFX.h>

#include "src/config.h"
#include "src/hal/display.h"  // u8g2 instance
#include "src/state.h"        // prefs
#include "src/ui/statusbar.h" // Statusbar::reserveH

// Extended Latin/Cyrillic faces preserve the saved family IDs.
// Source attribution and licenses live in ui/font_licenses/.
#include "src/ui/language_fonts.h"

namespace Font {

// File-private font tables. The rest of the codebase only ever sees the
// role accessors (useBody, useBold, ...).
static const uint8_t *s_body = pala_font_sans_r08;
static const uint8_t *s_bold = pala_font_sans_b08;
// _tf = ASCII only. Translated strings must NOT use these — see font.h.
static const uint8_t *s_uiSmall = u8g2_font_6x10_tf;
// Page/status counters use the existing compact numeric face.
static const uint8_t *s_uiTiny = pala_font_ui_tiny08;
// Extended Latin/Cyrillic toast face, independent of the reading family.
static const uint8_t *s_toast = pala_font_sans_r08;

// Owned settings.
//   s_size     — body font size (8/10/12/14). Out-of-set values fall back to 10.
//   s_lineGap  — extra px between body lines, [0, 4].
//   s_family   — body face family. Drives which table set applyBodySize picks.
//   s_bionic   — render body text in "bionic reading" mode (bold prefix of
//                each word). Hooked into text.cpp's draw path; the paginator
//                picks it up via measureBionicWord.
//   s_markdown — render markdown bold formatting.
static int s_size = 10;
static int s_lineGap = 0;
static Family s_family = Family::Helvetica;
static bool s_bionic = false;
static bool s_halfGaps = false;
static bool s_markdown = false;

// Layout-metrics cache. Invalid after the mutators below; recomputed on the
// next bodyLayout() call.
static LayoutMetrics s_layout;
static bool s_layoutValid = false;

// NVS keys — file-private; nothing outside font.cpp should reference them.
static constexpr const char *kKeyBodySize = "cfg_font";
static constexpr const char *kKeyLineGap = "cfg_lgap";
static constexpr const char *kKeyFamily = "cfg_font_fam";
static constexpr const char *kKeyBionic = "cfg_bionic";
static constexpr const char *kKeyHalfGaps = "cfg_hgap";
static constexpr const char* kKeyMarkdown = "cfg_markdown";

// Pick the (regular, bold) pair for a given size + family. Centralized so
// applyBodySize and applyFamily share one switch. Returns true if `sz` was
// in the supported set; out params are always set (10pt fallback otherwise).
static bool pickFaces(int sz, Family fam, const uint8_t *&outBody, const uint8_t *&outBold) {
  if (fam == Family::OpenDyslexic) {
    switch (sz) {
    case 8:
      outBody = pala_font_dys_r08;
      outBold = pala_font_dys_b08;
      return true;
    case 10:
      outBody = pala_font_dys_r10;
      outBold = pala_font_dys_b10;
      return true;
    case 12:
      outBody = pala_font_dys_r12;
      outBold = pala_font_dys_b12;
      return true;
    case 14:
      outBody = pala_font_dys_r14;
      outBold = pala_font_dys_b14;
      return true;
    default:
      outBody = pala_font_dys_r10;
      outBold = pala_font_dys_b10;
      return false;
    }
  }
  switch (sz) {
  case 8:
    outBody = pala_font_sans_r08;
    outBold = pala_font_sans_b08;
    return true;
  case 10:
    outBody = pala_font_sans_r10;
    outBold = pala_font_sans_b10;
    return true;
  case 12:
    outBody = pala_font_sans_r12;
    outBold = pala_font_sans_b12;
    return true;
  case 14:
    outBody = pala_font_sans_r14;
    outBold = pala_font_sans_b14;
    return true;
  default:
    outBody = pala_font_sans_r10;
    outBold = pala_font_sans_b10;
    return false;
  }
}

// In-memory body-size apply: select font tables + invalidate the layout
// cache. Any out-of-set size falls back to 10. Does NOT persist — public
// callers go through setBodySize() / loadSettings().
static void applyBodySize(int sz) {
  if (!pickFaces(sz, s_family, s_body, s_bold))
    sz = 10;
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
  if (gap < 0)
    gap = 0;
  if (gap > 4)
    gap = 4;
  s_lineGap = gap;
  s_layoutValid = false;
}

void useMenu() { u8g2.setFont(pala_font_sans_r10); }
void useMenuBold() { u8g2.setFont(pala_font_sans_b10); }
void useBody() { useTransparentFace(u8g2, s_body); }
void useBold() { useTransparentFace(u8g2, s_bold); }
void useToast() { u8g2.setFont(s_toast); }
void useUiSmall() { u8g2.setFont(s_uiSmall); }
void useUiTiny() { u8g2.setFont(s_uiTiny); }
void useAppLarge() { u8g2.setFont(pala_font_sans_b14); }

const LayoutMetrics &bodyLayout() {
  if (!s_layoutValid) {
    useBody();
    // Tight ASCII bounds of both weights. Accents/extended glyphs enlarge
    // individual pages below; unrelated rare glyphs no longer pad every line.
    static constexpr PalaFonts::Bounds compact[2][4] = {{{9, -2}, {12, -3}, {13, -4}, {14, -4}},
                                                        {{10, -3}, {12, -4}, {14, -5}, {17, -5}}};
    const int index = s_size == 8 ? 0 : s_size == 12 ? 2 : s_size == 14 ? 3 : 1;
    const auto bounds = compact[s_family == Family::OpenDyslexic ? 1 : 0][index];
    s_layout.ascent = bounds.ascent;
    s_layout.descent = bounds.descent;
    completePageLayout(s_layout);
    s_layoutValid = true;
  }
  return s_layout;
}

void completePageLayout(LayoutMetrics &m) {
  // One clear pixel even between the deepest and highest neighboring glyphs.
  m.lineH = m.ascent - m.descent + 1 + s_lineGap;
  m.paragraphGapH = s_halfGaps ? max(1, m.lineH / 2) : m.lineH;
  m.maxWidth = SCREEN_W - MARGIN_X * 2;
  const int height = SCREEN_H - TOP_PAD - BOT_PAD - Statusbar::reserveH();
  m.maxLines = max(1, height / m.lineH);
}
void includeTextBounds(LayoutMetrics &m, const char *text, size_t length) {
  for (size_t index = 0; index < length;) {
    uint32_t cp;
    index += PalaUtf8::decode(text, length, index, cp);
    if (!PalaUtf8::combining(cp))
      while (index < length) {
        uint32_t mark;
        const auto bytes = PalaUtf8::decode(text, length, index, mark);
        const auto composed = PalaUtf8::compose(cp, mark);
        if (!composed)
          break;
        cp = composed;
        index += bytes;
      }
    if (cp >= 32 && cp <= 126)
      continue; // covered by compact bounds; no glyph lookup
    useBody();
    glyphVerticalBounds(u8g2, cp, m.ascent, m.descent);
    if (s_bionic) {
      useBold();
      glyphVerticalBounds(u8g2, cp, m.ascent, m.descent);
    }
  }
  useBody();
}

void loadSettings() {
  int famVal = prefs.getInt(kKeyFamily, 0);
  applyFamily(famVal == 1 ? Family::OpenDyslexic : Family::Helvetica);
  applyBodySize(prefs.getInt(kKeyBodySize, 10));
  applyLineGap(prefs.getInt(kKeyLineGap, 0));
  s_bionic = (prefs.getInt(kKeyBionic, 0) != 0);
  // Public key wins; retain the earlier local boolean preference on upgrade.
  // TODO: Don't think this `getBool` does anything. Under the hood it is calling
  // `getInt` on the same key, so just redundant.
  s_halfGaps = (prefs.getInt(kKeyHalfGaps, prefs.getBool("cfg_half_gaps", false) ? 1 : 0) != 0);
  s_markdown = (prefs.getInt(kKeyMarkdown, prefs.getBool("cfg_markdown", false) ? 1 : 0) != 0);
}

void setBodySize(int sz) {
  applyBodySize(sz);
  prefs.putInt(kKeyBodySize, s_size); // s_size reflects validation fallback
}

void setLineGap(int gap) {
  applyLineGap(gap);
  prefs.putInt(kKeyLineGap, s_lineGap); // s_lineGap reflects clamp
}

void setFamily(Family fam) {
  applyFamily(fam);
  prefs.putInt(kKeyFamily, fam == Family::OpenDyslexic ? 1 : 0);
}

void setBionic(bool on) {
  if (s_bionic == on)
    return;
  s_bionic = on;
  // Layout cache is unchanged (per-line height/width-budget don't move), but
  // the on-disk page cache is stamped with the bionic flag so it'll reject
  // any stale entries on the next load — no explicit invalidation needed.
  prefs.putInt(kKeyBionic, on ? 1 : 0);
}

void setHalfParagraphGaps(bool on) {
  if (s_halfGaps == on)
    return;
  s_halfGaps = on;
  // Changing paragraph height changes page layout so drop the
  // cached metrics and invalidate caches.
  s_layoutValid = false;
  prefs.putInt(kKeyHalfGaps, on ? 1 : 0);
}

void setMarkdown(bool on) {
  if (s_markdown == on) return;
  s_markdown = on;
  // Layout cache is unchanged (per-line height/width-budget don't move), but
  // the on-disk page cache is stamped with the markdown flag so it'll reject
  // any stale entries on the next load — no explicit invalidation needed.
  prefs.putInt(kKeyMarkdown, on ? 1 : 0);
}

int currentBodySize() { return s_size; }
int currentLineGap() { return s_lineGap; }
Family currentFamily() { return s_family; }
bool bionicEnabled() { return s_bionic; }
bool halfParagraphGapsEnabled() { return s_halfGaps; }
bool markdownEnabled()          { return s_markdown; }

PageCacheLayout layoutForCache() {
  // Statusbar reserve goes into the cache stamp too — toggling between
  // Full / Minimal / Hidden modes changes the page's text area (and
  // therefore `maxLines`), which shifts every page boundary in the cached
  // offset table. See Statusbar::setMode and page_cache.cpp's magic history.
  return PageCacheLayout{
      /*bodySize        =*/s_size,
      /*lineGap         =*/s_lineGap,
      /*family          =*/(uint8_t)s_family,
      /*bionic          =*/(uint8_t)(s_bionic ? 1 : 0),
      /*halfGaps        =*/(uint8_t)(s_halfGaps ? 1 : 0),
      /*markdown        =*/(uint8_t)(s_markdown ? 1 : 0),
      /*statusbarReserve=*/(uint8_t)Statusbar::reserveH(),
  };
}

// ----------------------------------------------------------------------------
//  Bionic helpers — UTF-8-aware. Same prefix policy as Kevin's fork:
//    chars ≤ 3 → 1 strong char; otherwise chars/2 clamped to [1, 4].
//  Punctuation-only words (no ASCII alnum / non-ASCII lead byte) get 0 so
//  we don't bold things like "—" or "...".
// ----------------------------------------------------------------------------

int bionicPrefixBytes(const char *word, int byteLen) {
  return PalaUtf8::bionicPrefixBytes(word, byteLen);
}

// Walk a body-text line and call `onPart` for each maximal stretch of
// either non-space bytes (one word) or space bytes (one space run). For
// non-space stretches, `isWord` is true. Used by both the measure and the
// draw entry points so the segmentation logic only lives here.
template <typename Fn> static void forEachLineSegment(const char *line, Fn onPart) {
  if (!line)
    return;
  int i = 0;
  while (line[i]) {
    int start = i;
    bool space = (line[i] == ' ');
    while (line[i] && ((line[i] == ' ') == space))
      i++;
    int len = i - start;
    onPart(line + start, len, !space);
  }
}

// Shared glyph traversal keeps width and ink identical, including canonical
// accents, intermediate overhangs and uncomposed Cyrillic stress marks.
static int measureSlice(const char *src, int len) {
  return len > 0 ? renderText(u8g2, src, size_t(len)) : 0;
}
static void printSlice(int x, int y, const char *src, int len) {
  if (len > 0)
    renderText(u8g2, src, size_t(len), x, y, true);
}

// Width of one rendered word — measures the bold prefix in the Bold face
// and the tail in the Body face, plus kBionicRestGapPx between them.
// Falls back to a single Body measurement if the word doesn't qualify.
static int measureBionicWord(const char *word, int len) {
  if (len <= 0)
    return 0;
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

int measureBionicLine(const char *line) {
  if (!line)
    return 0;
  if (!s_bionic) {
    useBody();
    return renderText(u8g2, line, strlen(line));
  }
  int total = 0;
  forEachLineSegment(line, [&](const char *seg, int segLen, bool isWord) {
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

void drawBionicLine(int x, int y, const char *line) {
  if (!line)
    return;
  if (!s_bionic) {
    useBody();
    renderText(u8g2, line, strlen(line), x, y, true);
    return;
  }
  int cursorX = x;
  forEachLineSegment(line, [&](const char *seg, int segLen, bool isWord) {
    if (segLen <= 0)
      return;
    if (!isWord) {
      useBody(); // isBold ? useBold() : useBody();
      printSlice(cursorX, y, seg, segLen);
      cursorX += measureSlice(seg, segLen);
      // set isBold to new value, ensuring it exists for next line to be printed
      // end while
      return;
    }
    int strong = bionicPrefixBytes(seg, segLen);
    if (strong <= 0) {
      // find next delineator char in segment
      // while delineator char exists
      // print current segment at current weight
      useBody(); // idBold ? useBold() : useBody();
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

void invalidateLayoutCache() { s_layoutValid = false; }

} // namespace Font
