#pragma once
#include <U8g2_for_Adafruit_GFX.h>
#include "src/pure/utf8_clusters.h"
#include "src/pure/reader_composition.h"

namespace Font {
// The upstream decoder resets transparency whenever the face changes.
inline void useTransparentFace(U8G2_FOR_ADAFRUIT_GFX &renderer, const uint8_t *face) {
  renderer.setFont(face);
  renderer.setFontMode(1);
}
struct GlyphMetrics {
  int advance;
  int left;
  int width;
};
inline GlyphMetrics glyphMetrics(U8G2_FOR_ADAFRUIT_GFX &renderer, uint32_t cp) {
  if (cp > 0xFFFF)
    return {0, 0, 0};
  renderer.u8g2.font_decode.glyph_width = 0;
  renderer.u8g2.glyph_x_offset = 0;
  const int advance = u8g2_GetGlyphWidth(&renderer.u8g2, uint16_t(cp));
  return {advance, renderer.u8g2.glyph_x_offset, renderer.u8g2.font_decode.glyph_width};
}
// Read vertical BBX metadata without rasterizing or touching the display.
// GetGlyphWidth leaves the decoder immediately after x/y/advance. Rewind a
// copy to the y field; the pinned U8g2 decoder stores those bits LSB first.
inline void glyphVerticalBounds(U8G2_FOR_ADAFRUIT_GFX &renderer, uint32_t cp, int &ascent,
                                int &descent) {
  if (cp > 0xFFFF)
    return;
  renderer.u8g2.font_decode.glyph_height = 0;
  renderer.u8g2.font_decode.glyph_width = 0;
  u8g2_GetGlyphWidth(&renderer.u8g2, uint16_t(cp));
  const auto &decode = renderer.u8g2.font_decode;
  if (!decode.glyph_height || !decode.glyph_width)
    return;
  const auto &info = renderer.u8g2.font_info;
  if (!info.bits_per_char_y)
    return;
  const uint8_t *ptr = decode.decode_ptr;
  int bit = int(decode.decode_bit_pos) - info.bits_per_delta_x - info.bits_per_char_y;
  while (bit < 0) {
    --ptr;
    bit += 8;
  }
  unsigned value = 0;
  for (unsigned i = 0; i < info.bits_per_char_y; ++i) {
    value |= unsigned((*ptr >> bit) & 1) << i;
    if (++bit == 8) {
      ++ptr;
      bit = 0;
    }
  }
  const int y = int(value) - (1 << (info.bits_per_char_y - 1));
  if (int(decode.glyph_height) + y > ascent)
    ascent = int(decode.glyph_height) + y;
  if (y < descent)
    descent = y;
}

// One traversal defines both measurement and drawing. Include intermediate
// overhangs, not only the final glyph's ink. Compose the supported canonical
// Latin/Cyrillic pairs; remaining stress marks are centered on their base.
inline int renderText(U8G2_FOR_ADAFRUIT_GFX &renderer, const char *text, size_t length, int x = 0,
                      int y = 0, bool draw = false) {
  if (!text)
    return 0;
  int cursor = 0, right = 0, baseX = 0;
  GlyphMetrics base{0, 0, 0};
  bool haveBase = false;
  for (size_t index = 0; index < length;) {
    uint32_t cp;
    index += PalaUtf8::decode(text, length, index, cp);
    if (!PalaUtf8::combining(cp)) {
      while (index < length) {
        uint32_t mark;
        const auto bytes = PalaUtf8::decode(text, length, index, mark);
        const auto composed = PalaUtf8::compose(cp, mark);
        if (!composed)
          break;
        cp = composed;
        index += bytes;
      }
    }
    const auto glyph = glyphMetrics(renderer, cp);
    int origin = cursor;
    if (PalaUtf8::combining(cp) && haveBase)
      origin = baseX + base.left + (base.width - glyph.width) / 2 - glyph.left;
    else {
      baseX = cursor;
      base = glyph;
      haveBase = !PalaUtf8::combining(cp);
      cursor += glyph.advance;
    }
    const int end = origin + glyph.left + glyph.width;
    if (end > right)
      right = end;
    if (draw && cp <= 0xFFFF)
      renderer.drawGlyph(x + origin, y, uint16_t(cp));
  }
  const int width = cursor > right ? cursor : right;
  if (draw)
    renderer.setCursor(x + width, y);
  return width;
}
} // namespace Font
