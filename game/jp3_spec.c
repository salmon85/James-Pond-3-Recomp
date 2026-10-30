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

#define JP3_ENTRY_PC   0x000200u
#define JP3_HBLANK_PC  0xFFFFFD0Au
#define JP3_VBLANK_PC  0xFFFFFD10u

static void jp3_call_entry_point(void) { recomp_call_addr(JP3_ENTRY_PC); }
static void jp3_call_vblank(void)      { recomp_call_addr(JP3_VBLANK_PC); }
static void jp3_call_hblank(void)      { recomp_call_addr(JP3_HBLANK_PC); }

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

    .resume_main_loop_pc    = 0u,
    .save_resume_pc         = NULL,
    .dispatch_main_loop_pc  = 0u,
    .call_periodic          = NULL,

    .on_post_reset          = NULL,
    .on_frame_pre           = NULL,
    .on_frame_post          = NULL,
    .on_hblank              = NULL,

    .handle_arg             = NULL,
    .arg_usage              = NULL,
    .dispatch_override      = NULL,

    .fill_frame_record      = NULL,
    .frame_record_version   = 0u,

    .commands               = NULL,
    .command_count          = 0,
};
