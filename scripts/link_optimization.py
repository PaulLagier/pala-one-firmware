Import("env")
# Framework setup owns LINKFLAGS; append matching LTO flags afterwards.
if env["PIOENV"].startswith("companion-"):
    env.Append(LINKFLAGS=["-flto"])

# The Arduino SDK archive creates IPC tasks before setup(). Changing a local
# CONFIG_* define cannot resize those precompiled creation calls. Wrap their
# task-creation ABI and leave every non-IPC task untouched.
ipc_bytes = int(env.GetProjectOption("custom_ipc_stack_bytes", "0"))
if ipc_bytes:
    if ipc_bytes < 2048:
        raise ValueError("IPC callback stack headroom must be at least 2048 bytes")
    env.Append(LINKFLAGS=["-Wl,--wrap=xTaskCreatePinnedToCore", "-Wl,--undefined=__wrap_xTaskCreatePinnedToCore"])
