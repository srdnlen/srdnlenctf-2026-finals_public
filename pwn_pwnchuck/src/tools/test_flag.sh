#!/bin/zsh
#
# Round trip a generated flag payload through the game's decoder.
#
#   tools/test_flag.sh ['srdnlen{...}']
#
set -eu

ROOT="${0:A:h:h}"
cd "$ROOT"

FLAG="${1:-srdnlen{payload_round_trip}}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

python3 tools/mkflag.py --flag "$FLAG" --name flag_data_gen --out-dir "$TMP"

cc -std=c99 -Wall -Wextra -DPWNCHUCK_HOST -I source -I "$TMP" \
	-DFLAG_DATA_HEADER='"flag_data_gen.h"' -DEXPECTED="\"$FLAG\"" \
	-o "$TMP/test_flag" source/flag.c "$TMP/flag_data_gen.c" tests/test_flag.c
"$TMP/test_flag"
print "flag payload round trip: ok"
