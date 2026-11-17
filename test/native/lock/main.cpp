#include "test_framework.h"
#include "src/state.h"
#include "src/ui/lock.h"
#include "src/ui/reader_actions.h"
LockPrefs prefs;
namespace {
const ButtonEvent::Kind kinds[] = {
    ButtonEvent::None, ButtonEvent::Short, ButtonEvent::Double,   ButtonEvent::Triple,
    ButtonEvent::Quad, ButtonEvent::Long,  ButtonEvent::VeryLong, ButtonEvent::ClickHold};
void reset(bool legacy) {
  prefs.values.clear();
  Gestures::loadSettings();
  Gestures::setLegacyControls(legacy);
  Gestures::setActionShort(ACTION_NONE);
  Gestures::setActionDouble(ACTION_NONE);
  Gestures::setActionTriple(ACTION_NONE);
  Gestures::setActionQuad(ACTION_NONE);
  Gestures::setActionLong(ACTION_NONE);
  Gestures::setActionExtraLong(ACTION_NONE);
  Gestures::setActionClickHold(ACTION_NONE);
  Lock::loadSettings();
}
} // namespace
TEST_CASE("Custom lock accepts exactly the configured gesture after saved settings reload") {
  for (auto target :
       {ButtonEvent::Short, ButtonEvent::Double, ButtonEvent::Triple, ButtonEvent::Quad,
        ButtonEvent::Long, ButtonEvent::VeryLong, ButtonEvent::ClickHold}) {
    reset(false);
    switch (target) {
    case ButtonEvent::Short:
      Gestures::setActionShort(ACTION_LOCK);
      break;
    case ButtonEvent::Double:
      Gestures::setActionDouble(ACTION_LOCK);
      break;
    case ButtonEvent::Triple:
      Gestures::setActionTriple(ACTION_LOCK);
      break;
    case ButtonEvent::Quad:
      Gestures::setActionQuad(ACTION_LOCK);
      break;
    case ButtonEvent::Long:
      Gestures::setActionLong(ACTION_LOCK);
      break;
    case ButtonEvent::VeryLong:
      Gestures::setActionExtraLong(ACTION_LOCK);
      break;
    default:
      Gestures::setActionClickHold(ACTION_LOCK);
      break;
    }
    Lock::engage();
    Gestures::loadSettings();
    Lock::loadSettings();
    REQUIRE(Lock::isLocked());
    for (auto kind : kinds)
      CHECK(Lock::isUnlockGesture(ButtonEvent{kind}) == (kind == target));
    Lock::disengage();
    Lock::loadSettings();
    CHECK(!Lock::isLocked());
  }
}
TEST_CASE("Multiple assigned lock gestures all unlock and unrelated gestures do not") {
  reset(false);
  Gestures::setActionLong(ACTION_LOCK);
  Gestures::setActionClickHold(ACTION_LOCK);
  CHECK(Lock::isUnlockGesture(ButtonEvent{ButtonEvent::Long}));
  CHECK(Lock::isUnlockGesture(ButtonEvent{ButtonEvent::ClickHold}));
  CHECK(!Lock::isUnlockGesture(ButtonEvent{ButtonEvent::VeryLong}));
}
TEST_CASE("Legacy controls and unassigned automatic locks retain safe hold fallback") {
  for (bool legacy : {true, false}) {
    reset(legacy);
    for (auto kind : kinds)
      CHECK(Lock::isUnlockGesture(ButtonEvent{kind}) ==
            (kind == ButtonEvent::Long || kind == ButtonEvent::VeryLong ||
             kind == ButtonEvent::ClickHold));
  }
}

TEST_CASE("Optional quadruple binding defaults to none and persists public action IDs") {
  reset(false);
  CHECK_EQ(Gestures::actionQuad(), ACTION_NONE);
  for (int action = ACTION_NONE; action <= ACTION_ROTATE; ++action) {
    Gestures::setActionQuad(ButtonAction(action));
    Gestures::loadSettings();
    CHECK_EQ(int(Gestures::actionFor(ButtonEvent::Quad)), action);
    CHECK_EQ(Gestures::actionFor(ButtonEvent::None), ACTION_NONE);
  }
  Gestures::setActionQuad(ButtonAction(99));
  CHECK_EQ(Gestures::actionQuad(), ACTION_NONE);
}
TEST_CASE("Hiding the sleep badge does not unlock or change saved lock bindings") {
  reset(false);
  CHECK(Lock::iconEnabled());
  Gestures::setActionQuad(ACTION_LOCK);
  Lock::engage();
  Lock::setIconEnabled(false);
  Lock::loadSettings();
  CHECK(!Lock::iconEnabled());
  CHECK(Lock::isLocked());
  CHECK(Lock::isUnlockGesture(ButtonEvent{ButtonEvent::Quad}));
  CHECK(!Lock::isUnlockGesture(ButtonEvent{ButtonEvent::Long}));
}
