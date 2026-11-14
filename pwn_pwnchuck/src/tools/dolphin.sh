#!/bin/zsh
#
# Dolphin automation harness for PwnChuck development.
#
#   tools/dolphin.sh start [game.elf]  launch Dolphin with the isolated test profile
#   tools/dolphin.sh stop              kill PwnChuck Dolphin instances
#   tools/dolphin.sh status            print render window geometry and process state
#   tools/dolphin.sh shot out.png      capture the render window
#   tools/dolphin.sh hold <chord> <ms> activate Dolphin and hold a key chord
#   tools/dolphin.sh tap <chord>       hold a chord for 140 ms
#   tools/dolphin.sh zoom in out [n]   crop+upscale a screenshot for reading
#
# Chords are xdotool style: "Right", "shift+Right", "1", "e".  Keys only reach
# the emulated Wii Remote while the emulation window is frontmost, so hold
# activates Dolphin and uses peekaboo's explicit foreground delivery.
#
set -u

APP_BIN="/Applications/Emulatori/Dolphin.app/Contents/MacOS/Dolphin"
ROOT="${0:A:h:h}"
USER_DIR="$ROOT/.test/dolphin"
GAME_DEFAULT="$ROOT/dist/pwnchuck-dev.elf"
LOG="$ROOT/.test/dolphin.log"

die(){ print -u2 -- "dolphin.sh: $*"; exit 1 }

pids(){ pgrep -f "$USER_DIR" 2>/dev/null }

cmd_stop(){
	local p
	p="$(pids)"
	if [[ -n "$p" ]]; then
		kill -9 ${(f)p} 2>/dev/null
	fi
	GEOMETRY_CACHE=""
	sleep 0.4
	return 0
}

# Geometry of the emulation render window: "x y w h" (empty when absent).
# System Events is queried a few times because it occasionally times out under
# load.  Shots use a cached answer (the window does not move mid-test) but
# status always re-queries so a real death is never masked.
cmd_geometry_query(){
	local out i

	for i in 1 2 3 4; do
		out="$(osascript <<'APPLESCRIPT' 2>/dev/null
tell application "System Events" to tell process "Dolphin"
	repeat with w in windows
		try
			if (name of w) contains "JITARM64" then
				set p to position of w
				set s to size of w
				return ((item 1 of p) as text) & " " & ((item 2 of p) as text) & " " & ((item 1 of s) as text) & " " & ((item 2 of s) as text)
			end if
		end try
	end repeat
end tell
return ""
APPLESCRIPT
)"
		[[ -n "$out" ]] && break
		sleep 0.5
	done

	print -r -- "$out"
}

cmd_geometry(){
	if [[ -z "${GEOMETRY_CACHE:-}" ]]; then
		GEOMETRY_CACHE="$(cmd_geometry_query)"
	fi
	print -r -- "$GEOMETRY_CACHE"
}

# Close any blocking dialogs (analytics prompts, "stop the current emulation?").
#
# The isolated profile already has the analytics prompt answered, so in normal
# runs there is nothing to dismiss.  Keep this as a no-op: peekaboo's dialog
# command changed semantics in v4 and can send Escape, which stops the
# emulation in Dolphin and looks exactly like a game crash.
cmd_dismiss(){
	return 0
}

# Make the render window the key window by clicking its content area.
cmd_focus(){
	local g x y w h
	g="$(cmd_geometry)"
	[[ -n "$g" ]] || return 1
	x="${g%% *}"; g="${g#* }"
	y="${g%% *}"; g="${g#* }"
	w="${g%% *}"; h="${g##* }"
	# click just inside the content area (title bar is ~28 px)
	peekaboo click --coords "$((x + w / 2)),$((y + 28 + (h - 28) / 2))" --app Dolphin >/dev/null 2>&1
	sleep 0.2
	return 0
}

cmd_start(){
	local game="${1:-$GAME_DEFAULT}"
	[[ -f "$game" ]] || die "game file not found: $game"
	cmd_stop
	: > "$LOG"
	nohup "$APP_BIN" -u "$USER_DIR" -e "$game" >>"$LOG" 2>&1 &
	local i g
	for i in {1..40}; do
		sleep 0.5
		cmd_dismiss
		g="$(cmd_geometry)"
		[[ -n "$g" ]] && break
	done
	[[ -n "${g:-}" ]] || { print -u2 "dolphin.sh: render window did not appear"; tail -5 "$LOG"; return 1 }
	sleep 2
	cmd_dismiss
	print -- "render window: $g"
}

cmd_shot(){
	local out="${1:?output png required}" g x y w h
	g="$(cmd_geometry)"
	[[ -n "$g" ]] || die "no render window"
	x="${g%% *}"; g="${g#* }"
	y="${g%% *}"; g="${g#* }"
	w="${g%% *}"; h="${g##* }"
	screencapture -x -R "${x},$((y + 28)),${w},$((h - 28))" "$out"
	print -- "$out"
}

cmd_hold(){
	local key="${1:?key required}" ms="${2:-200}"
	local out

	cmd_dismiss
	# Keys only reach the emulated Wii Remote while the emulation window is
	# frontmost, so activate Dolphin and use peekaboo's explicit foreground
	# delivery (background delivery cannot be confirmed for a game window).
	osascript -e 'tell application "System Events" to set frontmost of process "Dolphin" to true' >/dev/null 2>&1
	sleep 0.3
	out="$(peekaboo press "$key" --foreground --hold "${ms}ms" 2>&1)" || die "peekaboo press failed: $out"
	case "$out" in
		*Error*|*error*|*failed*) die "peekaboo press: $out" ;;
	esac
	return 0
}

cmd_tap(){ cmd_hold "${1:?key required}" 140 }

cmd_zoom(){
	local src="${1:?source required}" dst="${2:?dest required}" scale="${3:-2}"
	python3 "$ROOT/tools/zoom.py" "$src" "$dst" --scale "$scale"
}

cmd_status(){
	local g

	g="$(cmd_geometry_query)"
	[[ -n "$g" ]] || GEOMETRY_CACHE=""
	print -- "pids: $(pids | tr '\n' ' ')"
	print -- "render window: ${g:-(none)}"
}

case "${1:-}" in
	start)  shift; cmd_start "$@" ;;
	stop)   cmd_stop ;;
	status) cmd_status ;;
	shot)   shift; cmd_shot "$@" ;;
	hold)   shift; cmd_hold "$@" ;;
	tap)    shift; cmd_tap "$@" ;;
	zoom)   shift; cmd_zoom "$@" ;;
	*) print -u2 "usage: tools/dolphin.sh {start|stop|status|shot|hold|tap|zoom} ..."; exit 2 ;;
esac
