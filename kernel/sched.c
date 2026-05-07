#include "rtos.h"

#include "port.h"
#include "rtos_config.h"

volatile uint32_t rtos_scheduler_lock_count;
volatile uint32_t rtos_scheduler_pending;

void rtos_start(void)
{
    if (rtos_current_task == 0) {
        return;
    }

    if (rtos_create_idle_task() != 0) {
        return;
    }

    rtos_current_task->state = RTOS_TASK_RUNNING;
    rtos_current_task->switch_count++;
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

void rtos_sched_lock(void)
{
    rtos_enter_critical();
    rtos_scheduler_lock_count++;
    rtos_exit_critical();
}

void rtos_sched_unlock(void)
{
    int should_yield = 0;

    rtos_enter_critical();
    if (rtos_scheduler_lock_count != 0U) {
        rtos_scheduler_lock_count--;
        if ((rtos_scheduler_lock_count == 0U) && (rtos_scheduler_pending != 0U)) {
            rtos_scheduler_pending = 0;
            should_yield = 1;
        }
    }
    rtos_exit_critical();

    if (should_yield != 0) {
        rtos_yield();
    }
}

uint32_t rtos_sched_lock_depth(void)
{
    return rtos_scheduler_lock_count;
}

uint32_t rtos_in_isr(void)
{
    return port_in_isr();
}

void rtos_sleep(uint32_t ms)
{
    uint32_t ticks;

    if (ms == 0U) {
        rtos_yield();
        return;
    }

    ticks = rtos_ms_to_ticks(ms);

    rtos_enter_critical();
    rtos_current_task->delay_ticks = ticks;
    rtos_current_task->wait_type = RTOS_WAIT_SLEEP;
    rtos_current_task->wait_result = RTOS_OK;
    rtos_current_task->state = RTOS_TASK_BLOCKED;
    rtos_exit_critical();

    rtos_yield();
}

void rtos_schedule_next(void)
{
    rtos_task_t *old_task = rtos_current_task;
    rtos_task_t *scan_task = old_task->next;
    rtos_task_t *best_task = 0;

    if ((rtos_scheduler_lock_count != 0U) && (old_task->state == RTOS_TASK_RUNNING)) {
        rtos_scheduler_pending = 1;
        return;
    }

    rtos_scheduler_pending = 0;

    if (old_task->state == RTOS_TASK_RUNNING) {
        old_task->state = RTOS_TASK_READY;
    }

    do {
        if (scan_task->state == RTOS_TASK_READY) {
            if ((best_task == 0) || (scan_task->priority > best_task->priority)) {
                best_task = scan_task;
            }
        }

        scan_task = scan_task->next;
    } while (scan_task != old_task->next);

    RTOS_ASSERT(best_task != 0);
    if (best_task == 0) {
        return;
    }

    best_task->state = RTOS_TASK_RUNNING;
    best_task->switch_count++;
    rtos_current_task = best_task;
    if (best_task != old_task) {
        rtos_trace_record(RTOS_TRACE_SWITCH, (uint32_t)old_task, (uint32_t)best_task);
    }
}
