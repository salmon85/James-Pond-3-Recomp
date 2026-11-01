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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define JP3_ENTRY_PC   0x000200u
#define JP3_HBLANK_PC  0xFFFFFD0Au
#define JP3_VBLANK_PC  0xFFFFFD10u

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
static int jp3_instruction_hook(uint32_t pc)
{
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

/* Portable (cross-process/legacy) loads restart the game fiber at the
 * main-loop top instead of resuming the saved host stack. A save captured in
 * an interrupt-masked transition then deadlocks in WaitForVint ($0B620E):
 * $FC0E stays clear because V-int never runs, and the loop at $0B6224 spins
 * forever. Transient object slots with type 0-3 are also only meaningful at
 * the captured yield; the main-loop scan dispatches them through the
 * $0A8F56 table into the VDP halt at $0B6D20. Re-enable interrupts and free
 * those slots before the restart. Same-process exact resumes skip this. */
static void jp3_portable_load_fixup(void)
{
    uint16_t old_sr = g_cpu.SR;
    g_cpu.SR &= (uint16_t)~0x0700u;   /* V-int/H-int enabled again */

    int freed = 0;
    if (g_ram[0xFCEC] == 0xFF && g_ram[0xFCED] == 0xFF &&
        g_ram[0xFCEE] == 0xFA && g_ram[0xFCEF] == 0x06) {
        for (unsigned i = 0; i < 32; i++) {
            uint8_t *type = &g_ram[0x46ACu + i * 0x82u + 0x1Eu];
            if (*type < 4) { *type = 0xFF; freed++; }
        }
    }
    if (old_sr != g_cpu.SR || freed)
        fprintf(stderr, "[LOAD] portable fixup: SR %04X->%04X, freed %d transient slots\n",
                (unsigned)old_sr, (unsigned)g_cpu.SR, freed);
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

    /* $0A1E5E is the steady-state frame-loop top (the loop that ends with
     * `bra $0A1E5E`). Saves are captured at the WaitForVint yield inside this
     * loop, so re-entering here resumes map and level states alike. The old
     * $0B72C8 entry re-ran mode init and broke cross-process map saves.
     * Same-process saves restore the suspended fiber snapshot and skip this. */
    .resume_main_loop_pc    = 0x0A1E5Eu,
    .save_resume_pc         = NULL,
    .dispatch_main_loop_pc  = 0u,
    .call_periodic          = NULL,
    .portable_load_fixup    = jp3_portable_load_fixup,

    .on_post_reset          = NULL,
    .on_frame_pre           = NULL,
    .on_frame_post          = NULL,
    .on_hblank              = NULL,

    .handle_arg             = NULL,
    .arg_usage              = NULL,
    .dispatch_override      = NULL,
    .instruction_hook       = jp3_instruction_hook,

    .fill_frame_record      = NULL,
    .frame_record_version   = 0u,

    .commands               = NULL,
    .command_count          = 0,
};
