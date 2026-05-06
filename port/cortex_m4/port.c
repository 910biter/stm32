#include "port.h"

void port_trigger_pendsv(void)
{
}

void port_enter_critical(void)
{
    __asm volatile ("cpsid i" ::: "memory");
}

void port_exit_critical(void)
{
    __asm volatile ("cpsie i" ::: "memory");
}
