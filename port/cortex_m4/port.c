#include "port.h"

#include <stdint.h>

#define SCB_ICSR        (*(volatile uint32_t *)0xE000ED04UL)
#define SCB_SHPR3       (*(volatile uint32_t *)0xE000ED20UL)
#define SCB_ICSR_PENDSVSET  (1UL << 28)

void port_init_scheduler(void)
{
    SCB_SHPR3 |= 0xFFUL << 16;
}

void port_trigger_pendsv(void)
{
    SCB_ICSR = SCB_ICSR_PENDSVSET;
    __asm volatile ("dsb" ::: "memory");
    __asm volatile ("isb");
}

void port_enter_critical(void)
{
    __asm volatile ("cpsid i" ::: "memory");
}

void port_exit_critical(void)
{
    __asm volatile ("cpsie i" ::: "memory");
}
