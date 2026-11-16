#pragma once

// Recovery affects LittleFS only. Device settings and pairing in NVS are kept.
enum class FsRecoveryResult { Ready, FormatFailed, MountFailed, DirectoryFailed };
bool fsRecoveryRetry();
// Call only after the physical recovery UI's final erase confirmation.
// Recheck mount first: a newly readable volume must never be formatted.
FsRecoveryResult fsRecoveryInitializeConfirmed();
