#pragma once
// Caller must already have explicit user confirmation. Storage failure leaves
// NVS untouched; after success the caller must reboot to discard all RAM caches.
enum class FactoryResetResult {Success,FormatFailed,MountFailed,DirectoryFailed,SettingsFailed};
FactoryResetResult factoryResetConfirmed();
