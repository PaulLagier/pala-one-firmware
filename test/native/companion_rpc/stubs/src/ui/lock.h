#pragma once
namespace Lock {
inline bool badge = true;
inline bool iconEnabled() { return badge; }
inline void setIconEnabled(bool value) { badge = value; }
} // namespace Lock
