#pragma once
namespace Sleep {
inline int seconds=120;
inline bool noImage=false,locked=false;
inline int idleTimeoutSecs(){return seconds;}
inline void setIdleTimeout(int value){seconds=value;}
inline bool noScreensaver(){return noImage;}
inline void setNoScreensaver(bool value){noImage=value;}
inline bool lockOnSleep(){return locked;}
inline void setLockOnSleep(bool value){locked=value;}
}
