#!/usr/bin/env python3
"""Generate standalone C header files from JSON translation sources.

Usage:
  PlatformIO pre-script:  extra_scripts = pre:scripts/gen_lang.py
  Standalone:             python scripts/gen_lang.py [--lang-dir PATH] [--check-only]
  CMake:                  add_custom_command(... COMMAND python gen_lang.py --lang-dir ...)

Each .json file in the lang directory produces a matching .h file.
en.json is the canonical baseline — all other language files are validated
against it for key parity, format-placeholder consistency, and authoring
constraints (no single quotes in CONFIRM keys, no empty values).

Generated .h files are standalone (each defines every D_* key), so lang.h
includes exactly one file based on the LANG_* build flag.
"""
import json
import re
import sys
from pathlib import Path

FMT_RE = re.compile(r"%(?:\d+\$)?(?:ll)?[dusfx]")
CONFIRM_KEY_RE = re.compile(r"^D_WEB_(?:SS_)?CONFIRM_")


# ---------------------------------------------------------------------------
# JSON loading
# ---------------------------------------------------------------------------

def load_lang(path):
    """Return (meta, sections, entries) from a language JSON file.

    meta:     dict from "_meta" key (or empty dict)
    sections: list of (insert_before_index, name, description)
    entries:  list of (key, value) for D_* keys, in file order
    """
    with open(path, encoding="utf-8") as f:
        data = json.load(f)

    meta = data.get("_meta", {})
    sections = []
    entries = []
    d_index = 0

    for key, val in data.items():
        if key == "_meta":
            continue
        if key.startswith("_section:"):
            name = key[len("_section:"):]
            sections.append((d_index, name, val if isinstance(val, str) else ""))
            continue
        entries.append((key, val))
        d_index += 1

    return meta, sections, entries


# ---------------------------------------------------------------------------
# Validation
# ---------------------------------------------------------------------------

def validate(en_entries, others):
    """Validate all language files against the English baseline.

    Returns (errors, warnings) — both lists of strings.
    Errors are fatal; warnings allow the build to continue.
    """
    errors = []
    warnings = []
    en_keys = [k for k, _ in en_entries]
    en_key_set = set(en_keys)
    en_values = {k: v for k, v in en_entries}

    def check_entry(lang_label, key, val):
        if not key.startswith("D_"):
            errors.append(f"{lang_label}: key '{key}' does not start with D_")
        if not isinstance(val, str) or not val.strip():
            errors.append(f"{lang_label}: key '{key}' has empty/non-string value")
            return
        if CONFIRM_KEY_RE.match(key) and ("'" in val or "\\" in val):
            errors.append(
                f"{lang_label}: CONFIRM key '{key}' contains single quote or backslash "
                "(breaks JS confirm() in onclick attributes)"
            )

    # English self-checks
    for key, val in en_entries:
        check_entry("en", key, val)

    # Per-language checks
    for lang_code, entries in others:
        lang_key_set = set(k for k, _ in entries)

        for k in sorted(lang_key_set - en_key_set):
            errors.append(f"{lang_code}: extra key '{k}' not in en.json")
        for k in sorted(en_key_set - lang_key_set):
            warnings.append(f"{lang_code}: missing key '{k}', will use English fallback")

        for key, val in entries:
            check_entry(lang_code, key, val)
            if key in en_values:
                en_fmts = FMT_RE.findall(en_values[key])
                lang_fmts = FMT_RE.findall(val)
                if en_fmts != lang_fmts:
                    errors.append(
                        f"{lang_code}: key '{key}' placeholders {lang_fmts} "
                        f"differ from en {en_fmts}"
                    )

    return errors, warnings


# ---------------------------------------------------------------------------
# Header generation
# ---------------------------------------------------------------------------

def escape_c_string(s):
    """Escape a value for use inside a C string literal (double-quoted)."""
    s = s.replace("\\", "\\\\")
    s = s.replace('"', '\\"')
    return s


