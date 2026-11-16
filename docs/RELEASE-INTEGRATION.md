# Shared reader firmware integration

The shared firmware combines public development commit
`71e3f84050657a1c81b8e37198f23a0e09ce2e46` with the completed local reader behavior.
Main’s existing forgiving browser-search and UTF-8 offset correction is retained
in the runtime-localized script. Community commits and attribution remain in the
normal merge history.

## Compatibility

- All eleven runtime languages, installed apps, custom menu order and public
  reader gesture mappings remain available. Reading-focused menus are reversible.
- Existing ClickTimings persistence keys and the verified plain/PBT stream remain
  authoritative. App callback queues and upload/authentication hardening are kept.
- The partition layout and OTA slot sizes are unchanged. Canonical text offsets
  remain the source of truth for progress and bookmarks.
- Window cache magic is `0x50434F4C`, with strict bounds and CRC32. Matching private
  `0x50434F4B` windows require their distinct layout stamp and CRC. Incompatible
  prefix caches rebuild without changing saved progress.
- Ordinary bookmarks still use V2. V3 is needed only for page labels above 65535;
  older firmware cannot read those extended records. Keep a backup for rollback.

## Reader behavior

Input capture starts before serial, filesystem and display setup. V1.2 avoids a
redundant warm-start clear; V1.1 retains its own panel initialization. Only the held
wake press is suppressed. Intentional later gestures remain queued.

A bounded 4 KiB text buffer stages converged page layout and prepares the next
page while idle. An oversized staged page falls back to complete direct rendering.
The stable public BookReadStream is reused. The 10000-entry page table is a rolling
RAM window rather than a book-length limit. Foreground indexing can be cancelled;
background app/web lookups do not consume input or draw progress screens.

## Release checks

The native suite covers actual wake setup branches for both boards, gesture
classification during display delays, real fonts and page pixels, plain/PBT
parity, more than 100000 pages, reverse window crossings, canonical progress,
cache corruption and short writes, extended bookmarks, reset failure ordering,
serial framing and installer acknowledgments. Release packaging verifies all six
board/default-language variants, source provenance, checksums and partition layout.

The release process builds both board revisions. Physical device evidence is for
V1.2 only. Stable publication requires a version tag; prerelease tags cannot replace
the stable installer. The protected online flasher must use the same release files
and SHA-256 identities as the GitHub release, with a separate board selection.
