#include <cstring>

#include "test_framework.h"
#include "map_kv_store.h"
#include "storage/list_items.h"

TEST_CASE("loadList returns empty when key is absent") {
  MapKvStore kv;
  ListData data = loadList(kv);
  CHECK_EQ(data.count, 0);
}

TEST_CASE("saveList + loadList round-trips items and done flags") {
  MapKvStore kv;
  ListData data;
  data.count = 3;
  std::strcpy(data.items[0].text, "alpha");
  data.items[0].done = 1;
  std::strcpy(data.items[1].text, "beta");
  data.items[1].done = 0;
  std::strcpy(data.items[2].text, "gamma");
  data.items[2].done = 1;

  saveList(kv, data);
  ListData back = loadList(kv);

  CHECK_EQ(back.count, 3);
  CHECK_EQ(String(back.items[0].text), String("alpha"));
  CHECK_EQ(back.items[0].done, 1);
  CHECK_EQ(String(back.items[2].text), String("gamma"));
  CHECK_EQ(back.items[2].done, 1);
}

TEST_CASE("loadList drops empty-text items from the stored blob") {
  MapKvStore kv;
  ListData data;
  data.count = 3;
  std::strcpy(data.items[0].text, "real");
  data.items[1].text[0] = '\0';
  std::strcpy(data.items[2].text, "also real");

  saveList(kv, data);
  ListData back = loadList(kv);

  CHECK_EQ(back.count, 2);
  CHECK_EQ(String(back.items[0].text), String("real"));
  CHECK_EQ(String(back.items[1].text), String("also real"));
}

TEST_CASE("saveList replaces prior content") {
  MapKvStore kv;
  ListData first;
  first.count = 2;
  std::strcpy(first.items[0].text, "x");
  std::strcpy(first.items[1].text, "y");
  saveList(kv, first);

  ListData second;
  second.count = 1;
  std::strcpy(second.items[0].text, "z");
  saveList(kv, second);

  ListData back = loadList(kv);
  CHECK_EQ(back.count, 1);
  CHECK_EQ(String(back.items[0].text), String("z"));
}

TEST_CASE("saveList skips the write when the stored bytes already match") {
  MapKvStore kv;
  ListData data;
  data.count = 2;
  std::strcpy(data.items[0].text, "alpha");
  data.items[0].done = 0;
  std::strcpy(data.items[1].text, "beta");
  data.items[1].done = 1;

  saveList(kv, data);
  size_t writesBefore = kv.putBytesCallCount();

  // Same content — must be a no-op (NVS flash-wear guard).
  saveList(kv, data);
  CHECK_EQ(kv.putBytesCallCount(), writesBefore);

  // Real change — must write again.
  data.items[0].done = 1;
  saveList(kv, data);
  CHECK(kv.putBytesCallCount() > writesBefore);
}
