#pragma once
namespace ScreenSettings {
inline bool flipped=false;
inline bool isScreenFlipped(){return flipped;}
inline void setScreenRotation(bool value){flipped=value;}
}
