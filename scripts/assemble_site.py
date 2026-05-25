#!/usr/bin/env python3
"""Assemble the ESP Web Tools installer site bundle.

Reads the per-env PlatformIO build outputs plus install/ static assets and
writes a self-contained directory (default `site/`) that can be served by
any static HTTP server — locally for testing or as a GitHub Pages artifact.

Single source of truth for the bundle layout: this script is invoked both
from the deploy-installer GitHub workflow and by developers wanting to
exercise the installer page locally.

Channel layout
--------------
When `--channel <name>` is given (e.g. "stable", "dev"), the script
publishes a two-channel layout suitable for the gh-pages branch:

    site/
    ├── index.html              # landing page: pick stable or dev
    ├── connected.html          # Improv post-provisioning landing (channel-agnostic)
    └── <channel>/
        ├── index.html          # the ESP Web Tools installer page
        ├── manifest-v1_{1,2}-{en,es}.json   # 4 manifests (board x language)
        ├── firmware-v1_{1,2}-{en,es}.bin    # 4 firmware images
        └── bootloader.bin / partitions.bin / boot_app0.bin

connected.html lives at the gh-pages root, not under a channel, because
its content is identical for stable + dev and the firmware-baked redirect
URL has no way to know which channel installed it.

The workflow uploads this to `gh-pages` with keep_files=true so the
*other* channel's directory survives untouched.

Without `--channel`, the installer files are written flat to `site/`
(the historical layout — handy for `python -m http.server site/`
during local iteration).
"""

import argparse
import json
import re
import shutil
import sys
from pathlib import Path
from typing import Optional


# Board / language metadata — used to generate variant HTML blocks
# dynamically so the installer page only shows buttons for firmware
# that was actually built.
_BOARD_ORDER = ["v1_2", "v1_1"]
_BOARD_META = {
    "v1_2": ("V1.2 board",
             'Newer Heltec Wireless Paper. If your board\'s silkscreen says '
             '<code>V1.2</code>, pick this.'),
    "v1_1": ("V1.1 board",
             "Older revision of the Heltec Wireless Paper."),
}
_LANG_ORDER = [("en", "English"), ("es", "Español")]


def _variant_html(available: set) -> str:
    """Return ``<div class="variant">`` blocks for available firmware."""
    sections = []
    for board_id in _BOARD_ORDER:
        title, desc = _BOARD_META[board_id]
        buttons = []
        for lang_id, lang_label in _LANG_ORDER:
            if f"{board_id}-{lang_id}" not in available:
                continue
            manifest = f"manifest-{board_id}-{lang_id}.json"
            version = board_id.replace("v", "V").replace("_", ".")
            buttons.append(
                f'      <esp-web-install-button manifest="{manifest}">\n'
                f'        <button slot="activate">Install {version} — {lang_label}</button>\n'
                f'        <span slot="unsupported">\n'
                f'          Your browser doesn\'t support Web Serial. Use desktop Chrome, Edge, or Opera.\n'
                f'        </span>\n'
                f'        <span slot="not-allowed">Open this page over HTTPS.</span>\n'
                f'      </esp-web-install-button>')
        if not buttons:
            continue
        sections.append(
            f'  <div class="variant">\n'
            f'    <h3>{title}</h3>\n'
            f'    <p>{desc}</p>\n'
            f'    <div class="lang-row">\n'
            + '\n'.join(buttons) + '\n'
            f'    </div>\n'
            f'  </div>')
    return '\n\n'.join(sections)


def require_file(p: Path) -> Path:
    if not p.is_file():
        sys.exit(f"missing: {p}\nRun `pio run` for all four leaf envs first:\n"
                 f"  pio run -e wireless-paper-v1_1-en\n"
                 f"  pio run -e wireless-paper-v1_1-es\n"
                 f"  pio run -e wireless-paper-v1_2-en\n"
                 f"  pio run -e wireless-paper-v1_2-es")
    return p


def find_boot_app0() -> Path:
    p = (Path.home() / ".platformio" / "packages"
         / "framework-arduinoespressif32" / "tools" / "partitions"
         / "boot_app0.bin")
    if not p.is_file():
        sys.exit(f"boot_app0.bin not found at {p}\nBuild with PIO at least "
                 f"once so the framework package is installed.")
    return p


