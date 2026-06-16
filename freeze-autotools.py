#!/usr/bin/env python3
"""
Neutralize autotools regeneration rules in all Makefile.in files.

Replaces recipes that invoke automake, autoconf, autoheader, or aclocal
with '@true' so that 'make' never tries to regenerate build files, even
if Makefile.am or configure.ac timestamps are newer than the .in files.

After running this script, the source tree can be distributed as a
release tarball that does not require autotools installed to build.
"""

import re
import sys
from pathlib import Path

TOP = Path(__file__).resolve().parent


def neutralize_targets(path, targets):
    """
    Read *path*, find each target header regex in *targets*, replace the
    recipe block (contiguous tab-indented lines) that follows with ``@true``.
    Returns the new file content (or None if no changes).
    """
    text = path.read_text()
    changed = False

    for header_re in targets:
        # Match: [target header line] \n [one or more tab-indented recipe lines]
        pattern = re.compile(
            r'(^' + header_re + r'.*\n)'   # target: dependency line
            r'((?:\t.*\n)+)',               # recipe block (tab-indented lines)
            re.MULTILINE,
        )
        m = pattern.search(text)
        if m:
            text = text[:m.start()] + m.group(1) + '\t@true\n' + text[m.end():]
            changed = True

    return text if changed else None


def main():
    makefiles = sorted(TOP.glob('**/Makefile.in'))
    if not makefiles:
        print('No Makefile.in files found.', file=sys.stderr)
        sys.exit(1)

    # Targets found in ALL Makefile.in files:
    common_targets = [
        r'\$\(srcdir\)/Makefile\.in:',   # runs $(AUTOMAKE)
    ]

    # Targets found ONLY in the top-level Makefile.in:
    toplevel_targets = common_targets + [
        r'\$\(top_srcdir\)/configure:',   # runs $(AUTOCONF)
        r'\$\(ACLOCAL_M4\):',             # runs $(ACLOCAL)
        r'\$\(srcdir\)/config\.h\.in:',   # runs $(AUTOHEADER)
    ]

    top = TOP / 'Makefile.in'
    modified = 0

    for mf in makefiles:
        targets = toplevel_targets if mf == top else common_targets
        new_text = neutralize_targets(mf, targets)
        if new_text is not None:
            mf.write_text(new_text)
            print(f'  Neutralized: {mf.relative_to(TOP)}')
            modified += 1
        else:
            print(f'  No change:   {mf.relative_to(TOP)}')

    print(f'\nModified {modified} of {len(makefiles)} Makefile.in files.')


if __name__ == '__main__':
    main()
