#!/usr/bin/env python3
"""Build the current checkout and serve the ESP Web Tools installer locally.

Builds the selected PlatformIO firmware environment(s), assembles the
installer page, and starts a local HTTP server so you can flash directly
from Chrome/Edge/Opera via Web Serial.

The served page includes a branch selector (local-dev mode only) that
lets you pick any local or remote branch, build it in an isolated git
worktree, and flash — without leaving the browser.

Usage:
    python scripts/local_flash.py                                # serve, build from UI
    python scripts/local_flash.py --build                        # build first, then serve
    python scripts/local_flash.py --build --env wireless-paper-v1_2-en  # build one variant
    python scripts/local_flash.py --port 9000                    # custom port
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import threading
import webbrowser
from functools import partial
from http.server import HTTPServer, SimpleHTTPRequestHandler
from pathlib import Path

ALL_ENVS = [
    "wireless-paper-v1_1-en",
    "wireless-paper-v1_1-es",
    "wireless-paper-v1_2-en",
    "wireless-paper-v1_2-es",
]

# ------------------------------------------------------------------
# Build state — shared between the HTTP handler and the build thread
# ------------------------------------------------------------------
_build_lock = threading.Lock()
_build_state = {
    "status": "idle",
    "branch": "",
    "envs": "",
    "log": "",
    "error": "",
}


def get_version(repo: Path, branch: str = None) -> str:
    """Derive a version label from git state."""
    try:
        if branch is None:
            branch = subprocess.run(
                ["git", "rev-parse", "--abbrev-ref", "HEAD"],
                capture_output=True, text=True, cwd=repo,
            ).stdout.strip()
        sha = subprocess.run(
            ["git", "rev-parse", "--short", branch],
            capture_output=True, text=True, cwd=repo,
        ).stdout.strip()
        if branch and sha:
            return f"{branch} ({sha})"
    except FileNotFoundError:
        pass
    return "local"


def _list_branches(repo: Path) -> list:
    """Return [{name, current}, ...] for all local + remote branches."""
    current = subprocess.run(
        ["git", "rev-parse", "--abbrev-ref", "HEAD"],
        capture_output=True, text=True, cwd=repo,
    ).stdout.strip()

    result = subprocess.run(
        ["git", "branch", "-a", "--format=%(refname:short)"],
        capture_output=True, text=True, cwd=repo,
    )
    branches = []
    seen = set()
    for name in result.stdout.strip().splitlines():
        name = name.strip()
        if not name or "HEAD" in name or "->" in name:
            continue
        if name in seen:
            continue
        seen.add(name)
        branches.append({"name": name, "current": name == current})
    return branches


def _parse_github_url(url: str):
    """Parse a GitHub URL into (clone_url, branch).

    Accepts:
      https://github.com/owner/repo/tree/branch-name
      https://github.com/owner/repo  (branch defaults to HEAD)
      owner/repo#branch
      owner/repo branch
    Returns (clone_url, branch) or raises ValueError.
    """
    # Full URL: https://github.com/owner/repo/tree/branch-name
    m = re.match(
        r"https?://github\.com/([^/]+)/([^/]+?)(?:\.git)?/tree/(.+?)/*$",
        url)
    if m:
        owner, repo, branch = m.group(1), m.group(2), m.group(3)
        return f"https://github.com/{owner}/{repo}.git", branch

    # URL without /tree: https://github.com/owner/repo
    m = re.match(
        r"https?://github\.com/([^/]+)/([^/]+?)(?:\.git)?/?$", url)
    if m:
        owner, repo = m.group(1), m.group(2)
        return f"https://github.com/{owner}/{repo}.git", "HEAD"

    # Shorthand: owner/repo#branch or owner/repo branch
    m = re.match(r"^([^/\s]+)/([^#\s]+)[#\s]+(.+)$", url.strip())
    if m:
        owner, repo, branch = m.group(1), m.group(2), m.group(3)
        return f"https://github.com/{owner}/{repo}.git", branch

    raise ValueError(f"Could not parse GitHub URL: {url}")


def _fetch_remote_branch(repo: Path, clone_url: str, branch: str) -> str:
    """Fetch a branch from a remote URL, return the local ref name."""
    ref_name = f"refs/pala-flash/{branch.replace('/', '-')}"
    _append_log(f"Fetching {branch} from {clone_url}...\n")
    if branch == "HEAD":
        result = subprocess.run(
            ["git", "fetch", clone_url, f"HEAD:{ref_name}"],
            capture_output=True, text=True, cwd=repo,
        )
    else:
        result = subprocess.run(
            ["git", "fetch", clone_url, f"{branch}:{ref_name}"],
            capture_output=True, text=True, cwd=repo,
        )
    if result.returncode != 0:
        raise RuntimeError(f"Fetch failed: {result.stderr}")
    return ref_name


def _append_log(msg: str) -> None:
    with _build_lock:
        _build_state["log"] += msg


def _build_in_place(repo: Path, envs: list) -> None:
    """Build the current checkout directly (fast — uses PIO cache)."""
    build_env = {**os.environ, "PYTHONIOENCODING": "utf-8"}
    for env in envs:
        _append_log(f"Building {env}...\n")
        result = subprocess.run(
            [sys.executable, "-m", "platformio", "run", "-e", env],
            capture_output=True, text=True, cwd=repo, env=build_env,
        )
        if result.returncode != 0:
            raise RuntimeError(result.stderr[-2000:])
        lines = result.stdout.strip().splitlines()
        _append_log(lines[-1] + "\n" if lines else "")


def _build_in_worktree(repo: Path, branch: str, envs: list) -> None:
    """Build a different branch in an isolated git worktree."""
    worktree_dir = Path(tempfile.mkdtemp(prefix="pala-flash-"))
    worktree_dir.rmdir()  # git worktree add wants a non-existent path
    try:
        _append_log(f"Creating worktree for {branch}...\n")
        result = subprocess.run(
            ["git", "worktree", "add", "--detach", str(worktree_dir), branch],
            capture_output=True, text=True, cwd=repo,
        )
        if result.returncode != 0:
            raise RuntimeError(result.stderr)

        build_env = {**os.environ, "PYTHONIOENCODING": "utf-8"}
        for env in envs:
            _append_log(f"Building {env}...\n")
            result = subprocess.run(
                [sys.executable, "-m", "platformio", "run", "-e", env],
                capture_output=True, text=True,
                cwd=worktree_dir, env=build_env,
            )
            if result.returncode != 0:
                raise RuntimeError(result.stderr[-2000:])
            lines = result.stdout.strip().splitlines()
            _append_log(lines[-1] + "\n" if lines else "")

        _append_log("Copying firmware...\n")
        for env in envs:
            for name in ("firmware.bin", "bootloader.bin", "partitions.bin"):
                src = worktree_dir / ".pio" / "build" / env / name
                if src.is_file():
                    dst = repo / ".pio" / "build" / env / name
                    dst.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(src, dst)
    finally:
        subprocess.run(
            ["git", "worktree", "remove", "--force", str(worktree_dir)],
            capture_output=True, cwd=repo,
        )
        if worktree_dir.exists():
            shutil.rmtree(worktree_dir, ignore_errors=True)


def _build_worker(repo: Path, branch: str, envs: list,
                  url: str = None) -> None:
    """Background thread: build firmware then reassemble the site."""
    global _build_state
    try:
        # If a remote URL was given, fetch it first to get a local ref.
        if url:
            clone_url, remote_branch = _parse_github_url(url)
            ref = _fetch_remote_branch(repo, clone_url, remote_branch)
            branch = ref  # use the fetched ref for the worktree
            version_label = f"{remote_branch} ({clone_url.split('/')[-2]})"
        else:
            version_label = None

        current = subprocess.run(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"],
            capture_output=True, text=True, cwd=repo,
        ).stdout.strip()

        if not url and branch == current:
            _build_in_place(repo, envs)
        else:
            _build_in_worktree(repo, branch, envs)

        _append_log("Assembling site...\n")
        version = version_label or get_version(repo, branch)
        result = subprocess.run(
            [sys.executable, str(repo / "scripts" / "assemble_site.py"),
             "--lenient", "--version", version],
            capture_output=True, text=True, cwd=repo,
        )
        if result.returncode != 0:
            raise RuntimeError(result.stderr)

        with _build_lock:
            _build_state["status"] = "done"
            _build_state["log"] += "Done!\n"

    except Exception as e:
        with _build_lock:
            _build_state["status"] = "error"
            _build_state["error"] = str(e)


class LocalFlashHandler(SimpleHTTPRequestHandler):
    """Static file server with a few JSON API endpoints for the build UI."""

    repo: Path = None  # set before server starts

    def do_GET(self):
        if self.path == "/api/branches":
            self._json_response(_list_branches(self.repo))
        elif self.path == "/api/status":
            with _build_lock:
                self._json_response(dict(_build_state))
        else:
            super().do_GET()

    def do_POST(self):
        if self.path == "/api/build":
            length = int(self.headers.get("Content-Length", 0))
            body = json.loads(self.rfile.read(length)) if length else {}
            branch = body.get("branch", "")
            url = body.get("url", "")
            envs = body.get("envs", ALL_ENVS)
            if isinstance(envs, str):
                envs = [envs]
            if not branch and not url:
                self._json_response(
                    {"error": "No branch or URL specified"}, 400)
                return

            with _build_lock:
                if _build_state["status"] == "building":
                    self._json_response(
                        {"error": "Build already in progress"}, 409)
                    return
                _build_state.update(
                    status="building", branch=url or branch,
                    envs=", ".join(envs), log="", error="")

            thread = threading.Thread(
                target=_build_worker,
                args=(self.repo, branch, envs),
                kwargs={"url": url or None},
                daemon=True,
            )
            thread.start()
            self._json_response({"status": "building"})
        else:
            self.send_error(404)

    def _json_response(self, data, code=200):
        body = json.dumps(data).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt, *args):
        if args and "/api/status" in str(args[0]):
            return
        super().log_message(fmt, *args)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build and serve the ESP Web Tools installer locally.")
    parser.add_argument(
        "--env", action="append", default=None, choices=ALL_ENVS,
        help="PIO environment(s) to build (repeatable). Default: all four.")
    parser.add_argument(
        "--build", action="store_true",
        help="Build firmware before serving (default: just serve, "
             "build from the browser UI).")
    parser.add_argument(
        "--port", type=int, default=8080,
        help="HTTP server port (default: 8080).")
    parser.add_argument(
        "--no-open", action="store_true",
        help="Don't auto-open the browser.")
    args = parser.parse_args()

    envs = args.env or ALL_ENVS
    repo = Path(__file__).resolve().parents[1]

    # --- initial build (opt-in) ---
    if args.build:
        build_env = {**os.environ, "PYTHONIOENCODING": "utf-8"}
        for env in envs:
            print(f"\n{'=' * 60}")
            print(f"  Building {env}")
            print(f"{'=' * 60}\n")
            result = subprocess.run(
                [sys.executable, "-m", "platformio", "run", "-e", env],
                cwd=repo, env=build_env,
            )
            if result.returncode != 0:
                print(f"\nBuild failed for {env}.", file=sys.stderr)
                return 1

    version = get_version(repo)
    print(f"\nAssembling installer site (version: {version})...")
    result = subprocess.run(
        [sys.executable, str(repo / "scripts" / "assemble_site.py"),
         "--lenient", "--version", version],
        cwd=repo,
    )
    if result.returncode != 0:
        print("Site assembly failed.", file=sys.stderr)
        return 1

    site_dir = repo / "site"
    if not site_dir.is_dir():
        print(f"Site directory not found: {site_dir}", file=sys.stderr)
        return 1

    # --- serve ---
    url = f"http://localhost:{args.port}"
    print(f"\nInstaller ready at {url}")
    print("Pick a branch in the UI to build & flash a different branch.")
    print("Press Ctrl+C to stop.\n")

    if not args.no_open:
        webbrowser.open(url)

    LocalFlashHandler.repo = repo
    handler = partial(LocalFlashHandler, directory=str(site_dir))
    server = HTTPServer(("localhost", args.port), handler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