def write_installer_bundle(out: Path, repo: Path, version: str,
                           channel: Optional[str], *,
                           lenient: bool = False) -> None:
    """Populate `out` with the installer page + firmware artefacts.

    When *lenient* is True, missing firmware variants are silently
    skipped instead of aborting — useful for local dev when only one
    board/language combination was built.
    """
    leaves = [
        ("wireless-paper-v1_1-en", "firmware-v1_1-en.bin"),
        ("wireless-paper-v1_1-es", "firmware-v1_1-es.bin"),
        ("wireless-paper-v1_2-en", "firmware-v1_2-en.bin"),
        ("wireless-paper-v1_2-es", "firmware-v1_2-es.bin"),
    ]
    pio  = repo / ".pio" / "build"
    inst = repo / "install"

    out.mkdir(parents=True, exist_ok=True)

    # Probe which firmware variants were actually built.
    available = set()      # e.g. {"v1_2-en", "v1_1-es"}
    first_built_env = None
    for env_name, out_name in leaves:
        fw = pio / env_name / "firmware.bin"
        if fw.is_file():
            key = out_name.replace("firmware-", "").replace(".bin", "")
            available.add(key)
            if first_built_env is None:
                first_built_env = pio / env_name
        elif not lenient:
            require_file(fw)   # exits with instructions

    if not available:
        if not lenient:
            sys.exit("No firmware variants found. Run `pio run` first.")

    # bootloader / partitions / boot_app0 are byte-identical across all
    # envs (same chip + partition table). Take them from the first built env.
    if first_built_env:
        shutil.copy(require_file(first_built_env / "bootloader.bin"), out / "bootloader.bin")
        shutil.copy(require_file(first_built_env / "partitions.bin"), out / "partitions.bin")
        shutil.copy(find_boot_app0(),                                  out / "boot_app0.bin")

    for env_name, out_name in leaves:
        key = out_name.replace("firmware-", "").replace(".bin", "")
        if key not in available:
            continue
        shutil.copy(pio / env_name / "firmware.bin", out / out_name)

    # Inject channel, version, and variant blocks into the template.
    index_html = require_file(inst / "index.html").read_text(encoding="utf-8")
    index_html = index_html.replace("{{CHANNEL}}", channel or "local")
    index_html = index_html.replace("{{VERSION}}", version)
    index_html = index_html.replace("{{VARIANTS}}", _variant_html(available))
    (out / "index.html").write_text(index_html, encoding="utf-8")

    for name in ("manifest-v1_1-en.json", "manifest-v1_1-es.json",
                 "manifest-v1_2-en.json", "manifest-v1_2-es.json"):
        key = name.replace("manifest-", "").replace(".json", "")
        if key not in available:
            continue
        data = json.loads(require_file(inst / name).read_text())
        data["version"] = version
        (out / name).write_text(json.dumps(data, indent=2) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--version", default="dev-local",
                        help="Version string injected into manifest JSON "
                             "(default: dev-local).")
    parser.add_argument("--out", default="site",
                        help="Output directory (default: site).")
    parser.add_argument("--channel", default=None,
                        help="Channel name (e.g. 'stable', 'dev'). When "
                             "set, the installer files are placed under "
                             "site/<channel>/ and a landing index.html is "
                             "written at site/ that links to both channels. "
                             "Omit for a flat local-dev layout.")
    parser.add_argument("--lenient", action="store_true",
                        help="Skip missing firmware variants instead of "
                             "aborting.  Useful when only one board/language "
                             "combination was built.")
    args = parser.parse_args()

    if args.channel is not None and not re.fullmatch(r"[a-z0-9_-]+", args.channel):
        sys.exit(f"invalid --channel {args.channel!r}: must match [a-z0-9_-]+")

    repo = Path(__file__).resolve().parents[1]
    out  = (repo / args.out).resolve()
    inst = repo / "install"

    if args.channel:
        out.mkdir(parents=True, exist_ok=True)
        # Root landing page + Improv post-provisioning page — same content
        # regardless of which channel is being deployed, so safe to rewrite
        # on every run.
        shutil.copy(require_file(inst / "landing.html"),   out / "index.html")
        shutil.copy(require_file(inst / "connected.html"), out / "connected.html")
        bundle_dir = out / args.channel
        write_installer_bundle(bundle_dir, repo, args.version, args.channel,
                               lenient=args.lenient)
        print(f"Assembled {args.channel} installer bundle -> {bundle_dir}  "
              f"(version: {args.version})")
        print(f"Landing page    -> {out / 'index.html'}")
        print(f"Connected page  -> {out / 'connected.html'}")
        for p in sorted(bundle_dir.iterdir()):
            print(f"  {args.channel}/{p.name:30s} {p.stat().st_size:>10d} bytes")
    else:
        write_installer_bundle(out, repo, args.version, None,
                               lenient=args.lenient)
        # Flat local-dev mode: connected.html lives alongside the installer.
        shutil.copy(require_file(inst / "connected.html"), out / "connected.html")
        print(f"Assembled installer bundle -> {out}  (version: {args.version})")
        for p in sorted(out.iterdir()):
            print(f"  {p.name:30s} {p.stat().st_size:>10d} bytes")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
