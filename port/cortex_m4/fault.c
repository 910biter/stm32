#include "rtos_fault.h"

#include <stddef.h>

#define SCB_CFSR    (*(volatile uint32_t *)0xE000ED28UL)
#define SCB_HFSR    (*(volatile uint32_t *)0xE000ED2CUL)
#define SCB_DFSR    (*(volatile uint32_t *)0xE000ED30UL)
#define SCB_MMFAR   (*(volatile uint32_t *)0xE000ED34UL)
#define SCB_BFAR    (*(volatile uint32_t *)0xE000ED38UL)
#define SCB_AFSR    (*(volatile uint32_t *)0xE000ED3CUL)

volatile rtos_fault_info_t rtos_fault_info;

void rtos_fault_clear(void)
{
    rtos_fault_info.active = 0;
}

void port_hardfault_handler_c(uint32_t *stacked_sp, uint32_t exc_return)
{
    rtos_fault_info.active = 1;
    rtos_fault_info.exc_return = exc_return;
    rtos_fault_info.stack_pointer = (uint32_t)stacked_sp;

    if (stacked_sp != NULL) {
        rtos_fault_info.r0 = stacked_sp[0];
        rtos_fault_info.r1 = stacked_sp[1];
        rtos_fault_info.r2 = stacked_sp[2];
        rtos_fault_info.r3 = stacked_sp[3];
        rtos_fault_info.r12 = stacked_sp[4];
        rtos_fault_info.lr = stacked_sp[5];
        rtos_fault_info.pc = stacked_sp[6];
        rtos_fault_info.xpsr = stacked_sp[7];
    }

    rtos_fault_info.cfsr = SCB_CFSR;
    rtos_fault_info.hfsr = SCB_HFSR;
    rtos_fault_info.dfsr = SCB_DFSR;
    rtos_fault_info.afsr = SCB_AFSR;
    rtos_fault_info.bfar = SCB_BFAR;
    rtos_fault_info.mmfar = SCB_MMFAR;

    while (1) {
        __asm volatile ("wfi");
    }
}
