import subprocess

Import("env")  # noqa: F821 — provided by PlatformIO at script-load time


def _git(*args):
    try:
        out = subprocess.check_output(
            ["git", *args],
            stderr=subprocess.DEVNULL,
            cwd=env["PROJECT_DIR"],  # noqa: F821
        )
        return out.decode().strip()
    except Exception:
        return ""


def build_id():
    sha = _git("rev-parse", "--short", "HEAD")
    if not sha:
        return None
    dirty = "-dirty" if _git("status", "--porcelain") else ""
    return sha + dirty


value = build_id()
if value:
    env.Append(CPPDEFINES=[("BUILD_GIT_HASH", env.StringifyMacro(value))])  # noqa: F821
    print(f"build_info.py: BUILD_GIT_HASH={value}")
else:
    # No git checkout (zip download, CI without history, etc.). Force
    # DEBUG_BUILD=0 so the library-screen header shows plain "Pala One"
    # instead of "Pala One nogit". config.h's fallback BUILD_GIT_HASH
    # = "unknown" still feeds FW_BUILD for About / web UI.
    env.Append(CPPDEFINES=[("DEBUG_BUILD", 0)])  # noqa: F821
    print("build_info.py: git unavailable, forcing DEBUG_BUILD=0")
