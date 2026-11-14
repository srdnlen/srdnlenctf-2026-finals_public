# hwprobe — PwnChuck venue hardware probe

A purpose-built one-screen libogc program that is run on the real Wii before
the event to confirm the controllers behave the way the game assumes. It
mirrors the input rules of `source/input.c` (nunchuk preferred, Wiimote
fallback, rest pose learned over 30 frames, tilt = `(raw - rest) * 256 / 300`
with a deadzone of 28, clamped to +-256) and shows everything live:

- **Wiimote**: raw accelerometer X/Y/Z (10-bit, ~512 = 0 g), the learned rest
  pose, the derived tilt in the game convention (`tilt_x > 0` = RIGHT,
  `tilt_y > 0` = UP), buttons held, battery, and which sections the current
  report carries (buttons / accel / expansion / IR).
- **Extension**: the detected type as the raw `EXP_*` code (none / nunchuk /
  classic controller / guitar hero 3 / balance board / motion plus /
  unknown), and when a nunchuk is attached its raw accel, rest, tilt, stick
  and C/Z buttons.
- `>>> GAME USES:` marks which accelerometer the game will actually read
  (nunchuk when usable, otherwise Wiimote) — check the flip signs on that
  sensor.
- **FLIP CHECK**: prints the axis signs explicitly (`tilt_x > 0 = RIGHT`,
  `tilt_y > 0 = UP`) so the person holding the controller can tilt it
  forward/right and confirm the game's signs.

Controls:

- `2` — relearn the rest pose (hold the controller still at rest first).
  The probe also relearns automatically if a sensor was missing for ~1 s
  (e.g. after swapping controllers).
- `HOME` — exit.

## Build

From the repository root:

```sh
docker run --rm -v "$PWD":/project -w /project \
    devkitpro/devkitppc:latest make -C tools/hwprobe
```

This produces `tools/hwprobe/hwprobe.elf` and `tools/hwprobe/hwprobe.dol`.
The root `Makefile` is the game's `Makefile.wii` wrapper and is not involved.

## Run on a Wii

1. Copy `tools/hwprobe/hwprobe.dol` to the SD card as
   `sd:/apps/hwprobe/boot.dol` (create the folder). USB storage works the
   same way.
2. Optional, for a name/description in the Homebrew Channel: save this next
   to it as `sd:/apps/hwprobe/meta.xml`:

   ```xml
   <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
   <app version="1">
     <name>PwnChuck hwprobe</name>
     <coder>PwnChuck</coder>
     <version>1.0</version>
     <release_date>20260921000000</release_date>
     <short_description>Venue hardware probe</short_description>
     <long_description>Live Wiimote/Nunchuk accelerometer, tilt, buttons and battery for the venue check.</long_description>
   </app>
   ```

3. Boot the Homebrew Channel, select **PwnChuck hwprobe**, launch.
   No Priiloader/forwarder change is needed: the game's autoboot setup stays
   untouched, the probe is a one-off from HBC (Priiloader can also launch the
   `.dol` through its file loader).
4. Sync the station Wiimote (press `1`+`2`, or the red SYNC button).
   The sensor bar is not needed — the probe never uses IR.

Emulator note: the probe is useful in Dolphin for a quick layout/accel check
(boot the `.elf`; recent Dolphin dev builds here refuse to boot any `.dol`
with "Failed to init core", including the game's own). But **the nunchuk
never attaches in Dolphin** — the probe shows `type none (code 0)` and
`GAME USES: WIIMOTE` even when a nunchuk is configured — so the extension
half of the checklist is only meaningful on real hardware.

## Venue checklist

Run the probe on each controller set (Wiimote + nunchuk) used at the event
and record:

1. **Controller set**: model numbers (e.g. RVL-003, RVL-036 Wii Remote Plus,
   RVL-004 nunchuk, third-party) and which station it goes to.
2. **Nunchuk attaches?** Expect `type nunchuk (code 1)` and `[expansion YES]`
   on the report line. If it says `none`/`classic`/`motion plus`/`unknown`,
   or `[expansion no]` with a nunchuk plugged in, the handshake never
   completed — record it. `motion plus (code 5)` means the nunchuk is behind
   an MP dongle; the game never calls `WPAD_SetMotionPlus()`, so try a plain
   remote.
3. **Raw values at rest**: hold the controller the way you play, wait for
   `learned 30/30`, and note `raw X/Y/Z` and `rest X/Y`. Both accelerometers
   idle near 512 (the Wiimote usually reads 600+ on Z, where gravity lands,
   depending on how it is held). Anything far off (~0 or ~1023 stuck on an
   axis) is a bad unit.
4. **Flip check**: on the sensor marked `GAME USES`, tilt the controller
   "forward" (away from you — the way that should walk the player UP the
   screen). `tilt Y` must read positive, `(UP)`. Then tilt right: `tilt X`
   must read positive, `(RIGHT)`. If either reads inverted, record which
   one: the game has one sign that would need flipping (`tilt_axis()` in
   `source/input.c` combined with `source/physics.c` lines 32-33).
5. **Buttons**: `A` = brake, `B` = action on the Wiimote; `C` = brake,
   `Z` = action on the nunchuk. Confirm the probe marks them as `held`/`ON`.
6. **Battery**: record `N/255` (0 = empty, 255 = full; libogc hands out the
   raw status byte). Re-check before each session and swap in fresh cells if
   the number is dropping.
7. **Link quality**: `dropped` should stay at 0 and `err` must be
   `0 (OK)`. Note any `no controller` / `transfer error` flapping.

Finally: boot the real game and press `1` to show its own debug overlay, then
tilt once more to confirm the in-game `tilt X`/`tilt Y` signs match what the
probe recorded.

## libogc notes (things the probe had to work around)

- `WPAD_EXP_MOTIONPLUS` does not exist in this libogc's `wpad.h`; the probe
  prints the raw `d->exp.type` value (wiiuse's `EXP_*`: 0 none, 1 nunchuk,
  2 classic, 3 guitar hero 3, 4 balance board, 5 motion plus).
- `WPAD_GetStatus()` returns the WPAD subsystem state
  (`WPAD_STATE_DISABLED`/`ENABLING`/`ENABLED`), not a per-channel status; the
  per-frame error is `d->err` (`WPAD_ERR_*`). The probe shows both.
- `WPAD_DATA_EXPANSION` only appears once wiiuse has seen a `0x3x_EXP`
  report, i.e. after the extension is detected — `[expansion no]` with a
  nunchuk physically attached means detection never finished.
- `d->battery_level` is the raw status byte 0..255 (higher = fuller); the
  probe's `~%` and bar are a linear estimate of that byte.
- The console is re-initialized onto the back framebuffer every frame
  (`CON_Init` clears the whole framebuffer) and the two framebuffers are
  flipped, which keeps the text update tear-free without using GX.
