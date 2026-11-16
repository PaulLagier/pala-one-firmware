#pragma once
#include "src/ui/screen.h"
#include "src/storage/fs_recovery.h"

// Boot-only screen. Normal menus, radios and storage users are not started
// until it reports a mounted volume. No navigation to the library is possible.
class StorageRecoveryScreen : public Screen {
public:
  struct Actions {
    bool (*retry)();
    FsRecoveryResult (*initializeConfirmed)();
  };
  StorageRecoveryScreen();
  explicit StorageRecoveryScreen(Actions actions) : actions_(actions) {}
  void onEnter() override;
  void onButton(const ButtonEvent& event) override;
  void onIdleTick() override;
  void draw() override;
  bool allowSleep() const override { return false; }
  bool ready() const { return ready_; }
private:
  enum class Page { Menu, Help, Confirm, Final, RetryFailed, InitializeFailed };
  Actions actions_;
  Page page_ = Page::Menu;
  int selected_ = 0;
  bool ready_ = false;
  bool awaitRelease_ = true;
  bool releaseSeen_ = false;
  uint32_t releaseSinceMs_ = 0;
  FsRecoveryResult failure_ = FsRecoveryResult::MountFailed;
  void transition(Page page, int selected = 0);
  void working(const char* message);
};

// Blocking boot recovery: consumes input locally, never enters normal loop().
// Returns only once storage is mounted. No USB/Wi-Fi provisioning is started.
void runStorageRecovery();
