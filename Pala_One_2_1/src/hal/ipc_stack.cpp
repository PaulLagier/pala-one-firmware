#include "src/hal/ipc_stack.h"
#include "src/pure/ipc_stack_policy.h"
#if defined(PALA_IPC_STACK_BYTES)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
namespace {TaskHandle_t handles[2]={};uint32_t allocated[2]={};}
extern "C" BaseType_t __real_xTaskCreatePinnedToCore(TaskFunction_t,const char*,uint32_t,void*,UBaseType_t,TaskHandle_t*,BaseType_t);
extern "C" __attribute__((used,noinline)) BaseType_t __wrap_xTaskCreatePinnedToCore(TaskFunction_t entry,const char* name,uint32_t depth,void* argument,UBaseType_t priority,TaskHandle_t* output,BaseType_t core){
  const bool sdk=output&&core>=0&&core<2&&argument==reinterpret_cast<void*>(uintptr_t(core))&&priority==configMAX_PRIORITIES-1;
  const uint32_t bytes=IpcStackPolicy::bytes(name,core,sdk,depth,PALA_IPC_STACK_BYTES);
  const BaseType_t result=__real_xTaskCreatePinnedToCore(entry,name,bytes,argument,priority,output,core);
  if(sdk&&IpcStackPolicy::namedTask(name,core)&&result==pdPASS){handles[core]=*output;allocated[core]=bytes;}
  return result;
}
uint32_t RuntimeStack::ipcBytes(int core){return core>=0&&core<2?allocated[core]:0;}
uint32_t RuntimeStack::ipcFreeBytes(int core){return core>=0&&core<2&&handles[core]?uxTaskGetStackHighWaterMark(handles[core]):0;}
#else
// The unmodified Arduino IDE build has no linker wrapping. Keep its ABI
// buildable; the local PlatformIO artifacts opt into the measured guard.
uint32_t RuntimeStack::ipcBytes(int){return 0;}
uint32_t RuntimeStack::ipcFreeBytes(int){return 0;}
#endif