def generate_header(lang_code, lang_label, entries, sections, en_entries, en_sections):
    """Produce a standalone .h file as a string.

    Uses en_entries order as canonical. Keys missing from this language's
    entries are filled from the English value.
    """
    guard = f"PALA_LANG_{lang_code.upper()}_H"
    lang_map = {k: v for k, v in entries}
    use_sections = sections if sections else en_sections

    # Build a mapping: D_ key index -> section to emit before it
    section_at = {pos: (name, desc) for pos, name, desc in use_sections}

    lines = [
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        "// " + "=" * 76,
        f"//  AUTO-GENERATED by scripts/gen_lang.py — DO NOT EDIT.",
        f"//  Source: src/lang/{lang_code}.json",
        f"//  Language: {lang_label} ({lang_code})",
        "// " + "=" * 76,
    ]

    for idx, (key, en_val) in enumerate(en_entries):
        if idx in section_at:
            name, desc = section_at[idx]
            lines.append("")
            lines.append("// " + "-" * 76)
            if desc:
                lines.append(f"//  {name} ({desc})")
            else:
                lines.append(f"//  {name}")
            lines.append("// " + "-" * 76)

        val = lang_map.get(key, en_val)
        escaped = escape_c_string(val)
        padding = max(1, 32 - len(key))
        lines.append(f"#define {key}{' ' * padding}\"{escaped}\"")

    lines.append("")
    lines.append(f"#endif  // {guard}")
    lines.append("")
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# File I/O
# ---------------------------------------------------------------------------

def write_if_changed(path, content):
    """Write content only if it differs from what's on disk. Return True if written."""
    if path.exists():
        try:
            existing = path.read_text(encoding="utf-8")
            if existing == content:
                return False
        except Exception:
            pass
    path.write_text(content, encoding="utf-8")
    return True


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main(lang_dir, check_only=False):
    """Generate language headers. Return 0 on success, 1 on error."""
    en_path = lang_dir / "en.json"
    if not en_path.exists():
        print(f"gen_lang.py: ERROR: canonical file not found: {en_path}", file=sys.stderr)
        return 1

    other_paths = sorted(p for p in lang_dir.glob("*.json") if p.name != "en.json")

    en_meta, en_sections, en_entries = load_lang(en_path)

    others_data = []
    for p in other_paths:
        meta, sects, entries = load_lang(p)
        others_data.append((p.stem, meta, sects, entries))

    # Validate
    others_for_validation = [(code, entries) for code, _, _, entries in others_data]
    errors, warnings = validate(en_entries, others_for_validation)

    for w in warnings:
        print(f"gen_lang.py: WARNING: {w}", file=sys.stderr)
    for e in errors:
        print(f"gen_lang.py: ERROR: {e}", file=sys.stderr)

    if errors:
        print(f"gen_lang.py: {len(errors)} error(s), aborting.", file=sys.stderr)
        return 1

    if check_only:
        print(
            f"gen_lang.py: validation passed ({len(en_entries)} keys, "
            f"{len(others_data)} language(s), {len(warnings)} warning(s))"
        )
        return 0

    # Generate English
    en_label = en_meta.get("label", "English")
    en_content = generate_header("en", en_label, en_entries, en_sections, en_entries, en_sections)
    changed = write_if_changed(lang_dir / "en.h", en_content)
    print(f"gen_lang.py: en.h {'updated' if changed else 'unchanged'}")

    # Generate other languages
    for lang_code, meta, sects, entries in others_data:
        label = meta.get("label", lang_code)
        content = generate_header(lang_code, label, entries, sects, en_entries, en_sections)
        changed = write_if_changed(lang_dir / f"{lang_code}.h", content)
        print(f"gen_lang.py: {lang_code}.h {'updated' if changed else 'unchanged'}")

    print(
        f"gen_lang.py: done ({len(en_entries)} keys, "
        f"{1 + len(others_data)} language(s), {len(warnings)} warning(s))"
    )
    return 0


# ---------------------------------------------------------------------------
# Entry points
# ---------------------------------------------------------------------------

# PlatformIO pre-build hook
try:
    Import("env")  # noqa: F821 — provided by PlatformIO at script-load time
    _pio_env = env  # noqa: F821
except NameError:
    _pio_env = None

if _pio_env is not None:
    _project_dir = Path(_pio_env["PROJECT_DIR"])
    _lang_dir = _project_dir / "Pala_One_2_1" / "src" / "lang"
    _rc = main(_lang_dir)
    if _rc != 0:
        sys.exit(_rc)

# Standalone / CMake
elif __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument(
        "--lang-dir",
        type=Path,
        default=Path(__file__).resolve().parent.parent / "Pala_One_2_1" / "src" / "lang",
        help="Directory containing *.json language files (default: auto-detected)",
    )
    parser.add_argument(
        "--check-only",
        action="store_true",
        help="Validate JSON files without writing headers",
    )
    args = parser.parse_args()
    sys.exit(main(args.lang_dir, check_only=args.check_only))
