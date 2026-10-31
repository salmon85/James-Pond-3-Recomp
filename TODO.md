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
- [x] Re-run the 14,221-frame jump probe: no crash and no misses.
- [x] Live 200-action move/jump/attack fuzz: no unique dispatch misses, no
      abort, and no display corruption.
- [x] Compare the one-frame title/menu logo tear with NTSC MAME: the same
      transition frame exists in the emulator, so it is authentic.

## Follow-ups

- [ ] Audit the upstream `$0BD496` computed-dispatch offset that produces the
      misaligned targets, so they can eventually be handled as in-function
      labels instead of declined floor misses.
- [ ] Validate later areas, enemies, level completion, and audio against NTSC
      MAME.

## Known limitations

- Generated C is a build artifact; never hand-edit it. Fix `game.toml`,
  discovery seeds, or the runtime instead.
- The ROM is user-supplied and is not tracked by Git.
- The title/menu wipe is a raster transition; stable menu frames are clean.
