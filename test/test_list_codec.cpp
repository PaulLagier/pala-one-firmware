#include <cstring>

#include "test_framework.h"
#include "pure/list_codec.h"

TEST_CASE("sanitizeListItemText collapses whitespace") {
  String s = "  hello\tworld\n";
  sanitizeListItemText(s);
  CHECK_EQ(s, String("hello world"));
}

TEST_CASE("sanitizeListItemText truncates to MAX_LIST_TEXT") {
  String s;
  for (int i = 0; i < 200; i++) s += 'x';
  sanitizeListItemText(s);
  CHECK_EQ(s.length(), (unsigned)MAX_LIST_TEXT);
}

TEST_CASE("list codec round-trips items and done flags") {
  ListData data;
  data.count = 2;
  std::strcpy(data.items[0].text, "milk");
  data.items[0].done = 0;
  std::strcpy(data.items[1].text, "eggs");
  data.items[1].done = 1;

  uint8_t buf[LIST_ENCODED_MAX_SIZE];
  size_t n = encodeList(data, buf);
  CHECK(n > 0u);

  ListData back;
  CHECK_EQ(decodeList(buf, n, back), 2);
  CHECK_EQ(String(back.items[0].text), String("milk"));
  CHECK_EQ(back.items[0].done, 0);
  CHECK_EQ(String(back.items[1].text), String("eggs"));
  CHECK_EQ(back.items[1].done, 1);
}

TEST_CASE("list codec drops empty items on decode") {
  ListData data;
  data.count = 3;
  std::strcpy(data.items[0].text, "real");
  data.items[1].text[0] = '\0';  // empty
  std::strcpy(data.items[2].text, "also real");

  uint8_t buf[LIST_ENCODED_MAX_SIZE];
  size_t n = encodeList(data, buf);

  ListData back;
  CHECK_EQ(decodeList(buf, n, back), 2);
  CHECK_EQ(String(back.items[0].text), String("real"));
  CHECK_EQ(String(back.items[1].text), String("also real"));
}

TEST_CASE("list codec handles empty input") {
  ListData data;
  CHECK_EQ(decodeList(nullptr, 0, data), 0);
}

TEST_CASE("listHasVisible reflects whether any item has text") {
  ListData data;
  CHECK(!listHasVisible(data));
  data.count = 1;
  CHECK(!listHasVisible(data));  // empty text
  std::strcpy(data.items[0].text, "x");
  CHECK(listHasVisible(data));
}
