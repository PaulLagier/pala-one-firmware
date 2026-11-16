# Experimental companion firmware snapshot — 16 November 2026

This branch preserves local development based on main commit
2c82e59e0f6e87afd204606b930f15cf7b5b7b8a. It is not a stable release
or a proposal to replace main/dev as a single merge.

It includes companion BLE integration, packed books, eleven runtime languages,
compact line spacing, simplified reader menus, explicit storage recovery and
selected improvements adapted from dev at 50543ad8398a32dfff7b6a20bf0300b13e7b248c.
Upstream-derived work must retain attribution when extracted into contributions.

The source also retains optional local diagnostics, including wake tracing.
Those builds differ from the production image currently installed on the reader.
V1.2 has prior physical device validation; V1.1 still needs physical acceptance.
Partition layout is unchanged. Existing public OTA channels do not carry these
companion extensions. No binaries, device backups, access codes or private
release records are included here.

## Integration plan

1. Extract independent correctness fixes against current dev, with regression
   tests. First: bounds checking when decoding truncated bookmark records.
2. Review buffered reading, page caches and rendering changes against equivalent
   work already in dev; contribute only remaining improvements.
3. Review storage recovery and USB provisioning separately, including compatibility.
4. Review companion transport, packed books and runtime languages as separate
   features, with protocol documentation and board-specific build validation.
5. Discuss simplified menus independently as a product choice. Preserve community
   apps/settings workflows or expose the reading-focused layout as an option.

Do not merge this whole snapshot or publish a stable release without those reviews.
