#!/usr/bin/env python3
"""Dev server for the web UI.

Serves webui/ at http://localhost:8000 and handles /api/* in one of two modes:

  - Proxy mode  (--device <ip>) : forwards /api/* to a running device.
  - Mock mode   (default)       : reads canned JSON from scripts/mock_data/.

Used during development so you can iterate on webui/index.html and the i18n
files without flashing firmware. Edit, save, refresh the browser. Once the
UI is good, run `python scripts/build_webui.py` (or any `pio run`) and the
gzipped bundle goes onto the device.

Examples:

  # Iterate against a real device on your network
  python scripts/dev_server.py --device 192.168.1.42

  # Iterate offline against mock JSON in scripts/mock_data/
  python scripts/dev_server.py

Mock layout: a request for /api/info maps to scripts/mock_data/api/info.json.
Add more mock files as endpoints are introduced — anything not found returns
404 so missing fixtures are loud.

Stdlib-only by design — no pip install needed for web-UI dev.
"""
from __future__ import annotations

import argparse
import sys
import urllib.error
import urllib.request
from http.server import HTTPServer, SimpleHTTPRequestHandler
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
WEBUI_DIR = REPO_ROOT / "webui"
MOCK_DIR  = REPO_ROOT / "scripts" / "mock_data"

# Set in main() before serving begins.
_DEVICE: str | None = None
_PROXY_TIMEOUT_S = 30.0  # large enough for book uploads on slow links

# Path prefixes that should NOT be served from webui/ -- they're firmware
# endpoints handled either by proxy (real device) or mock (canned files).
# /api/*           SPA JSON endpoints
# /upload          book upload (multipart, .txt)
# /upload-app      app binary upload (multipart, .bin)
# /screensavers/*  thumb / download / upload (binary endpoints used by SPA)
# /readbook-text   raw book text streaming  (Read & find screen)
# /jumpoffset      byte-offset persist      (Read & find screen)
_DYNAMIC_PREFIXES = (
    "/api/", "/upload", "/screensavers/", "/readbook-text", "/jumpoffset",
)


def _is_dynamic(path: str) -> bool:
    p = path.split("?", 1)[0]
    return any(p == pre.rstrip("/") or p.startswith(pre) for pre in _DYNAMIC_PREFIXES)


def _proxy(handler: "DevHandler", method: str) -> None:
    """Forward the current request to the device and stream the response."""
    body: bytes | None = None
    # Read the request body for any method that might carry one. Multipart
    # uploads can be MBs; SPA file/app uploads land here.
    if method in ("POST", "PUT", "PATCH"):
        length = int(handler.headers.get("Content-Length") or 0)
        body = handler.rfile.read(length) if length else None

    url = f"http://{_DEVICE}{handler.path}"
    req = urllib.request.Request(url, data=body, method=method)
    # Forward Content-Type so multipart / urlencoded posts make it through
    # unchanged. We deliberately don't forward Host/Origin/etc — the device's
    # WebServer doesn't care, and stripping them avoids confusing it.
    ct = handler.headers.get("Content-Type")
    if ct:
        req.add_header("Content-Type", ct)

    try:
        with urllib.request.urlopen(req, timeout=_PROXY_TIMEOUT_S) as resp:
            data = resp.read()
            handler.send_response(resp.status)
            for k, v in resp.headers.items():
                # Hop-by-hop headers don't survive proxying. Re-set Content-
                # Length from the actual buffered body length to be safe.
                if k.lower() in ("transfer-encoding", "connection",
                                 "content-length", "keep-alive"):
                    continue
                handler.send_header(k, v)
            handler.send_header("Content-Length", str(len(data)))
            handler.end_headers()
            handler.wfile.write(data)
    except urllib.error.HTTPError as e:
        # Device returned 4xx/5xx — pass it through so the SPA sees the real
        # error rather than a generic 502.
        body_bytes = e.read() or b""
        handler.send_response(e.code)
        handler.send_header("Content-Type",
                            e.headers.get("Content-Type", "text/plain"))
        handler.send_header("Content-Length", str(len(body_bytes)))
        handler.end_headers()
        handler.wfile.write(body_bytes)
    except (urllib.error.URLError, TimeoutError, ConnectionError) as e:
        handler.send_error(502, f"device unreachable at {_DEVICE}: {e}")


def _mock(handler: "DevHandler", method: str) -> None:
    """Serve canned JSON for the request path. Method is ignored on purpose —
    mocks are tiny fixtures, not behavior simulators. If you need POST-side
    state in dev, run against a real device."""
    # /api/info -> scripts/mock_data/api/info.json
    relpath = handler.path.split("?", 1)[0].lstrip("/")
    candidate = MOCK_DIR / f"{relpath}.json"
    if not candidate.is_file():
        handler.send_error(
            404,
            f"no mock for {method} {handler.path} (looked for {candidate.relative_to(REPO_ROOT)})"
        )
        return
    body = candidate.read_bytes()
    handler.send_response(200)
    handler.send_header("Content-Type", "application/json; charset=utf-8")
    handler.send_header("Cache-Control", "no-store")
    handler.send_header("Content-Length", str(len(body)))
    handler.end_headers()
    handler.wfile.write(body)


class DevHandler(SimpleHTTPRequestHandler):
    # SimpleHTTPRequestHandler resolves paths against `directory`. Pin it to
    # webui/ so we don't accidentally serve random repo files.
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(WEBUI_DIR), **kwargs)

    def _dispatch_dynamic(self, method: str) -> None:
        if _DEVICE:
            _proxy(self, method)
        else:
            _mock(self, method)

    def do_GET(self):
        if _is_dynamic(self.path):
            return self._dispatch_dynamic("GET")
        super().do_GET()

    def do_POST(self):
        if _is_dynamic(self.path):
            return self._dispatch_dynamic("POST")
        self.send_error(405, "static dev server only handles GET for non-dynamic paths")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--device",
        help="Hostname or IP of a running Pala device to proxy /api/* to. "
             "Omit to serve mocks from scripts/mock_data/ instead.",
    )
    parser.add_argument("--port", type=int, default=8000)
    args = parser.parse_args()

    if not WEBUI_DIR.is_dir():
        sys.exit(f"dev_server.py: webui/ not found at {WEBUI_DIR}")

    global _DEVICE
    _DEVICE = args.device

    mode = f"proxy -> {_DEVICE}" if _DEVICE else f"mock <- {MOCK_DIR.relative_to(REPO_ROOT)}"
    print(f"dev_server: serving {WEBUI_DIR.relative_to(REPO_ROOT)} "
          f"on http://localhost:{args.port}  (/api/* {mode})")
    print("Ctrl-C to stop.")
    try:
        HTTPServer(("", args.port), DevHandler).serve_forever()
    except KeyboardInterrupt:
        print()  # newline after ^C
        return 0
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
