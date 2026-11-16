#pragma once
namespace WifiProvisioning {
inline unsigned localBegins=0,localPolls=0;
inline void beginLocalSetup(){++localBegins;}
inline void loopLocalSetup(){++localPolls;}
}
