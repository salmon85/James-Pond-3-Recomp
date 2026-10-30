#include "sms_runtime.h"

#include <stdio.h>
#include <stdlib.h>

static uint16_t s_read_addresses[2];
static uint8_t s_read_values[2];
static unsigned int s_read_count;

static void check(int condition, const char *message)
{
    if (!condition) {
        (void)fprintf(stderr, "ordered_read16_test: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

uint8_t sms_read8(uint16_t addr)
{
    unsigned int index = s_read_count;

    if (index >= 2u) {
        (void)fprintf(stderr, "ordered_read16_test: unexpected extra bus read\n");
        exit(EXIT_FAILURE);
    }

    s_read_addresses[index] = addr;
    s_read_count++;
    return s_read_values[index];
}

void sms_write8(uint16_t addr, uint8_t val)
{
    (void)addr;
    (void)val;
}

uint8_t sms_io_in(uint8_t port)
{
    (void)port;
    return 0u;
}

void sms_io_out(uint8_t port, uint8_t value)
{
    (void)port;
    (void)value;
}

static void run_read_case(uint16_t address, uint8_t low, uint8_t high,
                          uint16_t expected)
{
    uint16_t next = (uint16_t)(address + 1u);
    uint16_t result;

    s_read_addresses[0] = 0u;
    s_read_addresses[1] = 0u;
    s_read_values[0] = low;
    s_read_values[1] = high;
    s_read_count = 0u;

    result = sms_read16(address);

    check(result == expected, "wrong little-endian assembled value");
    check(s_read_count == 2u, "read16 must perform exactly two byte reads");
    check(s_read_addresses[0] == address, "low byte callback must happen first");
    check(s_read_addresses[1] == next, "high byte callback must happen second");
}

int main(void)
{
    /* Distinct callback return values make byte order observable. */
    run_read_case(UINT16_C(0x1234), UINT8_C(0x34), UINT8_C(0x12),
                  UINT16_C(0x1234));
    /* Address arithmetic remains 16-bit and wraps after FFFF. */
    run_read_case(UINT16_C(0xffff), UINT8_C(0xcd), UINT8_C(0xab),
                  UINT16_C(0xabcd));
    (void)puts("ordered_read16 tests passed");
    return EXIT_SUCCESS;
}
