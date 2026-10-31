# James Pond 3 recomp — work items

## Current batch

- [x] Boot the retail ROM through Sega init, EA logo, intro, title, menu.
- [x] Load Garden of Edam with terrain, HUD, sprite, camera, and input.
- [x] Confirm NTSC 60 Hz (`$FFFD18=$FF`, `$FFFD19=$00`).
- [x] Identify the move/attack display collapse: computed object/state-table
      targets were reaching the Tier-3 capsule.
- [x] Remove invalid mid-instruction promotions (`$0BD4A8`, `$0BD524`,
      `$0BD528`, `$0BD618`) that caused an illegal-opcode abort.
- [x] Re-run the 12,121-frame move/jump/attack probe on the corrected build:
      no dispatch, floor, or interior-label misses.
- [x] Compare the one-frame title/menu logo tear with NTSC MAME: the same
      transition frame exists in the emulator, so it is authentic.
- [ ] Longer gameplay validation (later areas, enemies, audio) after the
      object-dispatch correction.
- [ ] Investigate the remaining `$0BD496` computed-dispatch offset source if
      later play still produces misaligned interior targets.

## Known limitations

- Generated C is a build artifact; never hand-edit it. Fix `game.toml`,
  discovery seeds, or the runtime instead.
- The ROM is user-supplied and is not tracked by Git.
- The title/menu wipe is a raster transition; stable menu frames are clean.
