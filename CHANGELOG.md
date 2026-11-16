# Changelog

## v3.1.1

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
