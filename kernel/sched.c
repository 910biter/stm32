#include "rtos.h"

#include "port.h"

void rtos_start(void)
{
    if (rtos_current_task == 0) {
        return;
    }

    rtos_current_task->state = RTOS_TASK_RUNNING;
    port_init_scheduler();
    port_start_first_task();

    while (1) {
    }
}

void rtos_yield(void)
{
    port_trigger_pendsv();
}

void rtos_schedule_next(void)
{
    rtos_task_t *old_task = rtos_current_task;
    rtos_task_t *next_task = old_task->next;

    old_task->state = RTOS_TASK_READY;

    while (next_task->state != RTOS_TASK_READY) {
        next_task = next_task->next;
    }

    next_task->state = RTOS_TASK_RUNNING;
    rtos_current_task = next_task;
}
