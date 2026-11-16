#include "test_framework.h"
#include "src/pure/companion_queue.h"
#include "src/pure/ipc_stack_policy.h"

TEST_CASE("Companion queue accepts only authenticated input from its current owner") {
  Companion::Queue queue;
  uint8_t byte = 42;
  CHECK(!queue.connect(Companion::NoOwner, 1));
  CHECK(!queue.connect(1, 0));
  REQUIRE(queue.connect(1, 55));
  const auto owner = queue.session();
  CHECK(!queue.connect(2, 99));
  CHECK(!queue.push(owner, false, &byte, 1, false));
  auto stranger = owner;
  stranger.owner = 2;
  CHECK(!queue.push(stranger, true, &byte, 1, false));
  CHECK(!queue.push(owner, true, nullptr, 1, false));
  CHECK(!queue.push(owner, true, &byte, 0, false));
  CHECK(!queue.push(owner, true, &byte, 513, false));
  REQUIRE(queue.push(owner, true, &byte, 1, false));
  auto work = queue.take();
  CHECK(work.reset);
  REQUIRE(work.hasPacket);
  CHECK_EQ(work.packet.bytes[0], 42);
  CHECK_EQ(work.packet.length, 1);
  CHECK(Companion::sameSession(work.packet.session, owner));
  CHECK(!queue.take().hasPacket);
}
TEST_CASE("Companion queue disconnect invalidates stale frames even with reused handles") {
  Companion::Queue queue;
  uint8_t byte = 7;
  REQUIRE(queue.connect(7, 22));
  const auto old = queue.session();
  REQUIRE(queue.push(old, true, &byte, 1, true));
  CHECK(!queue.disconnect(8));
  REQUIRE(queue.disconnect(7));
  auto work = queue.take();
  CHECK(work.reset);
  CHECK(!work.hasPacket);
  REQUIRE(queue.connect(7, 22)); // Same controller handle and random-token collision.
  const auto fresh = queue.session();
  CHECK(!Companion::sameSession(old, fresh));
  CHECK(!queue.push(old, true, &byte, 1, true));
  REQUIRE(queue.push(fresh, true, &byte, 1, true));
  work = queue.take();
  REQUIRE(work.hasPacket);
  CHECK(work.packet.data);
  CHECK(Companion::sameSession(work.packet.session, fresh));
  queue.end();
  CHECK(!queue.session().connected());
  CHECK(queue.take().reset);
  CHECK(!queue.push(fresh, true, &byte, 1, true));
}
TEST_CASE("Companion queue has bounded FIFO storage and explicit overflow recovery") {
  Companion::Queue queue;
  REQUIRE(queue.connect(3, 77));
  auto owner = queue.session();
  uint8_t bytes[512];
  for (unsigned i = 0; i < Companion::QueueCapacity; ++i) {
    std::memset(bytes, int(i), sizeof(bytes));
    REQUIRE(queue.push(owner, true, bytes, sizeof(bytes), true));
  }
  for (unsigned i = 0; i < Companion::QueueCapacity; ++i) {
    auto work = queue.take();
    REQUIRE(work.hasPacket);
    CHECK(!work.overflow);
    CHECK_EQ(work.packet.length, 512);
    for (auto byte : work.packet.bytes)
      CHECK_EQ(byte, i);
  }
  for (unsigned i = 0; i < Companion::QueueCapacity; ++i)
    REQUIRE(queue.push(owner, true, bytes, 1, true));
  CHECK(!queue.push(owner, true, bytes, 1, true));
  CHECK(!queue.push(owner, true, bytes, 1, false));
  auto work = queue.take();
  CHECK(work.overflow);
  CHECK(work.reset);
  CHECK(!work.hasPacket);
  REQUIRE(queue.push(owner, true, bytes, 1, false));
  work = queue.take();
  CHECK(!work.overflow);
  REQUIRE(work.hasPacket);
  REQUIRE(queue.push(owner, true, bytes, 1, true));
  queue.discardPending();
  CHECK(!queue.take().hasPacket);
}
TEST_CASE("SDK IPC stack policy preserves all unrelated task creation arguments") {
  for (const char *name :
       {"", "i", "ip", "ipc", "ipc0x", "ipc1x", "ipc2", "main", "nimble_host", "ipc0", "ipc1"}) {
    for (int core = -1; core <= 2; ++core) {
      for (bool sdk : {false, true}) {
        const bool expected =
            sdk && core >= 0 && core < 2 && std::string(name) == "ipc" + std::to_string(core);
        CHECK_EQ(IpcStackPolicy::bytes(name, core, sdk, 1024, 2048), expected ? 2048u : 1024u);
        CHECK_EQ(IpcStackPolicy::bytes(name, core, sdk, 4096, 2048), 4096u);
        CHECK_EQ(IpcStackPolicy::bytes(name, core, sdk, 0, 2048), 0u);
      }
    }
  }
  CHECK(!IpcStackPolicy::namedTask(nullptr, 0));
}
