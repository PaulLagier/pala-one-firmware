#include "src/storage/fs_recovery.h"
#include "src/storage/fs_util.h"
#include "src/state.h"

bool fsRecoveryRetry(){FS.end();return fsBegin();}
FsInitializationResult fsReinitializeStorageConfirmed(){
  FS.end();
  if(!FS.format())return FsInitializationResult::FormatFailed;
  if(!FS.begin(false))return FsInitializationResult::MountFailed;
  if(!FS.exists("/books")&&!FS.mkdir("/books"))return FsInitializationResult::DirectoryFailed;
  File books=FS.open("/books");const bool ready=books&&books.isDirectory();books.close();
  return ready?FsInitializationResult::Ready:FsInitializationResult::DirectoryFailed;
}
bool fsRecoveryReinitializeConfirmed(){
  // Recheck immediately before destruction: if storage has recovered, keep it.
  if(FS.begin(false))return true;
  return fsReinitializeStorageConfirmed()==FsInitializationResult::Ready;
}
