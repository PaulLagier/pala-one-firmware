#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <algorithm>
class Print {
public:
  virtual ~Print() = default;
  virtual size_t write(uint8_t) = 0;
  size_t print(const char *s) {
    size_t n = 0;
    while (*s)
      n += write((uint8_t)*s++);
    return n;
  }
};
class Adafruit_GFX {
public:
  uint8_t pixels[250 * 122] = {};
  void drawPixel(int x, int y, uint16_t c) {
    if (x >= 0 && x < 250 && y >= 0 && y < 122)
      pixels[y * 250 + x] = c ? 1 : 0;
  }
  void drawFastHLine(int x, int y, int n, uint16_t c) {
    while (n-- > 0)
      drawPixel(x++, y, c);
  }
  void drawFastVLine(int x, int y, int n, uint16_t c) {
    while (n-- > 0)
      drawPixel(x, y++, c);
  }
  void fillRect(int x, int y, int w, int h, uint16_t c) {
    for (int j = 0; j < h; ++j)
      drawFastHLine(x, y + j, w, c);
  }
  void fillRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
    for (int j = 0; j < h; ++j)
      for (int i = 0; i < w; ++i) {
        int dx = i < r ? r - i - 1 : i >= w - r ? i - (w - r) : 0;
        int dy = j < r ? r - j - 1 : j >= h - r ? j - (h - r) : 0;
        if (dx * dx + dy * dy <= r * r)
          drawPixel(x + i, y + j, c);
      }
  }
  void drawRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
    fillRoundRect(x, y, w, h, r, c);
    fillRoundRect(x + 1, y + 1, w - 2, h - 2, r - 1, !c);
  }
  void drawCircle(int x0, int y0, int r, uint16_t c) {
    int f = 1 - r, dx = 1, dy = -2 * r, x = 0, y = r;
    drawPixel(x0, y0 + r, c);
    drawPixel(x0, y0 - r, c);
    drawPixel(x0 + r, y0, c);
    drawPixel(x0 - r, y0, c);
    while (x < y) {
      if (f >= 0) {
        --y;
        dy += 2;
        f += dy;
      }
      ++x;
      dx += 2;
      f += dx;
      drawPixel(x0 + x, y0 + y, c);
      drawPixel(x0 - x, y0 + y, c);
      drawPixel(x0 + x, y0 - y, c);
      drawPixel(x0 - x, y0 - y, c);
      drawPixel(x0 + y, y0 + x, c);
      drawPixel(x0 - y, y0 + x, c);
      drawPixel(x0 + y, y0 - x, c);
      drawPixel(x0 - y, y0 - x, c);
    }
  }
  void fillCircle(int x0, int y0, int r, uint16_t c) {
    drawFastVLine(x0, y0 - r, 2 * r + 1, c);
    int f = 1 - r, dx = 1, dy = -2 * r, x = 0, y = r;
    while (x < y) {
      if (f >= 0) {
        --y;
        dy += 2;
        f += dy;
      }
      ++x;
      dx += 2;
      f += dx;
      drawFastVLine(x0 + x, y0 - y, 2 * y + 1, c);
      drawFastVLine(x0 - x, y0 - y, 2 * y + 1, c);
      drawFastVLine(x0 + y, y0 - x, 2 * x + 1, c);
      drawFastVLine(x0 - y, y0 - x, 2 * x + 1, c);
    }
  }
  void save(const char *path) {
    FILE *f = fopen(path, "wb");
    fprintf(f, "P4\n250 122\n");
    for (int y = 0; y < 122; ++y)
      for (int b = 0; b < 32; ++b) {
        uint8_t v = 0;
        for (int k = 0; k < 8; ++k) {
          int x = b * 8 + k;
          if (x < 250 && pixels[y * 250 + x])
            v |= 0x80 >> k;
        }
        fputc(v, f);
      }
    fclose(f);
  }
};
