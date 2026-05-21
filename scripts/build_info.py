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
    sha = _git("rev-parse", "--short", "HEAD") or "nogit"
    dirty = "-dirty" if _git("status", "--porcelain") else ""
    return sha + dirty


value = build_id()
env.Append(CPPDEFINES=[("BUILD_GIT_HASH", env.StringifyMacro(value))])  # noqa: F821
print(f"build_info.py: BUILD_GIT_HASH={value}")
