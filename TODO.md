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
- [x] Clamped the `$0BD424` object-state dispatch to its five-entry table;
      corrupted FCF4 values no longer jump into instruction operands
      (`$0BD524`/`$0BD618`).
- [x] 5,321-frame active movement/jumping/attacking run (debug build) with
      zero unique dispatch misses, no floor/illegal events, and no freeze.
- [x] Reproduced the sprint/off-floor freeze as a lost control-flow edge:
      the computed jump at `$0B18E6` dropped the path-local stack delta, so
      the `$0B1C5E` epilogue RTS mistook the register restore for skipped
      return slots and unwound the game fiber.
- [x] Carry `_sp_popped` through `hybrid_jmp_interpret`; the 48,161-frame
      normal sprint probe, the 80,000-frame benchmark sprint probe, and the
      80,000-frame release probe with `GENESIS_STRICT_JSR_STACK=1` complete
      with no game-fiber stall, JSR-stack mismatch, illegal opcode, or
      freeze.
- [x] Add a game-fiber-only block trail (`crash_trail_main`), `sched_info`,
      and `[SCHED-STUCK]` logging to the reverse-debug surface.
- [x] Live validation: both prior freeze locations now complete, including
      the hidden trigger that spawns the extra moons; no crash or stall.
- [x] Promote the Camonbert Canyon enemy update handlers and their state
      ladders (`$0A9FC6`, `$0AC402`, `$0AC41A`, `$0AC41E`, `$0AC2B6`) so
      enemies move and take damage instead of being declined by the Tier-3
      capsule.
- [x] Save states embed an in-process rollback snapshot of the suspended game
      fiber, so same-process loads restore the exact yield point, game mode,
      and scheduler state. 20 consecutive loads of one save each resumed
      exactly and advanced the game with no watchdog, dispatch, or
      illegal-opcode events.
- [x] Fix the level-exit freeze (saves 2 and 4): the `$0A9A64` skip thunk
      (`addq.l #4,a7; bra.w $0A8810`) consumed the JSR return slot above the
      `$0A8EFC` continuation pushed by handler `$0A9010`. The generated split
      RTS at `$0A886C` unwound and leaked 4 bytes per exit-object update until
      it overwrote the `$FFFFFD10` V-int trampoline. Generated RTSes now
      recognize a game-specified continuation only when the routine has a
      positive skipped-return count, defer the skip to the enclosing tail
      frame, and dispatch the continuation. Both saves ran 45 s with ~2,700
      animated frames, stable A7, intact trampolines, and zero dispatch
      failures in release and debug builds. The delta gate is required: the
      same address is also a normal object-behavior return.

## Follow-ups

- [x] Cross-process/older save loads fall back to restarting the game fiber
      at `$0B72C8`; the James Pond 3 adapter now clears a saved interrupt
      mask and frees transient object slots on that path so the load no
      longer deadlocks in `WaitForVint`. Same-process loads still take the
      exact-fiber path. The user's slot-1 save is unchanged by loading.
- [x] Enemy-fired apples/orbs sat still because their behavior entries
      surfaced only through computed dispatch (`$0ABB3A`, `$0ABBC2`,
      `$0AB854`, `$0B0C64`, `$0A9494`, `$0B76B8`) and were being run by the
      Tier-3 capsule. Promoted them to native entries; a single load now
      reports no interior-label or dispatch misses, and manual repeated-load
      testing keeps enemies and projectiles moving.
- [x] Cross-process/older map saves resumed at the mode dispatcher
      (`$0B72C8`), which re-ran level entry and left the map scrolling into a
      bogus level slice. Portable loads are now mode-aware: map saves restart
      at `$0A1E5E`, level saves at `$0B72C8`. Resuming a level save at
      `$0A1E5E` kept the map's display-only branch running, so the player
      stayed in its saved attack pose with `+0x79=$FF`, enemies stayed
      invulnerable, and object slots stopped expiring. Verified map movement,
      repeated loads, strict JSR stacks, and the map-to-level transition.
- [x] Slot 3 (a save taken after the leaked object burst) froze in the VDP
      DMA wait at `$0B5EA0`. Its saved A6 was a scratch pointer
      (`$FFFFD7D8`) rather than the VDP data port (`$00C00000`), so the
      resumed loop wrote VDP commands into WRAM and polled a WRAM word
      forever. The portable-load repair now restores A6; slot 3's 32 leaked
      objects expire back to 9 and the player state machine runs.
- [x] Minimizing the window under Wayland dropped the game to about 19.5 fps
      and made the audio robotic. The compositor stops sending frame callbacks
      to a minimized window, so the vsynced `SDL_RenderPresent` blocked and
      throttled the whole main loop, including the audio feed. KWin does not
      report a compositor-driven minimize through SDL's window flags (only a
      focus change), so the swap interval is now dropped while the window is
      unfocused and restored when focus returns; SDL-reported minimization
      also skips presentation. Verified 59.9 fps focused, minimized, and
      restored, with zero audio underrun flushes on both map and level.
- [x] Legacy in-level saves could not jump, attack, or sprint while a fresh
      boot could. Those saves carry garbage in the `$0B563A` boot block
      (`$FD1C/$FD20/$FD24` held `FFFF0000/000000B4/00183116` instead of
      `FFFFF9B9/F9BA/F9BB`), so the joypad edge copies at `$0B6EDE` landed in
      unrelated WRAM and `$F9B9-$F9BB` stayed zero. Confirmed a cold boot and
      a fresh save both keep the trio correct, then made portable loads
      restore the defaults whenever the trio is not one of the four `$09FF9A`
      controller mappings. Slot 1 then reported A/B/C edges and showed jump
      and attack on screen; saves 2 and 4 still ran 40 s animated in release
      and debug with no unrunnable RAM code.
- [x] A fully charged jetpack shot could freeze in the air and block further
      fire. The shot's state pair `$0A9A6A`/`$0A9ACC` (pointer table
      `$0C9E34`) was reached only by computed dispatch, so the tier-3 capsule
      ran `$0A9ACC` and declined it (`exit_pc=$0A8EFC` vs native `$0A9322`).
      Promoted both entries; slot 5 then fired six charged shots with no
      `$0A9ACC` miss and no persistent shot slot.
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
- Save files are only exact-resume in the process that wrote them; a save
  loaded by another process (or an older build) restores RAM/VDP and
  restarts the fiber in the loop that was running (`$0A1E5E` map,
  `$0B72C8` level), with game-specific transient-state repair.
- The title/menu wipe is a raster transition; stable menu frames are clean.

## Final step

- [ ] Create a standalone setup/builder program that asks the user for the
      game ROM, verifies it against the expected SHA-1/CRC, and then builds
      the game. If needed, copy the ROM into a `game` folder stored under
      `%APPDATA%\<game>` on Windows or `~/.config/<game>` on Linux.
      Keep this item last: any new todo entries go above it.
