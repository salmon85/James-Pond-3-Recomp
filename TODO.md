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
- [x] Final-build title/menu transition: the intro text now clears before the
      menu appears, and frames 11080-11620 show a single clean set of
      START/PASSWORD/OPTIONS entries (no five-second duplication).
- [x] Fix the jump freeze: the PEA/RTS decompressor hook leaked four bytes of
      guest stack per call until A7 collapsed at `$0B5D54`. The hook now
      performs the epilogue inline and leaves the return slot to the JSR
      wrapper.
- [x] Strict-stack regression: 3,000-frame cold boot, 12,121-frame
      move/jump/attack probe, and 14,221-frame jump probe all pass with
      `GENESIS_STRICT_JSR_STACK=1` and no dispatch/floor misses.
- [x] Audio output verified from the level via SDL disk capture: nonzero,
      varying music with tonal peaks around 280-500 Hz; the Z80 driver is
      released from reset and its PC advances.
- [x] Promote the `$0B5D54` VDP setup continuation to native code instead of
      the A7-neutral tier-3 capsule; the capsule was responsible for the
      `A7=$22` freeze. A 60-second audio-enabled idle in Garden of Edam now
      completes with `GENESIS_STRICT_JSR_STACK=1`.
- [x] Fix the time-based `$0B6FAA` livelock: a V/H interrupt could run the
      matching Z80 bus release between the request write and ACK poll. Mask
      IPL 1-7 for the helper body and restore the mask at its RTS.
- [x] Add a reverse-debug build (`-DSONIC_REVERSE_DEBUG=ON`) and a GDB capture
      harness that records the current generated function, guest registers,
      recent block trail, and host backtrace on crash/freeze.
- [x] Debug capture localized the frog/sky corruption to a floor capsule at
      `$0B5AF8`, a target of the `$0B5AF4` VDP dispatcher table. Promoted
      `$0B5AF4`, `$0B5AF8`, and `$0B5AFC` so the whole table is native.
- [x] 60-second audio-enabled Garden of Edam idle with strict JSR stack checks
      completes with no floor or dispatch misses.

## Follow-ups

- [ ] Audit the upstream `$0BD496` computed-dispatch offset that produces the
      misaligned targets, so they can eventually be handled as in-function
      labels instead of declined floor misses.
- [ ] Validate later areas, enemies, and level completion against NTSC MAME.
- [ ] Investigate the development `--wav` capture path, which writes a
      constant offset; the real SDL output path plays the level audio
      correctly.

## Known limitations

- Generated C is a build artifact; never hand-edit it. Fix `game.toml`,
  discovery seeds, or the runtime instead.
- The ROM is user-supplied and is not tracked by Git.
- The title/menu wipe is a raster transition; stable menu frames are clean.
