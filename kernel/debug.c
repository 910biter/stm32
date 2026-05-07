#include "rtos.h"

#include <stddef.h>

volatile uint32_t rtos_debug_snapshot_count;
volatile rtos_task_snapshot_t rtos_debug_snapshots[RTOS_MAX_TASKS];
volatile rtos_cpu_usage_snapshot_t rtos_cpu_usage_snapshot;

void rtos_debug_update(void)
{
    uint32_t count = rtos_task_count();
    uint32_t idle_ticks = 0;
    uint32_t total_ticks;
    uint32_t active_ticks;

    if (count > RTOS_MAX_TASKS) {
        count = RTOS_MAX_TASKS;
    }

    for (uint32_t i = 0; i < count; ++i) {
        rtos_task_t *task = rtos_task_at(i);

        rtos_debug_snapshots[i].name = task->name;
        rtos_debug_snapshots[i].task = task;
        rtos_debug_snapshots[i].sp = task->sp;
        rtos_debug_snapshots[i].stack_base = task->stack_base;
        rtos_debug_snapshots[i].stack_words = task->stack_words;
        rtos_debug_snapshots[i].stack_used_words = rtos_task_stack_used_words(task);
        rtos_debug_snapshots[i].delay_ticks = task->delay_ticks;
        rtos_debug_snapshots[i].base_priority = task->base_priority;
        rtos_debug_snapshots[i].priority = task->priority;
        rtos_debug_snapshots[i].state = task->state;
        rtos_debug_snapshots[i].wait_result = task->wait_result;
        rtos_debug_snapshots[i].stack_guard_ok = (uint32_t)rtos_task_stack_guard_ok(task);
        rtos_debug_snapshots[i].switch_count = task->switch_count;
        rtos_debug_snapshots[i].run_ticks = task->run_ticks;

        if (task->base_priority == RTOS_IDLE_PRIORITY) {
            idle_ticks += task->run_ticks;
        }
    }

    rtos_debug_snapshot_count = count;
    total_ticks = rtos_tick_count();
    if (idle_ticks > total_ticks) {
        idle_ticks = total_ticks;
    }
    active_ticks = total_ticks - idle_ticks;
    rtos_cpu_usage_snapshot.total_ticks = total_ticks;
    rtos_cpu_usage_snapshot.idle_ticks = idle_ticks;
    rtos_cpu_usage_snapshot.active_ticks = active_ticks;
    if (total_ticks != 0U) {
        rtos_cpu_usage_snapshot.idle_permille = (idle_ticks * 1000U) / total_ticks;
        rtos_cpu_usage_snapshot.active_permille = (active_ticks * 1000U) / total_ticks;
    } else {
        rtos_cpu_usage_snapshot.idle_permille = 0;
        rtos_cpu_usage_snapshot.active_permille = 0;
    }
    rtos_object_debug_update();
}

uint32_t rtos_debug_snapshot(rtos_task_snapshot_t *out, uint32_t max_count)
{
    uint32_t count;

    if ((out == NULL) || (max_count == 0U)) {
        return 0;
    }

    rtos_enter_critical();
    rtos_debug_update();
    count = rtos_debug_snapshot_count;
    if (count > max_count) {
        count = max_count;
    }

    for (uint32_t i = 0; i < count; ++i) {
        out[i].name = rtos_debug_snapshots[i].name;
        out[i].task = rtos_debug_snapshots[i].task;
        out[i].sp = rtos_debug_snapshots[i].sp;
        out[i].stack_base = rtos_debug_snapshots[i].stack_base;
        out[i].stack_words = rtos_debug_snapshots[i].stack_words;
        out[i].stack_used_words = rtos_debug_snapshots[i].stack_used_words;
        out[i].delay_ticks = rtos_debug_snapshots[i].delay_ticks;
        out[i].base_priority = rtos_debug_snapshots[i].base_priority;
        out[i].priority = rtos_debug_snapshots[i].priority;
        out[i].state = rtos_debug_snapshots[i].state;
        out[i].wait_result = rtos_debug_snapshots[i].wait_result;
        out[i].stack_guard_ok = rtos_debug_snapshots[i].stack_guard_ok;
        out[i].switch_count = rtos_debug_snapshots[i].switch_count;
        out[i].run_ticks = rtos_debug_snapshots[i].run_ticks;
    }
    rtos_exit_critical();

    return count;
}

rtos_cpu_usage_snapshot_t rtos_debug_cpu_usage_snapshot(void)
{
    rtos_cpu_usage_snapshot_t snapshot;

    rtos_enter_critical();
    rtos_debug_update();
    snapshot.total_ticks = rtos_cpu_usage_snapshot.total_ticks;
    snapshot.idle_ticks = rtos_cpu_usage_snapshot.idle_ticks;
    snapshot.active_ticks = rtos_cpu_usage_snapshot.active_ticks;
    snapshot.idle_permille = rtos_cpu_usage_snapshot.idle_permille;
    snapshot.active_permille = rtos_cpu_usage_snapshot.active_permille;
    rtos_exit_critical();

    return snapshot;
}
