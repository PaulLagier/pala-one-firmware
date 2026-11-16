// Pixel regression for the real U8g2 decoder. Font changes reset transparency;
// cached and direct pages must still paint identical pixels, including overlaps.
#include <U8g2_for_Adafruit_GFX.h>
#include "opendyslexic_u8g2_fonts.h"
#include "src/ui/font_render.h"
#include "src/pure/paginator.h"
#include <cassert>
#include <vector>
#include <string>
#include <cstdio>
int main() {
  const uint8_t* faces[] = {
    u8g2_font_open_dys_r08_te, u8g2_font_open_dys_r10_te,
    u8g2_font_open_dys_r12_te, u8g2_font_open_dys_r14_te,
    u8g2_font_open_dys_b08_te, u8g2_font_open_dys_b10_te,
    u8g2_font_open_dys_b12_te, u8g2_font_open_dys_b14_te
  };
  std::string sample;
  for (int i=0; i<8; ++i) sample +=
    "Pala One reader test. café, Größe, mañana.\n"
    "The next page must preserve every line, word, accent and page boundary.\n"
    "https://example.invalid/abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789\n";
  int cases=0;
  for (const auto* face : faces) {
    Adafruit_GFX gfx; U8G2_FOR_ADAFRUIT_GFX font; font.begin(gfx);
    font.setFontMode(1); Font::useTransparentFace(font, face);
    font.setForegroundColor(1); font.setBackgroundColor(0);
    const int ascent=font.getFontAscent(), height=ascent-font.getFontDescent();
    LayoutMetrics metrics{ascent,font.getFontDescent(),height,238,114/height};
    auto measure=[&](const char* text) {
      Font::useTransparentFace(font,face); return font.getUTF8Width(text);
    };
    StringReadStream stream(sample.c_str()); int y=ascent;
    std::vector<std::string> direct,cached;
    const auto next=paginatePage(stream,0,metrics,measure,[&](const char* text,size_t n) {
      direct.emplace_back(text,n); Font::useTransparentFace(font,face);
      font.setCursor(6,y); font.print(text); y+=height;
    });
    uint8_t pixels[250*122]; memcpy(pixels,gfx.pixels,sizeof(pixels));
    memset(gfx.pixels,0,sizeof(gfx.pixels));
    const auto nextCached=paginatePage(stream,0,metrics,measure,[&](const char* text,size_t n) {
      cached.emplace_back(text,n);
    });
    font.setFontMode(1); y=ascent;
    for (const auto& line : cached) {
      Font::useTransparentFace(font,face); font.setCursor(6,y);
      font.print(line.c_str()); y+=height;
    }
    assert(next==nextCached && direct==cached);
    assert(memcmp(pixels,gfx.pixels,sizeof(pixels))==0); ++cases;
  }
  printf("Font decoder: %d direct/cache pixel cases passed\n",cases);
}
