/*
 * jp3_spec.c — GameSpec for James Pond 3: Operation Starfish (Genesis, 1993).
 *
 * ROM identity: 2 MB, CRC32 0x26F64B2A, header "JAMES POND III".
 * The retail image keeps the stock Sega init routine at $000200; its reset
 * SSP is 0 (the 68000's 24-bit address wrap makes the first push land at
 * $FFFFFC) and it installs RAM-resident H/V interrupt trampolines at
 * $FFFFFD0A/$FFFFFD10. Those two stubs are executed from live work RAM by the
 * engine's RAM-handler capsule; everything else runs as generated C with the
 * Tier-3 interpreter floor as a coverage net.
 */
#include "game_spec.h"
#include "genesis_runtime.h"
#include "genesis_machine.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define JP3_ENTRY_PC   0x000200u
#define JP3_HBLANK_PC  0xFFFFFD0Au
#define JP3_VBLANK_PC  0xFFFFFD10u

/* Trainer toggles (T menu). Health is a current/max word pair; a lost life
 * resets max to 3, so the health toggle restores both words every frame.
 * Lives are only raised to 9, never lowered, so a larger legit count stays. */
#define JP3_RAM_LIVES       0xFFFA9Eu
#define JP3_RAM_HEALTH      0xFFFAA0u
#define JP3_RAM_MAX_HEALTH  0xFFFAA2u
#define JP3_RAM_FROG_LIVES  0xFFFB86u

/* Jump gating: $3D(player) is nonzero from takeoff until landing. A fresh B
 * press in mid air rewrites the 16.16 vertical velocity at $30(player), the
 * same word the ground jump handler sets, so the player rises again. */
#define JP3_RAM_PLAYER_PTR  0xFFFCECu
#define JP3_PLAYER_JUMPING  0x3Du
#define JP3_PLAYER_VELOCITY 0x30u
#define JP3_PLAYER_JETPACK_FUEL 0xD3u
#define JP3_RAM_BUTTON_B    0xFFF9BAu
#define JP3_AIR_JUMP_VELOCITY 0x000AA000u   /* ~sqrt(2) x the stock $78000 */

/* $96(player) is the level-start / post-hit invulnerability countdown (100
 * frames). The damage routine ignores hits while it is 1..100, so any value
 * in that range protects permanently. Hold 99, not 100: $0B41E2 decrements
 * the counter and at exactly 100 replays the level music. A per-frame host
 * write can land between the decrement and that compare, so 100 retriggers
 * the music at random. 99 never produces that value and still leaves the
 * counter above zero when the player update runs twice in one frame. */
#define JP3_PLAYER_INVULN   0x96u
#define JP3_INVULN_FRAMES   99u

/* $D4(player) is the selected ammo type (0 apple, 1 strawberry, 2 orange,
 * 3 the fourth fruit). $D5(player) is the simultaneous-shot limit, not a
 * count. The shot handler builds $800|($15+$D4) and the health-bar icon is
 * $D4+1, so the trainer only overrides the type byte and leaves the game's
 * ammo otherwise untouched. */
#define JP3_PLAYER_AMMO     0xD4u

enum {
    JP3_TRAINER_LIVES        = 1u << 0,
    JP3_TRAINER_HEALTH       = 1u << 1,
    JP3_TRAINER_FROG_LIVES   = 1u << 2,
    JP3_TRAINER_INFINITE_JUMPS = 1u << 3,
    JP3_TRAINER_JETPACK_FUEL   = 1u << 4,
    JP3_TRAINER_INVULN         = 1u << 5,
    JP3_TRAINER_EXIT_LEVEL     = 1u << 6,
    JP3_TRAINER_AMMO           = 1u << 7,
};

static const char *const jp3_trainer_labels[] = {
    "INFINITE LIVES",
    "INFINITE HEALTH",
    "INFINITE FROG LIVES",
    "INFINITE JUMPS",
    "INFINITE JETPACK FUEL",
    "INFINITE INVULNERABILITY",
    "EXIT LEVEL FROM PAUSE",
    "AMMO TYPE OVERRIDE",
    NULL
};

static const char *const jp3_ammo_labels[] = {
    "APPLE", "STRAWBERRY", "ORANGE", NULL
};

static int s_jump_button_held;

/* Set by the trainer each frame; read by the pause-menu input hook. */
static int s_exit_level_enabled;

