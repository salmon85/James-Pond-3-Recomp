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
- [x] Fix the title-menu duplication (earlier MAME-based "authentic" claim
      was wrong). The title code clears reg[0] bit 4 and writes VSRAM[1]=0 at
      `$09F71A`, but the scanline scheduler still delivered the H-blank
      interrupt it had latched at line start; the `$09FA16` handler then
      restored a stale `$03AA` plane-B scroll and drew a second menu for the
      60-frame window. H-int enable is now re-read at delivery. Frames
      10704-10762 show one menu and the logo fly-in is unchanged.
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
- [x] Slot 1's custard monsters (type 4, behavior pointers `$0CA17C` /
      `$0CA1B8`) were frozen, intangible, and harmless. Their handlers are
      reached only by computed dispatch, so the tier-3 capsule ran `$0AB650`
      once and declined it. Promoted the handlers, the `jmp (pc,d0.w)` ladder
      entries (`$0AB678/$0AB67C`, `$0AB704-$0AB714`), and the `pea`
      continuation targets (`$0AB66E`, `$0AB6F4`). A7 now stays put over
      1,300 frames (it leaked 4 bytes per call when only the bodies were
      promoted), the custards walk, collide, and cost the player HUD health.
- [x] Audited the `$0BD496` misaligned targets (`$0BD4A8`, `$0BD524`,
      `$0BD528`, `$0BD618`). They only appeared in old logs after A7 had
      already collapsed to a wild value (`A7=$000022`, `$FFFD10` holding stack
      bytes), so they were stack-corruption artifacts, not a real dispatcher
      offset. Slots 2-5 now run 700 frames each with zero misses.
- [ ] The tier-3 capsule accepts an interior-label miss when a `pea`
      continuation is on top of A7, but the native computed-JMP caller does
      not pop it; the result is a 4-byte-per-call guest-stack leak. Promoting
      each computed-jump ladder entry avoids it. The recompiler should emit an
      in-function switch for `jmp (pc,d0.w)` tables (same class as the
      `$0BD496` item above).
- [ ] Validate later areas, enemies, and level completion against NTSC MAME.
- [x] The development `--wav` capture always reported 0 bytes because the
      input-script EXIT path returned from `main()` before the cleanup that
      rewrites the RIFF/data sizes. A scripted run now stops the capture too;
      a 661-frame slot-1 capture reads back as 11.03 s of non-silent audio.

- [x] Slot 1's map song stopped after its first pass. The loop flag (bit 3 of
      the sound mailbox at Z80 `$0102`) is written only by the 68K sound API,
      chosen from the `$1FD912` table, but the Z80 driver's read-modify-write
      of that byte (`$0834` set 7, `$085A` res 7, `$088B` res 0) can straddle
      a 68K `bset`/`bclr` in an earlier scanline slice and write back a stale
      copy; the song then ends and `$1FD83C` is never re-issued. Z80 writes to
      `$0102` now keep bit 3 from RAM, and a load rebuilds the flag from the
      table for states captured before that (slot 1 reports `$0102 30->38`).
      Slot 1 then plays 50 s without touching the mixer's 380 silence floor,
      a fresh map entry still loops (390 s of music), and slots 2-5 load with
      no repair needed.

- [x] Slot 2's jetpack rats and bees stayed frozen: their handlers live only
      in the object behavior table at `$0C9B10` and were reached by computed
      `jsr (a5)`, so every one ran in the Tier-3 capsule. Promoted all
      remaining type-table handlers ($00-$D5) to `game.toml`, plus the
      debrief H-int body `$0AE182` and the mode-loop return label `$0B731E`
      that the dispatch still reported. Slot 2 now moves and animates its
      enemies with no unique dispatch/interior misses through frame 1,800.
