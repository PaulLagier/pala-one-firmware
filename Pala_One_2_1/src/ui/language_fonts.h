#pragma once
#include <stdint.h>
// Pala Sans and Pala Dys: Latin/Cyrillic builds. See font_licenses/.
// Internal family IDs remain compatible with existing saved settings.
extern const uint8_t pala_font_sans_r08[];
extern const uint8_t pala_font_sans_b08[];
extern const uint8_t pala_font_sans_r10[];
extern const uint8_t pala_font_sans_b10[];
extern const uint8_t pala_font_sans_r12[];
extern const uint8_t pala_font_sans_b12[];
extern const uint8_t pala_font_sans_r14[];
extern const uint8_t pala_font_sans_b14[];
extern const uint8_t pala_font_dys_r08[];
extern const uint8_t pala_font_dys_b08[];
extern const uint8_t pala_font_dys_r10[];
extern const uint8_t pala_font_dys_b10[];
extern const uint8_t pala_font_dys_r12[];
extern const uint8_t pala_font_dys_b12[];
extern const uint8_t pala_font_dys_r14[];
extern const uint8_t pala_font_dys_b14[];
namespace PalaFonts {
struct Bounds {
  int8_t ascent;
  int8_t descent;
};
Bounds bounds(bool dys, int size);
} // namespace PalaFonts
extern const uint8_t pala_font_ui_tiny08[];
extern const uint8_t pala_font_ui_small10[];

extern const uint8_t pala_font_ui_digits29[];