static void jp3_trainer_apply(uint32_t mask, int ammo)
{
    s_exit_level_enabled = (mask & JP3_TRAINER_EXIT_LEVEL) != 0;

    if ((mask & JP3_TRAINER_LIVES) && glue_peek16(JP3_RAM_LIVES) < 9)
        glue_poke16(JP3_RAM_LIVES, 9);
    if ((mask & JP3_TRAINER_FROG_LIVES) && glue_peek16(JP3_RAM_FROG_LIVES) < 9)
        glue_poke16(JP3_RAM_FROG_LIVES, 9);
    if (mask & JP3_TRAINER_HEALTH) {
        glue_poke16(JP3_RAM_MAX_HEALTH, 5);
        glue_poke16(JP3_RAM_HEALTH, 5);
    }

    int held = glue_peek8(JP3_RAM_BUTTON_B) != 0;
    uint32_t a0 = glue_peek32(JP3_RAM_PLAYER_PTR) & 0xFFFFFFu;
    int player_ok = a0 >= 0xFF0000u;
    if ((mask & JP3_TRAINER_INFINITE_JUMPS) && held && !s_jump_button_held) {
        if (player_ok && glue_peek8(a0 + JP3_PLAYER_JUMPING) != 0)
            glue_poke32(a0 + JP3_PLAYER_VELOCITY, JP3_AIR_JUMP_VELOCITY);
    }
    /* The jetpack fuel counter only ever rises while flying; holding it at
     * zero keeps the gauge full and the flight from cutting out. */
    if ((mask & JP3_TRAINER_JETPACK_FUEL) && player_ok)
        glue_poke8(a0 + JP3_PLAYER_JETPACK_FUEL, 0);
    if ((mask & JP3_TRAINER_INVULN) && player_ok)
        glue_poke16(a0 + JP3_PLAYER_INVULN, JP3_INVULN_FRAMES);
    /* Pin the gun to the trainer's ammo type while the override is on. A
     * pickup rewrites it for the rest of the frame that collected it; the
     * next frame pins it again. */
    if ((mask & JP3_TRAINER_AMMO) && player_ok && ammo >= 0 && ammo <= 2)
        glue_poke8(a0 + JP3_PLAYER_AMMO, (uint8_t)ammo);
    s_jump_button_held = held;
}

static void jp3_call_entry_point(void) { recomp_call_addr(JP3_ENTRY_PC); }
static void jp3_call_vblank(void)      { recomp_call_addr(JP3_VBLANK_PC); }
static void jp3_call_hblank(void)      { recomp_call_addr(JP3_HBLANK_PC); }

/* The Z80 bus helper at $0B6FA2 writes the request and then polls the ACK.
 * If a V/H interrupt runs between those two steps, its own bus release can
 * clear our request and the poll spins forever. Mask interrupts only for the
 * helper body and restore just the mask bits at its RTS (flags from the body
 * stay intact). */
static uint16_t s_busreq_sr;
static int s_busreq_depth;

/* $0B6D88/$0B6DE4 are RTSes reached after PEA #$0B6DE6. They are computed
 * jumps into the decompressor's epilogue (MOVEM restore + D0 = A5), not
 * returns. Perform the epilogue inline so the original JSR wrapper still owns
 * the return-address pop; calling it through recomp_call_addr() leaves that
 * slot behind and leaks 4 bytes of guest stack per decompression. */
static int s_fcfb_patched;
static uint8_t s_fcfb_saved;

static int jp3_instruction_hook(uint32_t pc)
{
    /* The pause-menu input builder ($0A16EE) masks B out of the accepted
     * button latch $F888 at $0A1710 unless the level-completed flag $FCFB
     * (or the exit-tile flag $F842) is set. The trainer's exit option forces
     * $FCFB for the two instructions that test it and restores the original
     * value at $0A1714, so the level-load/end-text readers never see it. */
    if (pc == 0x0A1704u) {
        if (s_exit_level_enabled && !s_fcfb_patched) {
            s_fcfb_saved = glue_peek8(0xFFFCFB);
            glue_poke8(0xFFFCFB, 1);
            s_fcfb_patched = 1;
        }
        return 0;
    }
    if (pc == 0x0A1714u) {
        if (s_fcfb_patched) {
            glue_poke8(0xFFFCFB, s_fcfb_saved);
            s_fcfb_patched = 0;
        }
        return 0;
    }
    if (pc == 0x0AE14Cu) {
        /* Boss-death debrief: $0AE14C jumps straight to the text window and
         * never runs the level-exit plane-B clear at $0B5832, so the arena
         * floor stays visible under the item grid. Hardware's debrief
         * background is black; clear plane B once on entry. */
        memset(g_machine.vdp.vram + 0xE000, 0, 0x1000);
        return 0;
    }
    if (pc == 0x0BD424u) {
        /* Five valid entries at $0BD428, stride 4. Clamp corrupted states. */
        if (g_cpu.D[0] > 16u) g_cpu.D[0] = 0u;
        return 0;
    }
    if (pc == 0x0B6FA2u) {
        if (s_busreq_depth++ == 0)
            s_busreq_sr = g_cpu.SR;
        g_cpu.SR |= 0x0700u;              /* mask IPL 1-7 */
        return 0;
    }
    if (pc == 0x0B6FB8u) {
        g_cpu.SR = (uint16_t)((g_cpu.SR & ~0x0700u) | (s_busreq_sr & 0x0700u));
        if (s_busreq_depth > 0) s_busreq_depth--;
        return 0;
    }
    if (pc == 0x0B6D88u || pc == 0x0B6DE4u) {
        uint32_t target = m68k_read32(g_cpu.A[7]) & 0xFFFFFFu;
        if (target == 0x0B6DE6u) {
            uint32_t d0_result = g_cpu.A[5];
            g_cpu.A[7] += 4;                 /* pop the PEA target */
            for (int i = 1; i <= 7; i++) {   /* MOVEM.L (A7)+,D1-D7/A0-A5 */
                g_cpu.D[i] = m68k_read32(g_cpu.A[7]);
                g_cpu.A[7] += 4;
            }
            for (int i = 0; i <= 5; i++) {
                g_cpu.A[i] = m68k_read32(g_cpu.A[7]);
                g_cpu.A[7] += 4;
            }
            g_cpu.D[0] = d0_result;
            g_cpu.SR &= (uint16_t)~0x0Fu;
            if (d0_result == 0) g_cpu.SR |= (1u << 2);
            if (d0_result >> 31) g_cpu.SR |= (1u << 3);
            return 1;
        }
    }
    return 0;
}

