# James Pond 3 — Static Recompilation

A source-level static recompilation of **James Pond 3: Operation Starfish**
(Millennium / Electronic Arts, Sega Genesis/Mega Drive, 1993) built on
[mstan/segagenesisrecomp](https://github.com/mstan/segagenesisrecomp).

The original 68000 ROM is translated to C at build time; the runner supplies
the clean-room Genesis hardware (VDP, bus, Z80, YM2612, SN76489) and SDL2
host layer. No emulator core is linked into the game executable.

## ROM

Use the No-Intro dump `James Pond 3 (USA, Europe).md` (2 MB):

| | |
|---|---|
| Size | 2,097,152 bytes |
| CRC32 | `0x26F64B2A` |
| SHA-256 | `8f52e27945b96d6c37fda28c44d39d65d805cc0b58332f9b4a2eb26dff540d29` |
| Header | `JAMES POND III`, serial `GM T-50646 -00` |
| Regions | U / E / J (ROM contains both 50 Hz and 60 Hz code paths) |

The ROM is **not** committed to this repository. Place it at the project root
with the exact filename above, or create the expected link:

```sh
ln -sf '../James Pond 3 (USA, Europe).md' game/jp3.md
```

## Build

Requirements: CMake ≥ 3.16, Ninja or Make, a C11/C++17 toolchain, SDL2
development files.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

The build regenerates the 68K→C translation from the ROM and `game/game.toml`
as part of the normal build (output under `build/generated/jp3/`). Generated
C is deliberately not tracked.

## Run

```sh
./build/JamesPond3Recomp game/jp3.md
```

Useful headless/automation flags (shared runner):

```sh
# Run 600 frames, hash the framebuffer every 60, write a PNG at exit
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
GENESIS_SCREENSHOT_AT_EXIT=/tmp/jp3.png \
./build/JamesPond3Recomp game/jp3.md --max-frames 600 --turbo --hash-frames 60
```

The runner writes runtime evidence next to the executable:
`dispatch_misses.toml`, `floor_coverage.toml`, `floor_unsafe.toml`, and
`interior_label_misses.toml`. Addresses recorded there are audited and, once
confirmed against the ROM, promoted into `game/game.toml`.

Repeatable probes live in `tools/`:

```sh
cd build
# Boot through the title/menu into Garden of Edam; writes gameplay_*.png
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  ./JamesPond3Recomp jp3.md --turbo --input-script ../tools/gameplay_probe.input

# Assert the ROM selected NTSC (60 Hz): $FFFD19=0, $FFFD18=$FF
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  ./JamesPond3Recomp jp3.md --turbo --input-script ../tools/ntsc_check.input

# Exercise movement, jumping, and tongue attacks in Garden of Edam
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  ./JamesPond3Recomp jp3.md --turbo --input-script ../tools/move_attack_probe.input

# Jump repeatedly while moving right (first-platform regression probe)
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  ./JamesPond3Recomp jp3.md --turbo --input-script ../tools/jump_probe.input
```

## Trainer

Press `T` while playing to open the trainer menu. `Up`/`Down` select,
`Enter` or `Space` toggles, `T` or `Esc` closes. The toggles are infinite
lives, infinite health (also grants the maximum five health), infinite
frog lives, infinite jumps (each press of the jump button in mid-air
boosts you upward again), infinite jetpack fuel (hold jump with the
jetpack equipped to fly without the tank draining) and infinite
invulnerability (the damage immunity you get at level start and after a
hit never expires), "exit level from pause" (the pause menu's B command
normally only works after the level has been completed) and "ammo type
override". `Left`/`Right` on the AMMO TYPE row pick the gun's ammo (apple,
strawberry or orange); while the override is on the gun always fires the
chosen type, and with it off the gun keeps whatever ammo the game gives
you. They stay enabled across levels and save loads until you quit.

## Save states

`Shift+F1..F9` saves, `F1..F9` loads; on a gamepad, LB saves slot 1 and RB
loads it. A save contains the portable CPU/RAM/VDP image plus an embedded
in-process rollback snapshot of the suspended game fiber. Loading it in the
session that wrote it restores the exact yield point — same game mode, frame
position, scheduler state and pending work — so repeated loads are
deterministic. Saves from an older build may not work as intended.
