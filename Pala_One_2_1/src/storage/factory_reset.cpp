#include "src/storage/factory_reset.h"
#include "src/state.h"

FactoryResetResult factoryResetConfirmed() {
  FS.end();
  if (!FS.format()) {
    FS.begin(false);
    return FactoryResetResult::FormatFailed;
  }
  // Never trigger a second implicit format after a failed explicit operation.
  if (!FS.begin(false))
    return FactoryResetResult::MountFailed;
  if (!FS.exists("/books") && !FS.mkdir("/books"))
    return FactoryResetResult::DirectoryFailed;
  auto directory = FS.open("/books", "r");
  const bool ready = directory && directory.isDirectory();
  directory.close();
  if (!ready)
    return FactoryResetResult::DirectoryFailed;
  // Retain pairing/settings/progress if filesystem initialization failed.
  if (!prefs.clear())
    return FactoryResetResult::SettingsFailed;
  return FactoryResetResult::Success;
}
