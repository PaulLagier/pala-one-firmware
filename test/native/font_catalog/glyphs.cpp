#include <U8g2_for_Adafruit_GFX.h>
#include "src/ui/language_fonts.h"
#include <cstdio>
int main(int argc, char **argv) {
  const uint8_t *faces[] = {
      pala_font_sans_r08, pala_font_sans_b08, pala_font_sans_r10, pala_font_sans_b10,
      pala_font_sans_r12, pala_font_sans_b12, pala_font_sans_r14, pala_font_sans_b14,
      pala_font_dys_r08,  pala_font_dys_b08,  pala_font_dys_r10,  pala_font_dys_b10,
      pala_font_dys_r12,  pala_font_dys_b12,  pala_font_dys_r14,  pala_font_dys_b14};
  Adafruit_GFX gfx;
  U8G2_FOR_ADAFRUIT_GFX renderer;
  renderer.begin(gfx);
  unsigned cp;
  unsigned missing = 0, total = 0;
  while (scanf("%u", &cp) == 1) {
    for (unsigned i = 0; i < 16; ++i) {
      renderer.setFont(faces[i]);
      int w = u8g2_GetGlyphWidth(&renderer.u8g2, uint16_t(cp));
      ++total;
      if (w <= 0) {
        printf("missing font=%u U+%04X\n", i, cp);
        ++missing;
      }
    }
  }
  fprintf(stderr, "glyph checks=%u missing=%u\n", total, missing);
  return missing ? 1 : 0;
}
