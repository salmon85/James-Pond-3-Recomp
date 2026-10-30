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

## Status

- [x] Recompiler builds on Linux; ROM identity verified.
- [x] 68K→C generation succeeds with no unsupported instruction reports.
- [x] Native executable boots the retail ROM through the Sega init/checksum
      path and reaches the EA logo / opening credits.
- [ ] Opening sequence completes and reaches the title/menu (in progress:
      a 16-bit write in the `$0B6DEE` palette converter overruns into the
      `$D9BA` script state byte).
- [ ] 60 Hz (NTSC) mode selected and validated.
- [ ] First gameplay level validated against a trusted emulator.

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
