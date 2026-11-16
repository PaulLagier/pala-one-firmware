Import("env")
# Match the LTO compile flags after framework linker setup.
env.Append(LINKFLAGS=["-flto"])
# The pinned SDK creates radio IPC tasks from precompiled code. Only those
# exact creation calls get extra stack; application task sizes stay intact.
ipc_bytes = int(env.GetProjectOption("custom_ipc_stack_bytes", "0"))
if ipc_bytes:
    if ipc_bytes < 2048:
        raise ValueError("IPC callback stack headroom must be at least 2048 bytes")
    env.Append(LINKFLAGS=["-Wl,--wrap=xTaskCreatePinnedToCore", "-Wl,--undefined=__wrap_xTaskCreatePinnedToCore"])