/* Handler $0A9010 pushes $0A8EFC as a continuation (`pea.l $a8efc(pc)`).
 * The $0A9A64 skip thunk (`addq.l #4,a7; bra.w $0A8810`) consumes the JSR
 * return slot above it, so $0A886C's RTS finds the continuation on top with a
 * positive skip still owed to the enclosing wrapper. That combination is the
 * only one where this address is a continuation; a plain object-behavior RTS
 * to the same address has no pending skip and must keep normal flow. */
static int jp3_rts_continuation(uint32_t rts_pc, uint32_t addr)
{
    return rts_pc == 0x0A886Cu && addr == 0x0A8EFCu;
}

/* Portable (cross-process/legacy) loads restart the game fiber at the
 * main-loop top instead of resuming the saved host stack. A save captured in
 * an interrupt-masked transition then deadlocks in WaitForVint ($0B620E):
 * $FC0E stays clear because V-int never runs, and the loop at $0B6224 spins
 * forever. Transient object slots with type 0-3 are also only meaningful at
 * the captured yield; the main-loop scan dispatches them through the
 * $0A8F56 table into the VDP halt at $0B6D20. Re-enable interrupts, restore
 * the VDP port register, and free those slots before the restart. Same-process
 * exact resumes skip this. */
static void jp3_portable_load_fixup(void)
{
    uint16_t old_sr = g_cpu.SR;
    g_cpu.SR &= (uint16_t)~0x0700u;   /* V-int/H-int enabled again */

    /* The per-frame loop assumes A6 still holds the VDP data port. A save
     * captured inside a routine that borrowed A6 as scratch otherwise writes
     * VDP commands into WRAM and spins forever on the DMA-busy poll at
     * $0B5EA0. */
    g_cpu.A[6] = 0x00C00000u;

    int freed = 0;
    if (g_ram[0xFCEC] == 0xFF && g_ram[0xFCED] == 0xFF &&
        g_ram[0xFCEE] == 0xFA && g_ram[0xFCEF] == 0x06) {
        for (unsigned i = 0; i < 32; i++) {
            uint8_t *type = &g_ram[0x46ACu + i * 0x82u + 0x1Eu];
            if (*type < 4) { *type = 0xFF; freed++; }
        }
    }

    /* Saves written by builds that never ran the boot block at $0B563A carry
     * garbage where the A/B/C press pointers belong. $0B6EDE then copies the
     * joypad edges into random RAM and jump/attack/sprint never arrive at
     * $F9B9-$F9BB. The only valid values are the 4 permutations in the
     * $09FF9A controller table, so anything else is restored to defaults. */
    static const uint32_t btn_ptr[3] = {0xFFFFF9B9u, 0xFFFFF9BAu, 0xFFFFF9BBu};
    uint32_t btn[3];
    for (unsigned i = 0; i < 3; i++) {
        const uint8_t *p = &g_ram[0xFD1Cu + i * 4u];
        btn[i] = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
                 ((uint32_t)p[2] << 8) | p[3];
    }
    int ptr_fixed = 0;
    if (!(btn[0] >= btn_ptr[0] && btn[0] <= btn_ptr[2] &&
          btn[1] >= btn_ptr[0] && btn[1] <= btn_ptr[2] &&
          btn[2] >= btn_ptr[0] && btn[2] <= btn_ptr[2] &&
          btn[0] != btn[1] && btn[1] != btn[2] && btn[0] != btn[2])) {
        for (unsigned i = 0; i < 3; i++) {
            uint8_t *p = &g_ram[0xFD1Cu + i * 4u];
            p[0] = (uint8_t)(btn_ptr[i] >> 24);
            p[1] = (uint8_t)(btn_ptr[i] >> 16);
            p[2] = (uint8_t)(btn_ptr[i] >> 8);
            p[3] = (uint8_t)btn_ptr[i];
        }
        ptr_fixed = 3;
    }
    if (old_sr != g_cpu.SR || freed || ptr_fixed)
        fprintf(stderr, "[LOAD] portable fixup: SR %04X->%04X, freed %d transient slots, "
                "repaired %d input pointers\n",
                (unsigned)old_sr, (unsigned)g_cpu.SR, freed, ptr_fixed);
}

