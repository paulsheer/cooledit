#!/usr/bin/env python3
"""
Generate is_wide_table[] for widechar.c from Unicode EastAsianWidth.txt.

Produces the static C array of (codepoint, is_wide) boundary entries used
by is_unicode_doublewidth_char() to determine terminal display width.

The table uses a toggling-boundary encoding: each entry toggles the width
state, so entries alternate wide=1/0. All codepoints below the first entry
are implicitly narrow (wide=0).

Usage:
    python3 gen_wide_table.py > is_wide_table.c

Requires network access on first run to download EastAsianWidth.txt.
Subsequent runs use the cached file.
"""

import sys
import os

try:
    from urllib.request import urlopen
except ImportError:
    from urllib import urlopen

UNICODE_VERSION = "17.0.0"
EAW_URL = f"https://www.unicode.org/Public/{UNICODE_VERSION}/ucd/EastAsianWidth.txt"
CACHE_FILE = f"EastAsianWidth-{UNICODE_VERSION}.txt"


def fetch_eaw():
    """Download EastAsianWidth.txt from unicode.org, caching locally."""
    if os.path.exists(CACHE_FILE):
        with open(CACHE_FILE, "r", encoding="utf-8") as f:
            return f.read()

    sys.stderr.write(f"Downloading {EAW_URL}...\n")
    resp = urlopen(EAW_URL)
    data = resp.read().decode("utf-8")
    with open(CACHE_FILE, "w", encoding="utf-8") as f:
        f.write(data)
    return data


def parse_codepoint(s):
    return int(s, 16)


def parse_eaw_ranges(text):
    """Parse EastAsianWidth.txt into a sorted list of (start, end, property).

    The file uses '#' for comments and ';' to separate codepoint from property.
    Codepoints specified as single values or ranges (A..B).
    """
    ranges = []
    for raw_line in text.splitlines():
        line = raw_line.split("#")[0].strip()
        if not line:
            continue
        try:
            cp_spec, prop = line.split(";")
        except ValueError:
            continue
        cp_spec = cp_spec.strip()
        prop = prop.strip()
        if ".." in cp_spec:
            lo, hi = cp_spec.split("..")
            start, end = parse_codepoint(lo), parse_codepoint(hi)
        else:
            start = end = parse_codepoint(cp_spec)
        ranges.append((start, end, prop))

    ranges.sort(key=lambda r: r[0])
    return ranges


def build_boundaries(ranges):
    """Build the alternating boundary table.

    Only W (Wide) and F (Fullwidth) codepoints are double-width.
    Finds the start and end+1 of every wide block and emits
    toggling entries sorted by codepoint.
    """
    # Collect all wide ranges
    wide_blocks = []
    for start, end, prop in ranges:
        if prop in ("W", "F"):
            wide_blocks.append((start, end))

    # Merge overlapping or adjacent wide blocks
    wide_blocks.sort()
    merged = []
    for start, end in wide_blocks:
        if merged and start <= merged[-1][1] + 1:
            merged[-1] = (merged[-1][0], max(merged[-1][1], end))
        else:
            merged.append((start, end))

    # Build boundary list: each wide block produces two boundaries
    # (start -> 1, end+1 -> 0). At end+1 the width toggles back to 0.
    boundaries = []
    for start, end in merged:
        boundaries.append((start, 1))
        boundaries.append((end + 1, 0))

    boundaries.sort(key=lambda b: b[0])

    # Remove redundant consecutive same-width entries (shouldn't occur
    # with properly merged blocks, but handles edge cases)
    deduped = []
    for cp, wide in boundaries:
        if deduped and cp == deduped[-1][0]:
            deduped[-1] = (cp, wide)
        elif deduped and wide == deduped[-1][1]:
            pass
        else:
            deduped.append((cp, wide))

    return deduped


def print_table(boundaries):
    """Emit the C static array."""
    print("static struct widetable is_wide_table[] = {")
    for cp, wide in boundaries:
        print(f"    {{ 0x{cp:x}, {wide} }},")
    print("};")


def main():
    text = fetch_eaw()
    ranges = parse_eaw_ranges(text)
    boundaries = build_boundaries(ranges)
    print_table(boundaries)


if __name__ == "__main__":
    main()
