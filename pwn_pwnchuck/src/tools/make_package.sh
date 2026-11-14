#!/bin/zsh
#
# Assemble the package handed to the teams: the redacted build, the Dolphin
# profile and the player notes.  Only these files leave the repo.
#
#   make dist && tools/make_package.sh
#
set -eu

ROOT="${0:A:h:h}"
cd "$ROOT"

OUT="dist/player-package"
ELF="dist/pwnchuck-player.elf"
DOL="dist/pwnchuck-player.dol"

[[ -f "$ELF" ]] || { print -u2 "tools/make_package.sh: run 'make dist' first"; exit 1 }

rm -rf "$OUT"
mkdir -p "$OUT/dolphin-profile/Config"

cp "$ELF" "$OUT/pwnchuck.elf"
cp "$DOL" "$OUT/pwnchuck.dol"
cp PLAYERS.md "$OUT/README.md"
cp dolphin-profile/README.md "$OUT/dolphin-profile/README.md"
cp dolphin-profile/Config/WiimoteNew.ini "$OUT/dolphin-profile/Config/WiimoteNew.ini"
cp dolphin-profile/Config/GFX.ini "$OUT/dolphin-profile/Config/GFX.ini"

# The shipped build must not carry the flag, the development cheats or any
# other development-only string.  tools/strings.py lists everything it does
# carry; this is the last gate before the files leave the repo.
for f in "$OUT/pwnchuck.elf" "$OUT/pwnchuck.dol"; do
	for marker in "srdnlen{" "DEV:" "seen btns" "1=debug" "2=respawn" \
		      "nunchuk" "wiimote" "scratch slot"; do
		if grep -q "$marker" "$f"; then
			print -u2 "refusing to package: $f leaks '$marker'"
			exit 1
		fi
	done
done

print "player package ready:"
find "$OUT" -type f | sort
