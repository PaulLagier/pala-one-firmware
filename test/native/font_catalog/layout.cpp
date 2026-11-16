#include "test_framework.h"
#include "src/ui/font.h"
#include "src/ui/font_render.h"
#include "src/ui/body_page.h"
#include "src/state.h"
#include <string>
#include <vector>
// Number-only roles are not used by these body-font tests.
extern const uint8_t u8g2_font_5x8_tf[] = {0};
extern const uint8_t u8g2_font_6x10_tf[] = {0};
FontPreferences prefs;
U8G2_FOR_ADAFRUIT_GFX u8g2;
namespace Statusbar {
int reserveH() { return 8; }
} // namespace Statusbar
TEST_CASE("compact font bounds contain every ASCII glyph in both weights and all sizes") {
  Adafruit_GFX gfx;
  u8g2.begin(gfx);
  for (auto family : {Font::Family::Helvetica, Font::Family::OpenDyslexic}) {
    Font::setFamily(family);
    for (int size : {8, 10, 12, 14}) {
      Font::setBodySize(size);
      const auto metrics = Font::bodyLayout();
      for (bool bold : {false, true}) {
        if (bold)
          Font::useBold();
        else
          Font::useBody();
        int ascent = 0, descent = 0;
        for (unsigned cp = 32; cp < 127; ++cp)
          Font::glyphVerticalBounds(u8g2, cp, ascent, descent);
        CHECK(ascent <= metrics.ascent);
        CHECK(descent >= metrics.descent);
      }
      // Language coverage must not force maximum accent padding on ordinary text.
      int fullAscent = 0, fullDescent = 0;
      for (bool bold : {false, true}) {
        if (bold)
          Font::useBold();
        else
          Font::useBody();
        for (unsigned cp = 32; cp < 0x2100; ++cp)
          Font::glyphVerticalBounds(u8g2, cp, fullAscent, fullDescent);
      }
      CHECK(metrics.lineH < fullAscent - fullDescent + 1);
    }
  }
}
TEST_CASE("composed and decomposed text has identical ink and width across font switches") {
  Adafruit_GFX gfx;
  u8g2.begin(gfx);
  for (auto family : {Font::Family::Helvetica, Font::Family::OpenDyslexic}) {
    Font::setFamily(family);
    for (int size : {8, 10, 12, 14}) {
      Font::setBodySize(size);
      for (bool bionic : {false, true}) {
        Font::setBionic(bionic);
        const char *composed = "café Größe Йога";
        const char *decomposed = "cafe\xCC\x81 Gro\xCC\x88ße И\xCC\x86ога";
        CHECK_EQ(Font::measureBionicLine(composed), Font::measureBionicLine(decomposed));
        memset(gfx.pixels, 0, sizeof(gfx.pixels));
        Font::drawBionicLine(6, 45, composed);
        std::vector<uint8_t> pixels(gfx.pixels, gfx.pixels + sizeof(gfx.pixels));
        memset(gfx.pixels, 0, sizeof(gfx.pixels));
        Font::drawBionicLine(6, 45, decomposed);
        CHECK(std::equal(pixels.begin(), pixels.end(), gfx.pixels));
      }
    }
  }
}
TEST_CASE("page metrics converge and rendering agrees with scanning across multilingual pages") {
  Adafruit_GFX gfx;
  u8g2.begin(gfx);
  std::string text;
  for (int i = 0; i < 12; ++i)
    text += "Café, Größe, Čeština, Українська. Йога и чтение.\n\n";
  for (auto family : {Font::Family::Helvetica, Font::Family::OpenDyslexic}) {
    Font::setFamily(family);
    for (int size : {8, 10, 12, 14})
      for (bool bionic : {false, true}) {
        Font::setBodySize(size);
        Font::setBionic(bionic);
        StringReadStream stream(text.c_str());
        uint32_t pos = 0;
        while (pos < text.size()) {
          LayoutMetrics metrics;
          const auto next = prepareBodyPage(stream, pos, metrics);
          REQUIRE(next > pos);
          CHECK(next <= text.size());
          int y = TOP_PAD + metrics.ascent;
          const auto drawn = paginatePage(stream, pos, metrics, Font::measureBionicLine,
                                          [&](const char *line, size_t len) {
                                            if (len) {
                                              CHECK(y - metrics.ascent >= TOP_PAD);
                                              CHECK(y - metrics.descent < SCREEN_H - 8);
                                              Font::drawBionicLine(MARGIN_X, y, line);
                                            }
                                            y += len ? metrics.lineH : metrics.paragraphGapH;
                                          });
          CHECK_EQ(next, drawn);
          pos = next;
        }
      }
  }
}

TEST_CASE("public half-paragraph preference takes precedence over earlier local key") {
  prefs.values.clear();
  prefs.values["cfg_half_gaps"] = 1;
  Font::loadSettings();
  CHECK(Font::halfParagraphGapsEnabled());
  prefs.values["cfg_hgap"] = 0;
  Font::loadSettings();
  CHECK(!Font::halfParagraphGapsEnabled());
  Font::setHalfParagraphGaps(true);
  CHECK_EQ(prefs.values["cfg_hgap"], 1);
  prefs.values["cfg_half_gaps"] = 0;
  Font::loadSettings();
  CHECK(Font::halfParagraphGapsEnabled());
}

TEST_CASE("prepared multilingual page paints the same pixels without a second layout pass") {
  Adafruit_GFX gfx;
  u8g2.begin(gfx);
  const char *sample = "Reading café, Größe, Українська.\n\nCompact accents and Bionic text. "
                       "Reading café, Größe, Українська.\n";
  for (auto family : {Font::Family::Helvetica, Font::Family::OpenDyslexic}) {
    Font::setFamily(family);
    for (int size : {8, 10, 12, 14})
      for (bool bionic : {false, true}) {
        Font::setBodySize(size);
        Font::setBionic(bionic);
        StringReadStream stream(sample);
        LayoutMetrics metrics;
        PageTextBuffer buffer;
        const auto next = prepareBodyPage(stream, 0, metrics, &buffer);
        REQUIRE(buffer.valid);
        int y = TOP_PAD + metrics.ascent;
        memset(gfx.pixels, 0, sizeof(gfx.pixels));
        const auto direct = paginatePage(stream, 0, metrics, Font::measureBionicLine,
                                         [&](const char *line, size_t len) {
                                           if (len)
                                             Font::drawBionicLine(MARGIN_X, y, line);
                                           y += len ? metrics.lineH : metrics.paragraphGapH;
                                         });
        std::vector<uint8_t> pixels(gfx.pixels, gfx.pixels + sizeof(gfx.pixels));
        memset(gfx.pixels, 0, sizeof(gfx.pixels));
        y = TOP_PAD + metrics.ascent;
        for (size_t i = 0; i < buffer.count; ++i) {
          const char *line = buffer.line(i);
          if (*line)
            Font::drawBionicLine(MARGIN_X, y, line);
          y += *line ? metrics.lineH : metrics.paragraphGapH;
        }
        CHECK_EQ(direct, next);
        CHECK(std::equal(pixels.begin(), pixels.end(), gfx.pixels));
      }
  }
}
