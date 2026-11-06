# James Pond 3 builder

The builders compile a native game executable on **your** machine from **your
own** ROM. No ROM is included, and the builder never modifies it.

## Windows

1. Extract the whole zip somewhere writable.
2. Run `Build James Pond 3.exe` (or drag the ROM onto it).
3. Enter the path to `James Pond 3 (USA, Europe).md`.

The game is written to `James Pond 3 Recomp` next to the builder, together
with `jp3.md` and `SDL2.dll`.

## Linux

```sh
chmod +x JP3-Builder-x86_64.AppImage
./JP3-Builder-x86_64.AppImage
```

If FUSE is unavailable, use
`./JP3-Builder-x86_64.AppImage --appimage-extract-and-run`.

The game is written to `James Pond 3 Recomp` in the current directory.

## Requirements and notes

- 64-bit x86, ~2 GB free disk space, a few minutes of build time.
- The ROM must be the 2,097,152-byte No-Intro dump
  `James Pond 3 (USA, Europe).md` (SHA-256
  `8f52e27945b96d6c37fda28c44d39d65d805cc0b58332f9b4a2eb26dff540d29`).
- Windows 10+; Linux with glibc 2.35+ (Ubuntu 22.04 or newer).
- The compiler, CMake, Ninja and SDL2 are bundled; nothing needs to be
  installed.
- Sources: https://github.com/salmon85/James-Pond-3-Recomp
