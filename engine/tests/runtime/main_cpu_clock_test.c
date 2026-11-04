#include "../../runner/main_cpu_clock.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    MainCpuClock clock = {0};
    assert(main_cpu_budget(&clock, 97, 1, 1) == 97 && !clock.remainder);
    assert(main_cpu_budget(&clock, 99, 4, 3) == 132 && !clock.remainder);
    assert(main_cpu_budget(&clock, 3, 3, 2) == 4 && clock.remainder == 1);
    assert(main_cpu_budget(&clock, 3, 3, 2) == 5 && !clock.remainder);
    unsigned sum = 0;
    for (unsigned n = 0; n < 400; ++n) sum += main_cpu_budget(&clock, 7, 4, 3);
    assert(sum == 3733 && clock.remainder == 1);
    assert(main_cpu_budget(&clock, 5, 1, 1) == 5 && !clock.remainder);
    puts("Enhanced main CPU clock: native identity, 3/2 budget, fraction carry PASS");
    return 0;
}
