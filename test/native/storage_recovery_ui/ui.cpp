#include "test_framework.h"
#include "runtime.h"
#include "src/ui/screens/storage_recovery_screen.h"

static unsigned retries = 0, initializations = 0;
static bool retryResult = false;
static unsigned retrySucceedsAt = 0;
static FsRecoveryResult initializationResult = FsRecoveryResult::Ready;
bool fsRecoveryRetry() { ++retries; return retryResult || (retrySucceedsAt && retries >= retrySucceedsAt); }
FsRecoveryResult fsRecoveryInitializeConfirmed() { ++initializations; return initializationResult; }

struct Fixture {
  StorageRecoveryScreen screen;
  Fixture() {
    retries = initializations = resets = updates = 0;
    retryResult = Lock::locked = false;
    retrySucceedsAt = delays = mockMillis = bootPolls = 0;
    bootEvents.clear(); bootEventIndex = 0;
    initializationResult = FsRecoveryResult::Ready;
    physicalButton = HIGH;
    screen.onEnter(); idle();
  }
  void idle() { screen.onIdleTick(); mockMillis += 40; screen.onIdleTick(); }
  void send(ButtonEvent::Kind kind) {
    ButtonEvent event; event.kind = kind;
    screen.onButton(event); idle();
  }
  void confirmPage() { send(ButtonEvent::Short); send(ButtonEvent::Short); send(ButtonEvent::Double); }
  void finalPage() { confirmPage(); send(ButtonEvent::Short); send(ButtonEvent::Double); }
  bool shows(const char* text) {
    for (const auto& row : rendered) if (row.text == text) return true;
    return false;
  }
};

TEST_CASE("recovery retry and help never initialize storage") {
  Fixture f;
  f.send(ButtonEvent::Double);
  CHECK_EQ(retries, 1u); CHECK_EQ(initializations, 0u);
  CHECK(f.shows(D_RECOVERY_RETRY_FAILED)); CHECK(!f.screen.ready());
  f.send(ButtonEvent::Double); f.send(ButtonEvent::Short); f.send(ButtonEvent::Long);
  CHECK(f.shows(D_RECOVERY_PRESERVED));
  f.send(ButtonEvent::Long); CHECK_EQ(initializations, 0u);
}
TEST_CASE("successful retry is the only way out without explicit initialization") {
  Fixture f; retryResult = true;
  f.send(ButtonEvent::Long); CHECK(f.screen.ready());
  f.send(ButtonEvent::Long); CHECK_EQ(retries, 1u); CHECK_EQ(initializations, 0u);
  CHECK(!f.screen.allowSleep()); CHECK(f.screen.nextScreen == nullptr);
}
TEST_CASE("both erase confirmations default to cancel") {
  Fixture f; f.confirmPage(); CHECK(f.shows(D_RECOVERY_DELETE_FILES));
  f.send(ButtonEvent::Long); CHECK(f.shows(D_RECOVERY_RETRY)); CHECK_EQ(initializations, 0u);
  f.finalPage(); CHECK(f.shows(D_RECOVERY_HOLD));
  f.send(ButtonEvent::Double); CHECK(f.shows(D_RECOVERY_RETRY)); CHECK_EQ(initializations, 0u);
}
TEST_CASE("final erase requires a fresh hold and refuses double or chord gestures") {
  Fixture f; f.finalPage(); f.send(ButtonEvent::Short);
  for (auto event : {ButtonEvent::Double, ButtonEvent::ClickHold,
                     ButtonEvent::Quad, ButtonEvent::None}) {
    f.send(event); CHECK_EQ(initializations, 0u); CHECK(!f.screen.ready());
  }
  f.send(ButtonEvent::Long); CHECK_EQ(initializations, 1u); CHECK(f.screen.ready());
  f.send(ButtonEvent::Long); CHECK_EQ(initializations, 1u);
}
TEST_CASE("triple click cancels every erase stage") {
  Fixture f; f.confirmPage(); f.send(ButtonEvent::Triple);
  CHECK(f.shows(D_RECOVERY_RETRY));
  f.finalPage(); f.send(ButtonEvent::Short); f.send(ButtonEvent::Triple);
  f.send(ButtonEvent::Long); CHECK_EQ(initializations, 0u); CHECK_EQ(retries, 1u);
}
TEST_CASE("a physically held transition press cannot advance even after classifier reset") {
  Fixture f; f.confirmPage(); f.send(ButtonEvent::Short);
  physicalButton = LOW;
  f.send(ButtonEvent::Double); // Enter final warning while still held.
  CHECK(f.shows(D_RECOVERY_HOLD));
  CHECK(!g_btns.isPressed()); // resetInputFrontend's classifier state is insufficient.
  f.send(ButtonEvent::Short); f.send(ButtonEvent::Long); f.send(ButtonEvent::VeryLong);
  CHECK_EQ(initializations, 0u);
  physicalButton = HIGH;
  f.send(ButtonEvent::Long); // Stale release event is swallowed before rearming.
  CHECK_EQ(initializations, 0u);
  f.send(ButtonEvent::Short); f.send(ButtonEvent::Long);
  CHECK_EQ(initializations, 1u);
}
TEST_CASE("presses held while the final selection is rendered need release first") {
  Fixture f; f.finalPage(); physicalButton = LOW;
  f.send(ButtonEvent::Short); f.send(ButtonEvent::Long); f.send(ButtonEvent::VeryLong);
  CHECK_EQ(initializations, 0u);
  physicalButton = HIGH; f.send(ButtonEvent::Long); CHECK_EQ(initializations, 0u);
  f.send(ButtonEvent::Long); CHECK_EQ(initializations, 1u);
}
TEST_CASE("boot input is discarded until a physical release") {
  Fixture f; physicalButton = LOW; f.screen.onEnter();
  f.send(ButtonEvent::Short); f.send(ButtonEvent::Double); CHECK_EQ(retries, 0u);
  physicalButton = HIGH; f.send(ButtonEvent::Long); CHECK_EQ(retries, 0u);
  f.send(ButtonEvent::Long); CHECK_EQ(retries, 1u); CHECK_EQ(initializations, 0u);
}
TEST_CASE("locked recovery only unlocks and resets to the safe menu") {
  Fixture f; f.finalPage(); f.send(ButtonEvent::Short); Lock::locked = true;
  f.send(ButtonEvent::Double); CHECK_EQ(initializations, 0u); CHECK(Lock::locked);
  f.send(ButtonEvent::Long); CHECK_EQ(initializations, 0u); CHECK(!Lock::locked);
  CHECK(f.shows(D_RECOVERY_RETRY));
}
TEST_CASE("initialization errors remain in recovery with the specific reason") {
  for (auto result : {FsRecoveryResult::FormatFailed, FsRecoveryResult::MountFailed,
                       FsRecoveryResult::DirectoryFailed}) {
    Fixture f; initializationResult = result;
    f.finalPage(); f.send(ButtonEvent::Short); f.send(ButtonEvent::Long);
    CHECK_EQ(initializations, 1u); CHECK(!f.screen.ready());
    CHECK(f.shows(result == FsRecoveryResult::FormatFailed ? D_RECOVERY_FORMAT_FAILED
          : result == FsRecoveryResult::MountFailed ? D_RECOVERY_MOUNT_FAILED
                                                    : D_RECOVERY_DIRECTORY_FAILED));
    f.send(ButtonEvent::Long); CHECK(f.shows(D_RECOVERY_RETRY));
  }
}

