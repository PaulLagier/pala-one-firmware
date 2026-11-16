#include "src/storage/fs_recovery.h"
#include "src/storage/fs_util.h"

bool fsRecoveryRetry() {
  FS.end();
  return fsBegin();
}

FsRecoveryResult fsRecoveryInitializeConfirmed() {
  FS.end();
  if (fsBegin()) return FsRecoveryResult::Ready;
  FS.end();
  if (!FS.format()) return FsRecoveryResult::FormatFailed;
  if (!FS.begin(false)) return FsRecoveryResult::MountFailed;
  if (!FS.exists("/books") && !FS.mkdir("/books"))
    return FsRecoveryResult::DirectoryFailed;
  File books = FS.open("/books");
  const bool ready = books && books.isDirectory();
  books.close();
  return ready ? FsRecoveryResult::Ready : FsRecoveryResult::DirectoryFailed;
}
