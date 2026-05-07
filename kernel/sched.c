#include "rtos.h"

#include "port.h"
#include "rtos_config.h"

void rtos_start(void)
{
    if (rtos_current_task == 0) {
        return;
    }

    if (rtos_create_idle_task() != 0) {
        return;
    }

    rtos_current_task->state = RTOS_TASK_RUNNING;
    port_init_scheduler();
    port_setup_systick(RTOS_CPU_HZ, RTOS_TICK_HZ);
    port_start_first_task();

    while (1) {
    }
}

void rtos_yield(void)
{
    port_trigger_pendsv();
}

void rtos_sleep(uint32_t ms)
{
    uint32_t ticks;

    if (ms == 0U) {
        rtos_yield();
        return;
    }

    ticks = ((ms * RTOS_TICK_HZ) + 999U) / 1000U;
    if (ticks == 0U) {
        ticks = 1U;
    }

    rtos_enter_critical();
    rtos_current_task->delay_ticks = ticks;
    rtos_current_task->state = RTOS_TASK_BLOCKED;
    rtos_exit_critical();

    rtos_yield();
}

void rtos_schedule_next(void)
{
    rtos_task_t *old_task = rtos_current_task;
    rtos_task_t *next_task = old_task->next;

    if (old_task->state == RTOS_TASK_RUNNING) {
        old_task->state = RTOS_TASK_READY;
    }

    while (next_task->state != RTOS_TASK_READY) {
        next_task = next_task->next;
    }

    next_task->state = RTOS_TASK_RUNNING;
    rtos_current_task = next_task;
}
