# Reader languages

Canonical translations live in `catalogs/{de,en,fr,es,pl,it,cs,ru,uk,nl,pt}.json`.
Every catalog must contain the complete English key set. Edit these JSON files;
the normal build runs `scripts/generate_locales.py` before compiling. Generated
runtime headers/data and manifests must not be edited by hand. `en.h` and
`es_la.h` are historical migration references used by an older preview tool;
they are not runtime translations.

Every image contains all eleven catalogs. EN/ES build targets choose only the
initial default. App `settings.get` advertises `language` and
`availableLanguages`; app `settings.set` and the browser language form persist
the same `cfg_lang`. Changing language preserves typography and canonical book
positions. Latin/Cyrillic book fonts are independent of the menu language.

`Language::select` is synchronous on the main thread. The generator partitions
catalogs into bounded pools and emits their key-location map. Selection validates
every pool before persistence/commit, retains the old dictionary until success,
and preloads pools used by the native interface. Browser pools are validated in
reusable scratch space and cached on first read. Their pointers remain stable
until the next successful language selection. Keep an owned String or a key,
rather than a cached translated pointer, across a language change.

The NVS wrapper allocates a bounded `CatalogInflater` workspace before output
pools to reduce allocation failures after BLE/Wi-Fi initialization. Lazy browser
loads create a scoped decoder when necessary. Catalog decoding does not use the
larger book inflater; book decoding keeps its own existing block lifetime.
Allocation, decoding and persistence failures preserve the old language/cache.

Generator gates check NFC, empty/NUL entries, printf/named placeholders,
confirmation safety and matching HTML markup. Browser messages are escaped data
attributes, separate from JavaScript. The generated `locale-manifest.json` is the
authority for current key/pool counts. Native memory/failure/lifetime, actual-font,
browser and large-book tests are in the Pala App workspace scripts.

Firmware stays private/local. Preserve the dual OTA slots and filesystem layout.
