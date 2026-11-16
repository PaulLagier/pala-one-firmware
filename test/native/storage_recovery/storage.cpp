#include "test_framework.h"
#include "src/storage/fs_util.h"
#include "src/storage/fs_recovery.h"

TEST_CASE("failed boot mount never formats storage") {
  FS = {}; FS.mounts = {false};
  CHECK(!fsBegin()); CHECK_EQ(FS.begins, 1u); CHECK_EQ(FS.formats, 0u);
  CHECK_EQ(FS.mkdirs, 0u);
}
TEST_CASE("successful boot mount is unchanged") {
  FS = {}; FS.mounts = {true};
  CHECK(fsBegin()); CHECK_EQ(FS.begins, 1u); CHECK_EQ(FS.formats, 0u);
}
TEST_CASE("recovery retry unmounts and never formats") {
  for (bool mounted : {false, true}) {
    FS = {}; FS.mounts = {mounted};
    CHECK_EQ(fsRecoveryRetry(), mounted);
    CHECK_EQ(FS.ends, 1u); CHECK_EQ(FS.formats, 0u); CHECK_EQ(FS.mkdirs, 0u);
  }
}
TEST_CASE("confirmed recovery keeps a volume that has become readable") {
  FS = {}; FS.mounts = {true};
  CHECK(fsRecoveryInitializeConfirmed() == FsRecoveryResult::Ready);
  CHECK_EQ(FS.formats, 0u); CHECK_EQ(FS.mkdirs, 0u);
}
TEST_CASE("confirmed recovery initializes a new volume and verifies books directory") {
  FS = {}; FS.mounts = {false, true};
  CHECK(fsRecoveryInitializeConfirmed() == FsRecoveryResult::Ready);
  CHECK_EQ(FS.ends, 2u); CHECK_EQ(FS.formats, 1u); CHECK_EQ(FS.mkdirs, 1u);
}
TEST_CASE("confirmed recovery distinguishes format and mount failures") {
  FS = {}; FS.mounts = {false}; FS.formatResult = false;
  CHECK(fsRecoveryInitializeConfirmed() == FsRecoveryResult::FormatFailed);
  CHECK_EQ(FS.begins, 1u); CHECK_EQ(FS.formats, 1u); CHECK_EQ(FS.mkdirs, 0u);
  FS = {}; FS.mounts = {false, false};
  CHECK(fsRecoveryInitializeConfirmed() == FsRecoveryResult::MountFailed);
  CHECK_EQ(FS.begins, 2u); CHECK_EQ(FS.formats, 1u); CHECK_EQ(FS.mkdirs, 0u);
}
TEST_CASE("confirmed recovery distinguishes missing unreadable and non-directory books") {
  FS = {}; FS.mounts = {false, true}; FS.mkdirResult = false;
  CHECK(fsRecoveryInitializeConfirmed() == FsRecoveryResult::DirectoryFailed);
  FS = {}; FS.mounts = {false, true}; FS.booksReadable = false;
  CHECK(fsRecoveryInitializeConfirmed() == FsRecoveryResult::DirectoryFailed);
  FS = {}; FS.mounts = {false, true}; FS.booksExists = true; FS.booksDirectory = false;
  CHECK(fsRecoveryInitializeConfirmed() == FsRecoveryResult::DirectoryFailed);
  CHECK_EQ(FS.mkdirs, 0u);
}
