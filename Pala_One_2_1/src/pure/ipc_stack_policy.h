#pragma once
#include <stdint.h>
namespace IpcStackPolicy {
inline bool namedTask(const char* name,int core){return core>=0&&core<2&&name&&name[0]=='i'&&name[1]=='p'&&name[2]=='c'&&name[3]==char('0'+core)&&name[4]==0;}
inline uint32_t bytes(const char* name,int core,bool sdkCreation,uint32_t requested,uint32_t minimum){
  return requested&&sdkCreation&&namedTask(name,core)&&requested<minimum?minimum:requested;
}
}
