#pragma once
#include <stdint.h>

/* Optional enhanced main-program clock. num/den is the guest/native speed
 * ratio (5/4 = 1.25x): each scheduler slice budget is enlarged by that ratio,
 * carrying the remainder so the rate is exact. IRQ, DMA, raster and Z80
 * clocks keep their native timing. */
typedef struct { uint32_t remainder; } MainCpuClock;
static inline uint32_t main_cpu_budget(MainCpuClock *clock, uint32_t cycles,
                                       unsigned num, unsigned den)
{
    if (!den || num <= den) { clock->remainder = 0; return cycles; }
    uint64_t guest = (uint64_t)cycles * num + clock->remainder;
    clock->remainder = (uint32_t)(guest % den);
    return (uint32_t)(guest / den);
}
