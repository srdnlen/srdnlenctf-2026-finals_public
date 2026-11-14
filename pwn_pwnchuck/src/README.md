# PwnChuck

A Nintendo-flavoured 2D Wii homebrew game for an in-person CTF. Two players
share one Wii Remote and **tilt** it to roll through five themed rooms and
explore a hidden trophy room.

* engine: libogc / GX, C99, one binary per build, no save data
* input: accelerometer only (nunchuk preferred, Wii Remote fallback)
* designed for 30–60 minutes of play and reversing by a small team

## The world

Five walkable rooms in a row — Pipe Hall, Kong's Maze, Layton's Library,
Clockwork Tower and Crystal Cavern — lead to the hidden `ROOM_SECRET` trophy
room. Roll with tilt; rooms contain props to climb and `?` blocks that pay out
coins. Pipe Hall and Layton's Library also have background music. The status
panel shows the hint count and coin count, and the coin purse tracks an
additional admin flag.

Press **A** during play to add a torch to the inventory. The inventory is an
in-progress CTF challenge: its storage is smaller than the accepted item count,
so adding enough items writes beyond the inventory and can affect adjacent
player state, including the timer. The relevant code is in `source/game.c` and
`source/physics.c`.

## Layout

    source/          game code
      main.c         boot, video setup, frame loop
      game.c         world update, renderer, title screen, status panel
      art.c          sprites and the per-room colour themes
      sprite.c       sprite blitter
      physics.c      movement, walls, solid props
      rooms.c        room table, transitions, room audio
      ledger.c       coin counter, admin flag, step tracking, coin audio
      audio/         embedded music and coin audio data
      input.c        WPAD -> InputState (nunchuk or Wii Remote accel)
      gx2d.c         immediate-mode 2D GX renderer
      text.c         bitmap font drawing
      font5x7.c      the 5x7 glyph table `text.c` draws from
      hints.c        the cryptic hints + the library plaque
      flag.c         flag payload decode
      flag_data.[ch] generated flag payload (zeroed in player builds)
    tools/
      mkflag.py      generate the flag payload
      verify_builds.py  prove player and station builds are the same program
      test_flag.sh   payload round trip through the decoder
      make_package.sh   assemble the folder handed to the teams
      gen_font.py    bitmap font generator (with hand-tuned glyphs)
      font_preview.py  render the font to a PNG to eyeball glyph shapes
      strings.py     list the text a shipped build carries (the RE view)
      zoom.py        crop and upscale a screenshot, for reading on-screen text
      dolphin.sh     Dolphin automation (start/shot/hold) for development
      hwprobe/       venue probe: prints live controller state on a real Wii
    dolphin-profile/ Dolphin key profile, aspect fix + player instructions

    RESEARCH.md      notes on the Wiimote/Nunchuk accelerometers (dev notes)

There is no host test suite. `tests/` was removed; `make check` still calls
into it and needs a one-line fix (see Tests).

## Build

Everything builds in the devkitPro image:

    docker run --rm -v "$PWD":/project -w /project devkitpro/devkitppc:latest make dev

| target | what it produces |
|--------|------------------|
| `make dev` | development build: symbols kept, dev cheats on, payload redacted (`pwnchuck.elf` / `.dol`) |
| `make dev FLAG='srdnlen{...}'` | same, with a payload (local testing) |
| `make dist` | **player build**: stripped, payload zeroed → `dist/pwnchuck-player.{elf,dol}` |
| `make release FLAG='srdnlen{...}'` | **station build**: stripped, payload embedded → `dist/pwnchuck-station.{elf,dol}` |
| `make check` | `verify_builds.py` only — broken for now, it still calls the removed `tests/` |
| `make clean` | remove build trees |

Packaging for the teams is a separate step, after `make dist`:

    tools/make_package.sh             # -> dist/player-package/

That assembles the redacted build, the Dolphin profile and `PLAYERS.md`, and
refuses to package a build that still carries the flag or a development string.

Each target builds in its own directory (`build-dev/`, `build-dist/`,
`build-release/`), so a development build can never leak its extras into a
player build.

The flag is embedded in the station build in the clear: players only ever run
the shipped `.dol` on the station, and never receive the binary or the source,
so there is nothing to reverse. `tools/mkflag.py` writes the payload table and
`make release` rewrites it zeroed straight after the build, so the plaintext does
not linger in the source tree. The game copies the text into the hidden room
only while that room is on screen and wipes it again on the way out.

## Run in Dolphin (development)

    tools/dolphin.sh start          # launches Dolphin with the test profile
    tools/dolphin.sh shot out.png   # screenshot of the render window
    tools/dolphin.sh hold Right 1500   # hold a key (tilt: hold shift+Right)
    tools/dolphin.sh tap 2          # tap a key
    tools/dolphin.sh zoom out.png   # crop and upscale, to read on-screen text
    tools/dolphin.sh status         # is it running
    tools/dolphin.sh stop           # quit it

Open the **ELF** (not the `.dol`) — some Dolphin builds show a *Failed to init
core* dialog for the DOL path. In the emulator the game uses the Wii Remote
accelerometer through Dolphin's *Motion Simulation → Tilt*; see
`dolphin-profile/README.md` for the key bindings (tilt needs the Shift
modifier).

`make dev` also drops the development build in `dist/pwnchuck-dev.elf`, and
that is what `tools/dolphin.sh` loads: the root `pwnchuck.elf` is overwritten
by `make dist` / `make release`, so it cannot be trusted after a release build.

If the picture has black bars above and below it, Dolphin's *Auto* aspect has
guessed wrong (an ELF carries no disc header): set **Graphics → Aspect Ratio**
to **4:3**, or copy `dolphin-profile/Config/GFX.ini`.

## Run on hardware (station)

1. Build the station image: `make release FLAG='srdnlen{...}'`.
2. Copy `dist/pwnchuck-station.dol` to the SD card and point the autoboot at it
   (Priiloader forwarder or an SD autoboot title); keep the console hidden so
   players only see the game.
3. Before the event, run `tools/hwprobe` on the same console to confirm the
   controller: which extension attaches, and which way the remote tilts. If a
   tilt direction reads inverted, flip the sign of the matching axis in
   `source/input.c` (one line) and rebuild both images.

Player machines get `dist/player-package/` (from `tools/make_package.sh`) — the
redacted build, the Dolphin profile and `PLAYERS.md`. That build has the payload
zeroed, so the hidden room renders without the flag. It is there so teams can
practise the exploit path before they sit at the station.

## Tests

There is no host test suite — `tests/` was removed. `make check` still runs
`$(MAKE) -C tests` and therefore fails; drop that line from the `check` target
to restore it. What still works on the host:

* `tools/verify_builds.py` checks the player and station binaries differ only by
  the flag data and the controls variant, that the player build contains no
  flag, and that no development-only string leaked into either shipped build.
  `make check` runs it once the `tests` line is gone.
* `tools/strings.py <build>` prints the text a build carries — the view a
  reverse engineer gets.
* `tools/test_flag.sh` round trips a generated payload through the game's
  decoder.

The dev build is the fast way to check behaviour: `make dev`, then open
`dist/pwnchuck-dev.elf` in Dolphin (see above). The world logic has no host
harness, so anything beyond the build itself is verified in the emulator or on
hardware.
