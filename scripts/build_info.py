import subprocess
import hashlib
import sys
from pathlib import Path

Import("env")  # noqa: F821 — provided by PlatformIO at script-load time


# Regenerate only changed locale artifacts before computing the source hash.
_locale_root = Path(env["PROJECT_DIR"])
subprocess.check_call([sys.executable, str(_locale_root / "scripts/generate_locales.py"),
    "--catalogs", str(_locale_root / "Pala_One_2_1/src/lang/catalogs"),
    "--out", str(_locale_root / "Pala_One_2_1/src/lang"), "--quiet"])
subprocess.check_call([sys.executable, str(_locale_root / "scripts/generate_web_assets.py"),
    "--assets", str(_locale_root / "Pala_One_2_1/src/web/assets"),
    "--out", str(_locale_root / "Pala_One_2_1/src/web"), "--quiet"])


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
is_local_release = env.GetProjectOption("custom_local_release", "no") == "yes"
if env["PIOENV"].endswith("-local") or is_local_release:
    root = Path(env["PROJECT_DIR"])
    source_files = [root / "platformio.ini", root / "Pala_One_2_1/Pala_One_2_1.ino"]
    source_files += list((root / "Pala_One_2_1").glob("*.h"))
    source_files += [p for p in (root / "Pala_One_2_1/src").rglob("*") if p.suffix in (".h", ".cpp")]
    source_files += [p for p in (root / "lib").rglob("*") if p.suffix in (".h", ".cpp")]
    digest = hashlib.sha256()
    for source in sorted(source_files):
        digest.update(str(source.relative_to(root)).encode())
        digest.update(source.read_bytes())
    source_id = digest.hexdigest()[:8]
    ver = ("local-rc-" if is_local_release else "local-ui2-") + source_id
    print(f"build_info.py: local source fingerprint={source_id}")
if ver:
    env.Append(CPPDEFINES=[("FW_VERSION", env.StringifyMacro(ver))])  # noqa: F821
    print(f"build_info.py: FW_VERSION={ver}")
else:
    # No git available — config.h's fallback "dev" wins. Same situations
    # as above (zip download, etc.).
    print("build_info.py: git unavailable, leaving FW_VERSION to config.h fallback")
