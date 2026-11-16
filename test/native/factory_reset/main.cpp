#include "test_framework.h"
#include "src/state.h"
#include "src/storage/factory_reset.h"
#include <algorithm>
TEST_CASE("factory reset retains NVS until explicit format mount and directory creation succeed") {
  FS = {};
  prefs = {};
  FS.formatOk = false;
  CHECK(factoryResetConfirmed() == FactoryResetResult::FormatFailed);
  CHECK_EQ(prefs.clears, 0u);
  CHECK(FS.calls == std::vector<std::string>({"end", "format", "mount"}));
  FS = {};
  FS.mountOk = false;
  CHECK(factoryResetConfirmed() == FactoryResetResult::MountFailed);
  CHECK_EQ(prefs.clears, 0u);
  FS = {};
  FS.mkdirOk = false;
  CHECK(factoryResetConfirmed() == FactoryResetResult::DirectoryFailed);
  CHECK_EQ(prefs.clears, 0u);
  FS = {};
  FS.existsNow = true;
  FS.validDirectory = false;
  CHECK(factoryResetConfirmed() == FactoryResetResult::DirectoryFailed);
  CHECK_EQ(prefs.clears, 0u);
  FS = {};
  prefs.clearOk = false;
  CHECK(factoryResetConfirmed() == FactoryResetResult::SettingsFailed);
  CHECK_EQ(prefs.clears, 1u);
  FS = {};
  prefs = {};
  CHECK(factoryResetConfirmed() == FactoryResetResult::Success);
  CHECK_EQ(prefs.clears, 1u);
  CHECK(std::find(FS.calls.begin(), FS.calls.end(), "unsafe-mount") == FS.calls.end());
}
