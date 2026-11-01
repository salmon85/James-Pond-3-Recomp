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

## Save states

`Shift+F1..F9` saves, `F1..F9` loads; on a gamepad, LB saves slot 1 and RB
loads it. A save contains the portable CPU/RAM/VDP image plus an embedded
in-process rollback snapshot of the suspended game fiber. Loading it in the
session that wrote it restores the exact yield point — same game mode, frame
position, scheduler state and pending work — so repeated loads are
deterministic. Saves from another process or an older build lack a usable
fiber-stack mapping; they restore the portable image and restart the game
fiber at the steady-state frame loop (`$0A1E5E`), which resumes map and level
states alike. (The old `$0B72C8` dispatcher entry re-ran mode init and broke
cross-process map saves.)

## Status

- [x] Recompiler builds on Linux; ROM identity verified.
- [x] 68K→C generation succeeds with no unsupported instruction reports.
- [x] Native executable boots the retail ROM through the Sega init/checksum
      path, EA logo, intro story, title screen, and menu.
- [x] First gameplay level ("Garden of Edam") renders with terrain, HUD,
      player sprite, camera, and working input.
- [x] Movement, jumping, tongue attack, and object/state-table dispatch run
      through native code. The corrected build completes a 12,121-frame
      move/jump/attack probe with no dispatch, floor, or interior-label
      misses.
- [x] 60 Hz (NTSC): the game reads `$A10001 = $80` and sets
      `$FFFD18 = $FF` (export) / `$FFFD19 = $00` (NTSC). The runner pacer is
      59.94 fps.
- [x] Jump-freeze regression fixed: the decompressor epilogue no longer leaks
      four bytes of guest stack per call. Cold boot, move/jump/attack, and
      repeated-jump probes pass with `GENESIS_STRICT_JSR_STACK=1`.
- [x] Level-freeze regression fixed: the `$0B5D54` VDP setup continuation is
      now a native generated function. The former tier-3 capsule framed it
      A7-neutrally and exited at `A7=$22`; a 60-second audio-enabled idle in
      Garden of Edam now completes without a freeze.
- [x] Time-based `$0B6FAA` livelock fixed: V/H interrupts are masked for the
      Z80 bus request/ACK helper so an interrupt cannot release the bus
      between the request and its poll.
- [x] Frog/sky state corruption fixed: the `$0B5AE6` VDP dispatcher's table
      targets `$0B5AF4`, `$0B5AF8`, and `$0B5AFC` are native, so the level
      update no longer runs through an A7-neutral floor capsule.
- [x] `$0BD424` object-state dispatch clamped to its five valid table entries;
      a corrupted `FCF4` no longer jumps into instruction operands. A
      5,321-frame active-play run completed with zero unique dispatch misses
      and no freeze.
- [x] Sprint/off-floor freeze fixed: computed-jump dispatch now carries the
      path-local stack delta, so split map-builder epilogues return through
      their caller instead of unwinding the game fiber. The extended
      sprint/jump/attack probe completes 80,000 frames with
      `GENESIS_STRICT_JSR_STACK=1`, no stall, and no freeze. Live play
      through both prior freeze locations, including the hidden trigger that
      spawns the extra moons, also completes cleanly.
- [x] Save states resume the suspended game fiber from an embedded rollback
      snapshot instead of restarting at a guessed main-loop PC. 20
      consecutive loads of one save each restored the exact yield point and
      advanced the game; no watchdog, dispatch, or illegal-opcode events.
- [x] Saves written by another process (or an older build) fall back to the
      portable RAM/VDP restore plus a restart at the steady-state frame loop
      (`$0A1E5E`). Map saves resume on the map and level saves resume
      in-level; James Pond 3 also repairs its interrupt mask and frees
      transient object slots at load time. The original save file is not
      modified.
- [x] Optional reverse-debug build (`-DSONIC_REVERSE_DEBUG=ON`) exposes the
      current generated function and block trail for GDB/TCP capture.
- [x] Audio output verified from Garden of Edam with an SDL disk capture:
      varying music and tonal peaks, and the Z80 driver is released from reset
      with an advancing PC. (The separate development `--wav` capture path
      still writes a constant offset; normal SDL playback is unaffected.)
- [ ] Validate full-level progression and later areas against a trusted
      emulator.
- [x] The one-frame title/menu logo tear while the menu wipes in is the
      original game's raster transition: the same frame appears in NTSC MAME.
      Stable menu frames and gameplay are clean.
- [x] The early menu/duplicated-entry overlap is gone after removing the
      invalid object-handler promotions: the intro text now clears first, and
      stable frames show a single START/PASSWORD/OPTIONS list.

The only audited runtime-driven entries retained are `$0B5B00` (VDP/scroll
helper) and `$0A8016` (tilemap-blitter jump target). The `$0BD496` object-update
cluster also produced runtime targets, but `$0BD4A8`, `$0BD524`, `$0BD528`,
and `$0BD618` decode to mid-instruction operands. Promoting `$0BD524` made the
recompiler execute the `$F836` operand as an opcode and abort. They are left as
declined floor misses; the corrected build does not abort.

## Layout

```
CMakeLists.txt        top-level build
game/game.toml        ROM identity, discovery seeds, RAM layout
game/jp3_spec.c       GameSpec: entry/IRQ vectors, ROM CRC, floor policy
engine/               vendored segagenesisrecomp framework
```

`engine/` is a vendored snapshot of the framework so the repository is
self-contained. The framework is PolyForm Noncommercial 1.0.0; see
`engine/LICENSE.md` and `engine/THIRD-PARTY-LICENSES.md`.
