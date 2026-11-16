#pragma once
// A failed mount never authorizes erasing. Retry is strictly non-destructive.
bool fsRecoveryRetry();
// Only the final, explicit physical confirmation may call this. A newly
// readable filesystem is returned untouched. Device NVS is never cleared.
bool fsRecoveryReinitializeConfirmed();

// Shared explicit-erasure primitive. Unlike recovery retry, this deliberately
// formats the existing LittleFS volume. It never clears device settings.
enum class FsInitializationResult {Ready,FormatFailed,MountFailed,DirectoryFailed};
FsInitializationResult fsReinitializeStorageConfirmed();
