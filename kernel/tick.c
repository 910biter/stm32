#include "rtos.h"

#include "port.h"

#define DEBUG_UPDATE_PERIOD_TICKS 100U

static volatile uint32_t tick_count;
static uint32_t debug_update_ticks;

void rtos_tick_handler(void)
{
    tick_count++;
    rtos_task_account_tick();
    rtos_task_tick();
    rtos_timer_tick();
    rtos_check_stack_guards();
    if (debug_update_ticks == 0U) {
        rtos_debug_update();
        debug_update_ticks = DEBUG_UPDATE_PERIOD_TICKS;
    }
    debug_update_ticks--;
    port_trigger_pendsv();
}

uint32_t rtos_tick_count(void)
{
    return tick_count;
}

uint32_t rtos_ms_to_ticks(uint32_t ms)
{
    uint32_t ticks;

    if (ms == RTOS_TIMEOUT_FOREVER) {
        return 0;
    }

    ticks = ((ms * RTOS_TICK_HZ) + 999U) / 1000U;
    if (ticks == 0U) {
        ticks = 1U;
    }

    return ticks;
}
