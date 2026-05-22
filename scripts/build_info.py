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


def fw_version():
    # `git describe --tags --always --dirty`:
    #   tagged commit       -> "v3.0"
    #   N commits past tag  -> "v3.0-3-gabc1234"
    #   no tags in history  -> "abc1234"   (just the sha, via --always)
    #   uncommitted changes -> "...-dirty"
    #
    # Letting git describe own the version string means a tag push is the
    # one place a maintainer has to bump anything — no more forgetting to
    # edit config.h alongside the release. The CI workflow needs
    # `fetch-depth: 0` on actions/checkout so tags are visible.
    return _git("describe", "--tags", "--always", "--dirty") or None


sha = build_id()
if sha:
    env.Append(CPPDEFINES=[("BUILD_GIT_HASH", env.StringifyMacro(sha))])  # noqa: F821
    print(f"build_info.py: BUILD_GIT_HASH={sha}")
else:
    # No git checkout (zip download, CI without history, etc.). Force
    # DEBUG_BUILD=0 so the library-screen header shows plain "Pala One"
    # instead of "Pala One unknown". config.h's fallback BUILD_GIT_HASH
    # = "unknown" remains the macro value for anything else that needs it.
    env.Append(CPPDEFINES=[("DEBUG_BUILD", 0)])  # noqa: F821
    print("build_info.py: git unavailable, forcing DEBUG_BUILD=0")

ver = fw_version()
if ver:
    env.Append(CPPDEFINES=[("FW_VERSION", env.StringifyMacro(ver))])  # noqa: F821
    print(f"build_info.py: FW_VERSION={ver}")
else:
    # No git available — config.h's fallback "dev" wins. Same situations
    # as above (zip download, etc.).
    print("build_info.py: git unavailable, leaving FW_VERSION to config.h fallback")