- [x] Slot 3's boss-death debrief drew the arena floor through the item grid
      and left the FISH background wrong. The debrief entry `$0AE14C` jumps
      straight to the text window and never runs the level-exit plane-B clear
      at `$0B5832`, so plane B still held the level tiles. A game hook now
      clears VRAM `$E000-$EFFF` once at `$0AE14C`; frames 400-1,600 show a
      black background and the full 11+11 item-grid layout, and pressing C
      continues to the map. Item tiles are uploaded per collected item, so
      the icons reflect the save's inventory (18 here) rather than the
      different inventory in the hardware capture.
- [x] Slot 1's save crashed about 120 frames after loading, ending in an
      `unrunnable $FFFD10` and a garbage screen. The type-$3D handler
      `$0AC524` pushes the `$0AC53E` continuation and jumps through
      `jmp $ac542(pc,d0.w)`, but slots `$0AC542`/`$0AC546` had no native
      bodies. The tier-3 capsule ran the ladder, stopped at the body's RTS,
      peeked the pushed continuation as its return, and the native caller
      popped the wrong slot — a 4-byte leak per frame until A7 crossed and
      overwrote the `$FFFFFD10` V-int trampoline. Promoted both ladder slots
      and the `$0AC53E` continuation; 1,561 frames now run with a stable A7
      and zero floor, dispatch, or stack events.

- [x] In-game trainer menu (`T`): infinite lives, infinite health (current
      and max health held at 5 so a lost life cannot reset the bar), and
      infinite frog lives. The runner owns the menu, selection and overlay;
      the game supplies the labels and `jp3_trainer_apply` for
      `$FFFA9E`/`$FFFAA0`/`$FFFAA2`/`$FFFB86`. Toggles live in host state,
      so they survive level changes and save loads. Verified with a live
      X11 menu run, a level reload with health still on, and RAM dumps
      showing poked 1s restored to 9 lives / 5 health / 9 frog lives,
      while an empty mask leaves poked values untouched.
- [x] Trainer: infinite jumps. The game only launches a jump while
      `$3D(player)` is zero and it stays nonzero until landing, so the
      toggle instead rewrites the 16.16 vertical velocity at `$30(player)`
      on each fresh B press in mid-air (same field the ground jump handler
      sets, boosted to `$AA000`). Verified headless: four mid-air presses
      climb from y=4080 to y=3525 while the same script with the toggle off
      does one normal jump; 1,200+ further frames run with no strict-stack
      mismatch and `$FFFD10` intact.
- [x] Trainer: infinite jetpack fuel. Flight drains the player's `$D3`
      fuel counter (`$FFFAD9`) from 0 to 128 over ~540 frames, then the
      jetpack cuts out. Holding that byte at 0 while the toggle is on
      keeps the gauge full and the flight going; slot 1 climbs from y=3941
      past 1,200 frames (y=390) where the untoggled run stalls at ~frame
      540. HUD canister stays full, strict stack stays balanced, and
      `$FFFD10` remains intact.

- [x] Slot 1 crashed a few hundred frames after loading its later-area save:
      A7 walked down 4 bytes per frame until it overwrote the `$FFFD10`
      V-int trampoline (`unrunnable RAM code $FFFD10`). The live type-$4E
      object runs `$0ACF76`, which pushes the `$0ACF90` continuation and
      jumps through `jmp $ACF94(pc,d0.w)`; the ladder slots and continuation
      were not native, so the capsule left the pushed longword on the guest
      stack. Promoted `$0ACF94`, `$0ACF98`, and `$0ACF90` (same class as the
      `$0AC524`/`$0AC53E` fix). Slots 1-5 now run 2,000+ frames each with
      `GENESIS_STRICT_JSR_STACK=1` and zero mismatches, `$FFFD10` intact,
      and zero dispatch/floor/interior-label evidence entries.

## Final step

- [ ] Create a standalone setup/builder program that asks the user for the
      game ROM, verifies it against the expected SHA-1/CRC, and then builds
      the game. If needed, copy the ROM into a `game` folder stored under
      `%APPDATA%\<game>` on Windows or `~/.config/<game>` on Linux.
      Keep this item last: any new todo entries go above it.