TEST_CASE("boot recovery returns only after a successful retry and restores CPU/input") {
  Fixture f;
  retrySucceedsAt = 2;
  bootEvents = {ButtonEvent::None, ButtonEvent::Double, ButtonEvent::Double, ButtonEvent::Double};
  runStorageRecovery();
  CHECK_EQ(retries, 2u); CHECK_EQ(initializations, 0u);
  CHECK_EQ(lastCpuMHz, 240u); CHECK_EQ(bootEventIndex, 4u);
  CHECK(!g_btns.isPressed());
}
TEST_CASE("boot recovery consumes lock gestures without passing them to erase") {
  Fixture f; Lock::locked = true; retryResult = true;
  bootEvents = {ButtonEvent::None, ButtonEvent::Double, ButtonEvent::Long, ButtonEvent::Double};
  runStorageRecovery();
  CHECK(!Lock::locked); CHECK_EQ(retries, 1u); CHECK_EQ(initializations, 0u);
}

TEST_CASE("release debounce cannot be bypassed by a brief high bounce") {
  Fixture f; f.finalPage();
  physicalButton = LOW; f.send(ButtonEvent::Short);
  physicalButton = HIGH; f.screen.onIdleTick();
  mockMillis += 10; f.screen.onIdleTick();
  ButtonEvent hold; hold.kind = ButtonEvent::Long;
  f.screen.onButton(hold); CHECK_EQ(initializations, 0u);
  physicalButton = LOW; f.screen.onIdleTick();
  physicalButton = HIGH; f.screen.onIdleTick();
  mockMillis += 10; f.screen.onIdleTick();
  f.screen.onButton(hold); CHECK_EQ(initializations, 0u);
  mockMillis += 25; f.screen.onIdleTick();
  f.screen.onButton(hold); CHECK_EQ(initializations, 1u);
}

TEST_CASE("selection is visibly outlined and both warnings select cancel") {
  Fixture f;
  CHECK_EQ(selectedBaseline, 52); CHECK_EQ(selectionColor, 1);
  f.confirmPage(); CHECK_EQ(selectedBaseline, 83); CHECK_EQ(selectionColor, 1);
  f.send(ButtonEvent::Short); CHECK_EQ(selectedBaseline, 103);
  f.send(ButtonEvent::Double); CHECK_EQ(selectedBaseline, 83);
  f.send(ButtonEvent::Short); CHECK_EQ(selectedBaseline, 103); CHECK_EQ(selectionColor, 1);
}

TEST_CASE("a longer final hold also confirms once after all warnings") {
  Fixture f; f.finalPage(); f.send(ButtonEvent::Short);
  f.send(ButtonEvent::VeryLong); CHECK_EQ(initializations, 1u); CHECK(f.screen.ready());
  f.send(ButtonEvent::VeryLong); CHECK_EQ(initializations, 1u);
}
