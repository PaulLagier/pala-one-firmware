#pragma once
namespace Sleep {
inline int seconds=120; inline bool noScreen=false,lock=false;
inline int idleTimeoutSecs(){return seconds;}
inline bool noScreensaver(){return noScreen;}
inline bool lockOnSleep(){return lock;}
inline void setIdleTimeout(int value){seconds=value;}
inline void setNoScreensaver(bool value){noScreen=value;}
inline void setLockOnSleep(bool value){lock=value;}
}
