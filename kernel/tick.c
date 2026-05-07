#include "rtos.h"

#include "port.h"

static volatile uint32_t tick_count;

void rtos_tick_handler(void)
{
    tick_count++;
    port_trigger_pendsv();
}

uint32_t rtos_tick_count(void)
{
    return tick_count;
}
