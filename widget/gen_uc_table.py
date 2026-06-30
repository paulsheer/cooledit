#!/usr/bin/env python3
"""
Generate a compact Unicode category lookup table for classify.c.

Fetches DerivedGeneralCategory.txt from Unicode 17.0.0 and produces a C array
of { start_code_point, category } structs suitable for binary search.

Usage:
    curl -O https://www.unicode.org/Public/UCD/latest/ucd/extracted/DerivedGeneralCategory.txt
    python3 gen_uc_table.py DerivedGeneralCategory.txt > uc_table.inc
"""

import re
import sys

CATEGORIES = [
    "Cn", "Lu", "Ll", "Lt", "Lm", "Lo",
    "Mn", "Mc", "Me",
    "Nd", "Nl", "No",
    "Pc", "Pd", "Ps", "Pe", "Pi", "Pf", "Po",
    "Sm", "Sc", "Sk", "So",
    "Zs", "Zl", "Zp",
    "Cc", "Cf", "Cs", "Co",
]
CAT_TO_ENUM = {c: i for i, c in enumerate(CATEGORIES)}


def parse_derived_general_category(path):
    """Parse DerivedGeneralCategory.txt and return sorted merged ranges."""
    ranges = []
    with open(path, "r") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            m = re.match(r'^([0-9A-F]+)(?:\.\.([0-9A-F]+))?\s*;\s*(\w+)', line)
            if m:
                start = int(m.group(1), 16)
                end = int(m.group(2), 16) if m.group(2) else start
                cat = m.group(3)
                if cat in CAT_TO_ENUM:
                    ranges.append((start, end, CAT_TO_ENUM[cat]))

    # Sort by start code point
    ranges.sort(key=lambda x: x[0])

    # Build a fully-covered, merged list from 0 to 0x10FFFF
    merged = []
    expected = 0

    for start, end, cat in ranges:
        if start > expected:
            merged.append([expected, start - 1, CAT_TO_ENUM["Cn"]])
        if merged and merged[-1][2] == cat and start <= merged[-1][1] + 1:
            merged[-1][1] = max(merged[-1][1], end)
        else:
            merged.append([start, end, cat])
        expected = max(expected, end + 1)

    if expected <= 0x10FFFF:
        merged.append([expected, 0x10FFFF, CAT_TO_ENUM["Cn"]])

    # Merge adjacent same-category entries
    i = 0
    while i < len(merged) - 1:
        if merged[i][2] == merged[i + 1][2] and merged[i][1] + 1 >= merged[i + 1][0]:
            merged[i][1] = max(merged[i][1], merged[i + 1][1])
            del merged[i + 1]
        else:
            i += 1

    return merged


def emit_table(merged, raw_count):
    """Print the C source for the lookup table."""
    print("/* Auto-generated from Unicode 17.0.0 DerivedGeneralCategory.txt */")
    print(f"/* {len(merged)} range entries (compressed from {raw_count} raw entries) */")
    print()
    print("/* Each entry: code points >= start have this category.")
    print("   End is implicit (next entry's start - 1).")
    print("   Binary search pattern follows is_unicode_doublewidth_char() in widechar.c */")
    print()
    print("struct uc_range {")
    print("    C_wchar_t start;")
    print("    unsigned char category;")
    print("};")
    print()
    print("static struct uc_range uc_table[] = {")

    for r in merged:
        cat_name = CATEGORIES[r[2]]
        print(f"    {{ 0x{r[0]:06X}, UC_{cat_name} }},")

    print("};")
    print()
    print(f"#define UC_TABLE_SIZE {len(merged)}")


def main():
    if len(sys.argv) < 2:
        sys.stderr.write(f"Usage: {sys.argv[0]} <DerivedGeneralCategory.txt>\n")
        sys.exit(1)

    merged = parse_derived_general_category(sys.argv[1])
    emit_table(merged, len(merged))


if __name__ == "__main__":
    main()
