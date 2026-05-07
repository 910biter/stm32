#include "rtos.h"

#include "port.h"

static volatile uint32_t tick_count;

void rtos_tick_handler(void)
{
    tick_count++;
    rtos_task_tick();
    rtos_debug_update();
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
