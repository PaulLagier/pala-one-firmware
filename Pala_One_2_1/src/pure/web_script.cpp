#include "src/pure/web_script.h"
#include <cstdio>

String webJsString(const char *text) {
  String out = "\"";
  if (text) {
    for (const unsigned char *p = reinterpret_cast<const unsigned char *>(text); *p; ++p) {
      const unsigned char c = *p;
      if (c == '\\' || c == '"') {
        out += '\\';
        out += char(c);
      } else if (c < 0x20 || c == '<' || c == '>' || c == '&') {
        char escaped[7];
        snprintf(escaped, sizeof(escaped), "\\u%04x", unsigned(c));
        out += escaped;
      } else if (c == 0xe2 && p[1] == 0x80 && (p[2] == 0xa8 || p[2] == 0xa9)) {
        out += p[2] == 0xa8 ? "\\u2028" : "\\u2029";
        p += 2;
      } else
        out += char(c);
    }
  }
  out += '"';
  return out;
}
