#!/usr/bin/env python3
"""Compare the player build with the station build.

The two builds share all of their game code.  Two differences are intended:

* the flag payload: the station build carries it, the player build ships it
  zeroed;
* the controls variant: the player build runs in an emulator, so its title
  screen lists keyboard controls and it accepts the D-pad as a tilt fallback,
  while the station build is tilt only.

This script checks that nothing else differs by any meaningful amount, and
that the player build does not contain the plaintext flag.  (The station build
does carry it -- the payload is plain text, and players only ever run the
shipped .dol, never the binary.)

  tools/verify_builds.py dist/pwnchuck-player.elf dist/pwnchuck-station.elf
  tools/verify_builds.py ... --flag 'srdnlen{...}'
"""
import argparse
import sys

FLAG_PREFIX = b"srdnlen{"
PAYLOAD_BUDGET = 256     # flag payload (128) + length (1), with slack
CONTROLS_BUDGET = 4096   # the emulator-controls variant (title text + fallback)

# Development-only markers and giveaway words.  A shipped build must carry
# none of them: this is the regression guard for "no clues for the reverse
# engineers".  tools/strings.py is the wider audit.
FORBIDDEN = [
    b"seen btns",     # the debug overlay
    b"1=debug",
    b"2=respawn",
    b"DEV:",          # the development cheat messages
    b"nunchuk",       # the input source labels (the title screen uses NUNCHUK)
    b"wiimote",
    b"scratch slot",
    b"underflow",     # words that would hand the bug over
    b"overflow",
    b"wraparound",
    b"bounds check",
]


def read(path):
    try:
        with open(path, "rb") as fh:
            return fh.read()
    except OSError as exc:
        sys.exit(f"cannot read {path}: {exc}")


def diff_ranges(player, station):
    """Contiguous ranges where the two byte strings differ."""
    ranges = []
    start = None
    for i, (a, b) in enumerate(zip(player, station)):
        if a != b:
            if start is None:
                start = i
        elif start is not None:
            ranges.append((start, i))
            start = None
    if start is not None:
        ranges.append((start, len(player)))
    return ranges


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("player")
    ap.add_argument("station")
    ap.add_argument("--flag", default=None)
    args = ap.parse_args()

    player = read(args.player)
    station = read(args.station)
    problems = []

    if len(player) != len(station):
        print(f"sizes differ: {len(player)} vs {len(station)} bytes "
              f"(the controls variant is code, so this is expected)")
        if abs(len(player) - len(station)) > CONTROLS_BUDGET:
            problems.append("size difference is larger than the controls variant")
    else:
        ranges = diff_ranges(player, station)
        total = sum(end - start for start, end in ranges)
        leaked = sum(1 for start, end in ranges for i in range(start, end)
                     if player[i] != 0)
        print(f"size: {len(player)} bytes (identical)")
        print(f"differing bytes: {total} in {len(ranges)} region(s)")
        print(f"  flag data (zero in the player build): {total - leaked}")
        print(f"  controls variant: {leaked}")
        for start, end in ranges[:6]:
            print(f"  0x{start:08x}..0x{end:08x} ({end - start} bytes)")

        if total > PAYLOAD_BUDGET + CONTROLS_BUDGET:
            problems.append(f"{total} differing bytes exceeds the intended budget "
                            f"({PAYLOAD_BUDGET} payload + {CONTROLS_BUDGET} controls)")

    needles = [FLAG_PREFIX]
    if args.flag:
        needles.append(args.flag.encode())
    # The player build ships zeroed, so the flag must never appear in it.  The
    # station build carries the flag as plain text by design, so only the
    # development markers are checked there.
    for name, blob in (("player", player), ("station", station)):
        if name == "player":
            for needle in needles:
                if needle in blob:
                    problems.append(f"{name} build contains plaintext {needle!r}")
        for marker in FORBIDDEN:
            if marker in blob:
                problems.append(
                    f"{name} build leaks development string {marker!r}")

    if problems:
        for problem in problems:
            print(f"FAIL: {problem}", file=sys.stderr)
        return 1

    print("ok: only the flag data and the controls variant differ; "
          "no plaintext flag in the player build and no development "
          "strings in either build")
    return 0


if __name__ == "__main__":
    sys.exit(main())
