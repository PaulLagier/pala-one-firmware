#pragma once
namespace BookmarkSettings {
inline bool toggle = false;
inline bool toggleEnabled() { return toggle; }
inline bool setToggleEnabled(bool enabled) {
  toggle = enabled;
  return true;
}
} // namespace BookmarkSettings
