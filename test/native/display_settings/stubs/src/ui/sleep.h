#pragma once
namespace Sleep {
inline int seconds = 120;
inline int idleTimeoutSecs() { return seconds; }
inline void setIdleTimeout(int value) { seconds = value; }
} // namespace Sleep
