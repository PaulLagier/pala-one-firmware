# webui/ — embedded web UI source

This folder holds the human-edited source for the device's web admin UI.
A Python build step (`scripts/build_webui.py`) inlines, gzips, and emits it
as a `PROGMEM` byte array at `Pala_One_2_1/src/web/generated/webui.gz.h`,
which is committed and linked into the firmware. The device serves it from
the root URL (`/`).

## Dev loop (no flashing)

Edit a file in `webui/`, then refresh the browser tab. Two ways to back the
`/api/*` calls:

```bash
# Against a real device on your network
python scripts/dev_server.py --device 192.168.1.42

# Offline, against mock JSON in scripts/mock_data/
python scripts/dev_server.py
```

Open <http://localhost:8000>. Static files come from `webui/`; anything under
`/api/*` is proxied to the device or served from `scripts/mock_data/<path>.json`.

## Flashing your changes

```bash
# PlatformIO regenerates webui.gz.h on every `pio run` via a pre-script.
pio run -e wireless-paper-v1_2-en -t upload

# Arduino IDE has no per-sketch pre-build hook, so regenerate manually first:
python scripts/build_webui.py
# then Verify/Upload in Arduino IDE as usual.
```

CI re-runs `build_webui.py` and fails the workflow if the committed header
would change, so stale bundles can't reach `main`.

## Layout

- `webui/index.html` — single page; the build inlines its `<link rel='stylesheet'>`
  and `<script src='...'>` references.
- `webui/i18n/{en,es}.js` — strings, keyed by language. Each file populates
  `window.__pala_i18n[lang]`. Add a third language by creating
  `webui/i18n/<code>.js` with the same key set and adding a matching
  `<script src='i18n/<code>.js'>` tag in `index.html`.
