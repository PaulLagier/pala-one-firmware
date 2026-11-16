#include "src/lang/join.h"
#include <string.h>
#include <algorithm>
#include <numeric>

namespace Language {
String join(std::initializer_list<const char *> parts) {
  const size_t bytes =
      std::accumulate(parts.begin(), parts.end(), size_t(0), [](size_t total, const char *part) {
        return total + (part ? strlen(part) : 0);
      });
  String out;
  out.reserve(bytes);
  std::for_each(parts.begin(), parts.end(), [&](const char *part) {
    if (part)
      out += part;
  });
  return out;
}
} // namespace Language
