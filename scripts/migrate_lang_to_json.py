#!/usr/bin/env python3
"""One-time migration: parse en.h and es_la.h into en.json and es_la.json.

Run once, verify the output, then delete this script.
"""
import json
import re
import sys
from pathlib import Path

DEFINE_RE = re.compile(r'^#define\s+(D_\S+)\s+"((?:[^"\\]|\\.)*)"')
SECTION_SEP_RE = re.compile(r"^//\s*-{10,}\s*$")
SECTION_TITLE_RE = re.compile(r"^//\s+(.+?)\s*$")
C_UCN_RE = re.compile(r"\\u([0-9a-fA-F]{4})")


def decode_c_ucn(val):
    """Convert C universal character names (\\u00b0) to actual Unicode."""
    return C_UCN_RE.sub(lambda m: chr(int(m.group(1), 16)), val)


def parse_h_file(path):
    """Extract section markers and D_ key-value pairs from a .h file."""
    lines = path.read_text(encoding="utf-8").splitlines()
    entries = []
    i = 0
    while i < len(lines):
        line = lines[i]

        # Detect section: // ----...  //  Title ...  // ----...
        if SECTION_SEP_RE.match(line) and i + 2 < len(lines):
            title_match = SECTION_TITLE_RE.match(lines[i + 1])
            if title_match and SECTION_SEP_RE.match(lines[i + 2]):
                raw_title = title_match.group(1).strip()
                # Split "Title (source file)" into name and description
                paren = raw_title.find("(")
                if paren > 0:
                    name = raw_title[:paren].strip().rstrip("—").rstrip().rstrip("-").rstrip()
                    desc = raw_title[paren + 1 :].rstrip(")")
                else:
                    name = raw_title.split("—")[0].strip() if "—" in raw_title else raw_title
                    desc = raw_title.split("—", 1)[1].strip() if "—" in raw_title else ""
                entries.append(("_section", name, desc))
                i += 3
                continue

        # Detect #define D_KEY "value"
        m = DEFINE_RE.match(line)
        if m:
            key = m.group(1)
            val = decode_c_ucn(m.group(2))
            entries.append(("_define", key, val))

        i += 1
    return entries


def build_json(entries, meta, include_sections=True):
    data = {"_meta": meta}
    for kind, a, b in entries:
        if kind == "_section" and include_sections:
            section_key = f"_section:{a}"
            data[section_key] = b
        elif kind == "_define":
            data[a] = b
    return data


def main():
    lang_dir = Path(__file__).resolve().parent.parent / "Pala_One_2_1" / "src" / "lang"

    en_path = lang_dir / "en.h"
    es_path = lang_dir / "es_la.h"
    if not en_path.exists() or not es_path.exists():
        print(f"ERROR: expected {en_path} and {es_path}", file=sys.stderr)
        return 1

    en_entries = parse_h_file(en_path)
    en_keys = [a for kind, a, _ in en_entries if kind == "_define"]
    en_json = build_json(
        en_entries,
        {
            "language": "en",
            "label": "English",
            "description": "Canonical key set. Add new keys here first, then mirror in other language files.",
            "rules": [
                "Preserve %u / %d / %s / %llu format placeholders across languages.",
                "D_WEB_CONFIRM_* keys must not contain single quotes or backslashes.",
                "No value may be empty or whitespace-only.",
            ],
        },
        include_sections=True,
    )

    es_entries = parse_h_file(es_path)
    es_keys = [a for kind, a, _ in es_entries if kind == "_define"]
    es_json = build_json(
        es_entries,
        {"language": "es_la", "label": "Spanish (Latin America)"},
        include_sections=False,
    )

    out_en = lang_dir / "en.json"
    out_en.write_text(json.dumps(en_json, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"Wrote {out_en.name} ({len(en_keys)} keys)")

    out_es = lang_dir / "es_la.json"
    out_es.write_text(json.dumps(es_json, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"Wrote {out_es.name} ({len(es_keys)} keys)")

    # Quick parity check
    en_set = set(en_keys)
    es_set = set(es_keys)
    missing = en_set - es_set
    extra = es_set - en_set
    if missing:
        print(f"WARNING: keys in en.h missing from es_la.h: {sorted(missing)}")
    if extra:
        print(f"WARNING: keys in es_la.h not in en.h: {sorted(extra)}")
    if not missing and not extra:
        print(f"Key sets match ({len(en_keys)} keys)")

    return 0


if __name__ == "__main__":
    sys.exit(main())
