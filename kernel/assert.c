#include "rtos.h"

volatile rtos_assert_info_t rtos_assert_info;

void rtos_assert_failed(const char *expression, const char *file, uint32_t line)
{
    __asm volatile ("cpsid i" ::: "memory");

    rtos_assert_info.active = 1;
    rtos_assert_info.expression = expression;
    rtos_assert_info.file = file;
    rtos_assert_info.line = line;
    rtos_assert_info.tick = rtos_tick_count();
    rtos_assert_info.task = rtos_current_task;

    while (1) {
        __asm volatile ("wfi");
    }
}
