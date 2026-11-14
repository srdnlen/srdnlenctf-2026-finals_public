#!/usr/bin/env python3
"""List the text a shipped build carries.

The builds are stripped, so strings are the main thing a reverse engineer
gets for free.  This prints printable runs that read like words, which is
what a team would grep for after opening the binary.
"""
import re
import sys

MIN_RUN = 5


def runs(data):
    for match in re.finditer(rb"[ -~]{%d,}" % MIN_RUN, data):
        yield match.start(), match.group().decode("ascii")


def wordy(text):
    """Text a human would read, as opposed to code bytes that happen to fall
    in the printable range."""
    if not re.search(r"[A-Za-z]{3,}", text):
        return False
    if not re.search(r"[aeiouAEIOU]", text):
        return False
    readable = sum(c.isalpha() or c in " -'.,!?():/" for c in text)
    return readable >= 0.8 * len(text)


def main():
    if len(sys.argv) < 2:
        sys.exit("usage: strings.py <build.elf> [...]")

    for path in sys.argv[1:]:
        with open(path, "rb") as fh:
            data = fh.read()

        print("== %s (%d bytes)" % (path, len(data)))
        for offset, text in runs(data):
            if wordy(text):
                print("  %06x  %s" % (offset, text))


if __name__ == "__main__":
    main()