/* The song loop flag is bit 3 of the sound mailbox at Z80 $0102, written only
 * by the 68K sound API ($184AF4/$184AF0, chosen by the $1FD912 table). The Z80
 * driver edits only bits 7 and 0 of that byte, but its read-modify-write can
 * straddle a 68K bset/bclr and write back a copy read before the flag changed;
 * the song then reaches its end and never auto-restarts, because $1FD83C is
 * re-issued only when the song id changes. gbus_z80_write keeps bit 3 68K-owned
 * from now on; this rebuilds it from the table for states captured before that
 * fix. $FBE6 is the current song id, and the driver only loops a song while
 * the flag agrees with the table. */
static void jp3_on_state_loaded(void)
{
    unsigned song = ((unsigned)g_ram[0xFBE6] << 8) | g_ram[0xFBE7];
    if (song > 15u) return;
    uint8_t loop = g_rom[0x1FD912u + song] ? 0x08u : 0x00u;
    uint8_t *flag = &g_machine.bus.z80_ram[0x0102];
    if ((*flag & 0x08u) == loop) return;
    uint8_t old = *flag;
    *flag = (uint8_t)((*flag & (uint8_t)~0x08u) | loop);
    fprintf(stderr, "[LOAD] sound loop flag repaired: $0102 %02X->%02X (song %u)\n",
            old, *flag, song);
}

/* Portable saves resume in the loop that was actually running. $0A1E5E is
 * the map/overworld loop; $FFFF2799 is $FF once a level is active, and the
 * in-level per-frame update lives in the $0B72C8 mode loop (it dispatches the
 * player state machine at $0B758C). Resuming a level save at $0A1E5E leaves it
 * in the map's display-only branch, so the player, enemies and object slots
 * all freeze while the screen keeps being rebuilt from stale data. */
static uint32_t jp3_save_resume_pc(uint8_t game_mode)
{
    return game_mode ? 0x0B72C8u : 0x0A1E5Eu;
}

const GameSpec g_game_spec = {
    .display_name           = "James Pond 3: Operation Starfish",
    .short_name             = "JP3",

    /* No-Intro "James Pond 3 (USA, Europe).md" */
    .expected_rom_crc32     = 0x26F64B2Au,
    .expected_rom_size      = 0x200000u,

    /* No community disassembly exists. Heuristic discovery leaves dynamic
     * object/state handlers unresolved, so keep the interpreter floor on and
     * promote its coverage into game.toml after runs. */
    .tier3_floor_default    = 1,

    .call_entry_point       = jp3_call_entry_point,
    .call_vblank            = jp3_call_vblank,
    .call_hblank            = jp3_call_hblank,

    /* Fallback for portable loads: the map/overworld frame loop. Level saves
     * are routed to $0B72C8 by save_resume_pc. Same-process saves restore the
     * suspended fiber snapshot and skip both. */
    .resume_main_loop_pc    = 0x0A1E5Eu,
    .save_resume_pc         = jp3_save_resume_pc,
    .dispatch_main_loop_pc  = 0u,
    .call_periodic          = NULL,
    .portable_load_fixup    = jp3_portable_load_fixup,
    .on_state_loaded        = jp3_on_state_loaded,

    .on_post_reset          = NULL,
    .on_frame_pre           = NULL,
    .on_frame_post          = NULL,
    .trainer_labels         = jp3_trainer_labels,
    .trainer_item_count     = 8,
    .trainer_apply          = jp3_trainer_apply,
    .trainer_choice_title   = "AMMO TYPE",
    .trainer_choice_values  = jp3_ammo_labels,
    .trainer_choice_count   = 3,
    .on_hblank              = NULL,

    .handle_arg             = NULL,
    .arg_usage              = NULL,
    .dispatch_override      = NULL,
    .instruction_hook       = jp3_instruction_hook,
    .rts_continuation       = jp3_rts_continuation,

    .fill_frame_record      = NULL,
    .frame_record_version   = 0u,

    .commands               = NULL,
    .command_count          = 0,
};
