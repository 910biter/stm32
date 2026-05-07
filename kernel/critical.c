#include "rtos.h"

#include "port.h"

void rtos_enter_critical(void)
{
    port_enter_critical();
}

void rtos_exit_critical(void)
{
    port_exit_critical();
}
