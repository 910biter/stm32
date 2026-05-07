#ifndef RTOS_FAULT_H
#define RTOS_FAULT_H

#include <stdint.h>

typedef struct rtos_fault_info {
    uint32_t active;
    uint32_t exc_return;
    uint32_t stack_pointer;
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t dfsr;
    uint32_t afsr;
    uint32_t bfar;
    uint32_t mmfar;
} rtos_fault_info_t;

extern volatile rtos_fault_info_t rtos_fault_info;

void rtos_fault_clear(void);

#endif
