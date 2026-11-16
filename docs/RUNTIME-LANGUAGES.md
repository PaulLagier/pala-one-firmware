# Runtime languages

The canonical catalogs in `src/lang/catalogs` cover German, English, French,
Spanish, Polish, Italian, Czech, Russian, Ukrainian, Dutch and Portuguese.
Each contains the same 521 keys for the full public interface, apps/settings,
web pages, recovery and Bluetooth. No missing translation is replaced with
English. French incorporates the original contribution from PR #164 and
retains that commit history. Runtime/recovery/Bluetooth additions are recorded
in the same catalogs.

`scripts/generate_locales.py --catalogs Pala_One_2_1/src/lang/catalogs --out
Pala_One_2_1/src/lang --interface-source Pala_One_2_1/src --require-all` generates
the tables and manifest. It checks completeness, Unicode NFC, NUL/empty strings,
native printf arguments, numbered JavaScript placeholders and HTML markup.
Legacy inline confirmation strings must also obey the quoting restrictions.
Text placeholders such as the localized `<saved>` marker are literal values,
not HTML tags. Generation is deterministic and rejects invalid catalogs before
replacing output files.

EN, ES and FR build flags select the initial dictionary. The default is defined
once for all translation units in `default_language.h`. Firmware uses runtime
getters; existing host adapter tests can retain literal headers, while dedicated
runtime tests compile the real core for all three defaults.

The manifest separates native-interface pools from browser-only pools. Native
pools are prepared before switching; browser pools are validated in reusable
scratch memory, then cached when needed. Each allocation is at most 2 KiB.
All selection/lookup operations run on the main thread. A successful selection
invalidates previously returned pointers; keep an owned String or a key across
language changes. Within a selection, cached browser pointers stay valid.
Allocation, decoding or persistence failure keeps the previous language and
its pointers. No filesystem/flash partition changes are required.

Catalog decompression uses a heap-owned workspace of at most 2 KiB, derived
from Mark Adler's puff (zlib 1.3.1 contribution; original license and modification
notice retained). The wrapper checks zlib framing, exact input/output sizes and
Adler-32. Book decompression remains on the existing SDK miniz path.

The language core is tested against every canonical string for all eleven
languages, allocation/decode/persistence failures, browser-pointer lifetimes
and malformed compressed data under ASan/UBSan. Native/JS placeholders and
catalog-generation rejection gates have independent Python tests.

Boot now loads the persisted selection before the first recovery/menu draw.
The companion adapter exposes all runtime catalogs through its existing RPC.
Web page fragments resolve translated strings per request; reader/screensaver
scripts quote translated JavaScript strings and escape HTML script delimiters.
Settings tables resolve labels on use instead of retaining catalog pointers.

USB RPC `0x81` now accepts and persists a supported two-byte language code;
see the vendored Improv README for framing and failure behavior. Parser tests
cover malformed/unsupported codes and ordinary query recovery.

The reader web settings have a separate language form. Invalid or unsupported
codes return an error before other settings are changed. Language-only changes
refresh the active interface without repaginating the book. The static installer
adds browser-language preselection and an explicit USB language action after
closing the normal installation dialog. Its JavaScript assets are included by
`assemble_site.py`; standard board/default manifests remain compatible.

Native interface coverage is verified with the pinned upstream U8g2 decoder for
all catalog codepoints in every regular/bold face and size. Original source and
font licenses live under `ui/font_licenses`. The body uses compact ordinary-line
bounds and grows page geometry for accents actually encountered. Composed and
decomposed supported Latin/Cyrillic pairs share measurement and rasterization;
Bionic prefixes and long-word splits retain combining clusters. Page offset
scans, drawing and extraction use the same converged metrics. The page-cache
magic changes to reject old font/layout offsets; source bytes and canonical
positions are unchanged. Existing half-paragraph, family and Bionic settings
and their public persistence keys are retained.

Host checks also exercise allocation/decode/persistence rollback, corrupted
catalogs, every localized reader/editor script, script-safe string round trips,
malformed/truncated/wrong-language USB replies, serial cleanup, compact metrics,
Unicode rasterization and matching draw/scan offsets under ASan/UBSan. They do
not claim physical V1.1 testing or phone pairing. Firmware-size/build evidence
for the exact final commit belongs with the PR and release artifacts.
