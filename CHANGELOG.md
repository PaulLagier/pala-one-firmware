# Changelog

## v3.1.3

- Reject browser book uploads that would replace an existing destination,
  including sanitized/truncated filename collisions and late destination creation.
- Honor configured custom lock gestures after wake and during storage recovery;
  retain the legacy/unassigned automatic-lock hold fallback.
- Report failed Wi-Fi association without declaring the radio unavailable, so
  setup can retry and saved credentials remain unchanged on failure.
- Serve the existing image-editor script as a deterministic gzip asset, keeping
  all eleven localized editors and the existing OTA partition layout.

## v3.1.2

- Compact browser-script local names without changing reader/search/editor behavior
  so all six variants fit the existing OTA partition.
- Check all six board/default-language builds and final padded image sizes before
  merging. v3.1.1 is retained as a source preview after its final variant exceeded
  the application slot; no firmware packages were published for it.

## v3.1.1 (source preview)

- Allow USB language and menu setup while a new device is waiting for storage
  initialization. The recovery allow-list permits only device information and
  these NVS settings; Wi-Fi status, scan and connection commands remain blocked.
- Retain the complete shared reader update below. The v3.1.0 source tag is kept
  as a preliminary snapshot; no firmware packages are distributed for it.

## v3.1.0 (source preview)

- Add a reversible reading-focused menu view with clear return actions, fixed
  menu typography, a compact Device menu and read-only bookmark excerpts.
- Capture wake input early, preserve intentional queued gestures and avoid the
  redundant V1.2 warm-start clear. Retain the V1.1 panel initialization path.
- Stage page text once and prepare the next page while idle using the verified
  plain/PBT stream. Keep a complete render fallback for oversized staged pages.
- Navigate large books with a bounded page window, strict CRC-checked caches
  and cancellable foreground indexing. Preserve canonical reading offsets.
- Support large bookmark page labels while continuing to write ordinary
  bookmarks in the existing V2 format. Extended V3 bookmarks require this or
  later firmware; keep a backup before rolling back to an older version.
- Retain all eleven runtime languages, custom menu order, installed apps,
  public gesture mappings and the hardened app/upload protocols.
- Compress browser stylesheets without changing their decoded content. Keep
  stored Wi-Fi passwords out of form values and report factory-reset failures.
- Preserve main’s forgiving browser search and UTF-8 jump/status offsets in all
  eleven localized reader scripts, including Unicode expansion boundaries.
- Include source provenance, board-specific packages, English installation
  instructions and SHA-256 checksums. Keep prerelease tags off the stable site.
