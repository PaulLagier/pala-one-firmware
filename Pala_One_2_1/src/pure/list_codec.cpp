#include "list_codec.h"

#include <cstring>

void sanitizeListItemText(String& s) {
  s.replace("\r", "");
  s.replace("\n", " ");
  s.replace("\t", " ");
  while (s.indexOf("  ") != -1) s.replace("  ", " ");
  s.trim();
  if ((int)s.length() > MAX_LIST_TEXT) s = s.substring(0, MAX_LIST_TEXT);
}

int decodeList(const uint8_t* buf, size_t got, ListData& out) {
  out.count = 0;
  if (got < 1) return 0;

  uint8_t count = buf[0];
  if (count > MAX_LIST_ITEMS) count = MAX_LIST_ITEMS;
  size_t pos = 1;

  ListItemData scratch[MAX_LIST_ITEMS];
  for (uint8_t i = 0; i < count; i++) {
    if (pos + 1u + MAX_LIST_TEXT + 1u > got) break;
    scratch[i].done = buf[pos++];
    std::memcpy(scratch[i].text, &buf[pos], MAX_LIST_TEXT + 1);
    scratch[i].text[MAX_LIST_TEXT] = '\0';
    pos += (MAX_LIST_TEXT + 1);

    String t(scratch[i].text);
    sanitizeListItemText(t);
    if (t.length() == 0) continue;

    std::strncpy(out.items[out.count].text, t.c_str(), MAX_LIST_TEXT);
    out.items[out.count].text[MAX_LIST_TEXT] = '\0';
    out.items[out.count].done = scratch[i].done ? 1 : 0;
    out.count++;
  }
  return out.count;
}

size_t encodeList(const ListData& data, uint8_t* outBuf) {
  std::memset(outBuf, 0, LIST_ENCODED_MAX_SIZE);
  outBuf[0] = (uint8_t)data.count;
  size_t pos = 1;
  for (int i = 0; i < data.count && i < MAX_LIST_ITEMS; i++) {
    outBuf[pos++] = data.items[i].done ? 1 : 0;
    std::memset(&outBuf[pos], 0, MAX_LIST_TEXT + 1);
    std::strncpy(reinterpret_cast<char*>(&outBuf[pos]), data.items[i].text, MAX_LIST_TEXT);
    pos += (MAX_LIST_TEXT + 1);
  }
  return pos;
}

bool listHasVisible(const ListData& data) {
  for (int i = 0; i < data.count; i++) {
    if (data.items[i].text[0]) return true;
  }
  return false;
}
