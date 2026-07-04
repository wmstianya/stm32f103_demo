/**
 * @file    test_ignitionFailAction.c
 * @brief   Host-side unit test for the ignition-failure decision contract.
 * @author  Cursor Agent
 * @date    2026-07-04
 *
 * This project ships no on-target test harness and the Keil/ARMCC firmware
 * cannot be linked on a host, so this test pins the *behavioural contract* of
 * SYSTEM/system_control/system_control.c :: decideIgnitionFailAction().
 *
 * Contract: after a full in-cycle ignition batch (Max_Ignition_Times) fails,
 * grace-retry (drop to standby + cooldown, no fault) is chosen ONLY when the
 * unit is under union control AND has already burned since power-on AND the
 * grace round has not yet been used; otherwise the fault E11 is raised now.
 *
 * The reference implementation below MUST stay equivalent to the firmware
 * helper. If the firmware decision logic changes, update this file too.
 *
 * Build & run:
 *   cc -std=c99 -Wall -Wextra -Werror -o /tmp/test_ign \
 *      tools/tests/test_ignitionFailAction.c && /tmp/test_ign
 *
 * Revision history:
 *   2026-07-04  Initial version.
 */

#include <stdio.h>

typedef enum
{
    ignitionFailGraceRetry = 0,
    ignitionFailAlarm      = 1
} ignitionFailAction;

/* ---- Reference implementation: mirror of the firmware helper ------------- */
static ignitionFailAction decideIgnitionFailAction(unsigned char underUnion,
                                                    unsigned char everBurned,
                                                    unsigned char graceUsed)
{
    if (underUnion && everBurned && !graceUsed)
        return ignitionFailGraceRetry;
    return ignitionFailAlarm;
}

/* ---- Minimal assert harness ---------------------------------------------- */
static int gFailures = 0;

static const char *actionName(ignitionFailAction a)
{
    return (a == ignitionFailGraceRetry) ? "GRACE" : "ALARM";
}

static void expectAction(unsigned char u, unsigned char e, unsigned char g,
                         ignitionFailAction expected)
{
    ignitionFailAction actual = decideIgnitionFailAction(u, e, g);
    if (actual != expected)
    {
        printf("  FAIL: union=%u burned=%u graceUsed=%u -> %s (expected %s)\n",
               u, e, g, actionName(actual), actionName(expected));
        gFailures++;
    }
    else
    {
        printf("  ok  : union=%u burned=%u graceUsed=%u -> %s\n",
               u, e, g, actionName(actual));
    }
}

int main(void)
{
    printf("test_ignitionFailAction: full truth table (union x burned x graceUsed)\n");

    /* The single grace case: union control, has burned before, grace unused. */
    expectAction(1, 1, 0, ignitionFailGraceRetry);

    /* Everything else must alarm immediately. */
    expectAction(0, 0, 0, ignitionFailAlarm);
    expectAction(0, 0, 1, ignitionFailAlarm);
    expectAction(0, 1, 0, ignitionFailAlarm); /* not under union -> strict */
    expectAction(0, 1, 1, ignitionFailAlarm);
    expectAction(1, 0, 0, ignitionFailAlarm); /* first start (never burned) */
    expectAction(1, 0, 1, ignitionFailAlarm);
    expectAction(1, 1, 1, ignitionFailAlarm); /* grace already used this cycle */

    if (gFailures == 0)
    {
        printf("\nALL TESTS PASSED\n");
        return 0;
    }
    printf("\n%d TEST(S) FAILED\n", gFailures);
    return 1;
}
