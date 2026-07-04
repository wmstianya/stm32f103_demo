/**
 * @file    test_idleAirPowerRamp.c
 * @brief   Host-side unit test for the standby-fan power ramp contract.
 * @author  Cursor Agent
 * @date    2026-07-04
 *
 * This project ships no on-target test harness and the Keil/ARMCC firmware
 * cannot be linked on a host, so this test pins the *behavioural contract* of
 * SYSTEM/system_control/system_control.c :: idleAirPowerRampStep().
 *
 * The reference implementation below MUST stay byte-for-byte equivalent to the
 * firmware helper. If the firmware ramp logic or IDLE_AIR_RAMP_STEP_PERCENT
 * changes, update this file too.
 *
 * Build & run:
 *   cc -std=c99 -Wall -Wextra -Werror -o /tmp/test_idle_ramp \
 *      tools/tests/test_idleAirPowerRamp.c && /tmp/test_idle_ramp
 *
 * Revision history:
 *   2026-07-04  Initial version.
 */

#include <stdio.h>

/* Mirror of the firmware constant (SYSTEM/system_control/system_control.h). */
#define IDLE_AIR_RAMP_STEP_PERCENT  20

/* ---- Reference implementation: mirror of the firmware helper ------------- */
static unsigned char idleAirPowerRampStep(unsigned char currentPower,
                                          unsigned char targetPower)
{
    if (targetPower > currentPower)
    {
        if (targetPower - currentPower > IDLE_AIR_RAMP_STEP_PERCENT)
            return (unsigned char)(currentPower + IDLE_AIR_RAMP_STEP_PERCENT);
        return targetPower;
    }
    if (currentPower > targetPower)
    {
        if (currentPower - targetPower > IDLE_AIR_RAMP_STEP_PERCENT)
            return (unsigned char)(currentPower - IDLE_AIR_RAMP_STEP_PERCENT);
        return targetPower;
    }
    return targetPower;
}

/* ---- Minimal Unity-style assert harness ---------------------------------- */
static int gFailures = 0;

static void assertEqualUint(unsigned int expected, unsigned int actual,
                            const char *what)
{
    if (expected != actual)
    {
        printf("  FAIL: %-42s expected=%u actual=%u\n", what, expected, actual);
        gFailures++;
    }
    else
    {
        printf("  ok  : %-42s = %u\n", what, actual);
    }
}

/* Drive the ramp to convergence with a bounded loop (no infinite loops). */
static unsigned char rampUntilSettled(unsigned char start, unsigned char target,
                                      int *outSteps)
{
    unsigned char v = start;
    int steps = 0;
    const int maxSteps = 200; /* safety bound: |0..100| / 1 well within */

    while (v != target && steps < maxSteps)
    {
        unsigned char next = idleAirPowerRampStep(v, target);
        /* single-step change must never exceed the configured slew rate */
        int delta = (next > v) ? (next - v) : (v - next);
        if (delta > IDLE_AIR_RAMP_STEP_PERCENT)
        {
            printf("  FAIL: step %d exceeded slew rate (%d)\n", steps, delta);
            gFailures++;
        }
        v = next;
        steps++;
    }
    if (outSteps)
        *outSteps = steps;
    return v;
}

int main(void)
{
    int steps = 0;

    printf("test_idleAirPowerRamp: single-step behaviour\n");
    /* ramp up is capped at the step size */
    assertEqualUint(20, idleAirPowerRampStep(0, 100), "up capped 0->100");
    assertEqualUint(70, idleAirPowerRampStep(50, 100), "up capped 50->100");
    /* ramp up snaps to target when within one step */
    assertEqualUint(60, idleAirPowerRampStep(50, 60), "up within-step 50->60");
    assertEqualUint(100, idleAirPowerRampStep(90, 100), "up within-step 90->100");
    /* ramp down is capped at the step size */
    assertEqualUint(80, idleAirPowerRampStep(100, 0), "down capped 100->0");
    /* ramp down snaps to target when within one step */
    assertEqualUint(20, idleAirPowerRampStep(30, 20), "down within-step 30->20");
    assertEqualUint(0, idleAirPowerRampStep(15, 0), "down within-step 15->0");
    /* already at target */
    assertEqualUint(60, idleAirPowerRampStep(60, 60), "settled 60->60");

    printf("test_idleAirPowerRamp: convergence (bounded loop)\n");
    assertEqualUint(100, rampUntilSettled(0, 100, &steps), "converge 0->100 value");
    assertEqualUint(5, (unsigned)steps, "converge 0->100 steps (5 = ceil(100/20))");
    assertEqualUint(0, rampUntilSettled(100, 0, &steps), "converge 100->0 value");
    assertEqualUint(5, (unsigned)steps, "converge 100->0 steps");
    assertEqualUint(60, rampUntilSettled(0, 60, &steps), "converge 0->60 value");
    assertEqualUint(3, (unsigned)steps, "converge 0->60 steps");

    if (gFailures == 0)
    {
        printf("\nALL TESTS PASSED\n");
        return 0;
    }
    printf("\n%d TEST(S) FAILED\n", gFailures);
    return 1;
}
