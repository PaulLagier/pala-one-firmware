#pragma once
// Requires the reset form's explicit confirmation. Success requires a reboot.
enum class FactoryResetResult {
  Success,
  FormatFailed,
  MountFailed,
  DirectoryFailed,
  SettingsFailed
};
FactoryResetResult factoryResetConfirmed();
