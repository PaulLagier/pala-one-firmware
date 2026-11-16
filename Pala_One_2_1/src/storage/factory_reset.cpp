#include "src/storage/factory_reset.h"
#include "src/storage/fs_recovery.h"
#include "src/state.h"

FactoryResetResult factoryResetConfirmed(){
  switch(fsReinitializeStorageConfirmed()){
    case FsInitializationResult::FormatFailed:return FactoryResetResult::FormatFailed;
    case FsInitializationResult::MountFailed:return FactoryResetResult::MountFailed;
    case FsInitializationResult::DirectoryFailed:return FactoryResetResult::DirectoryFailed;
    case FsInitializationResult::Ready:break;
    default:return FactoryResetResult::DirectoryFailed;
  }
  // Do not discard pairing/preferences/reading metadata until storage is ready.
  if(!prefs.clear())return FactoryResetResult::SettingsFailed;
  return FactoryResetResult::Success;
}
