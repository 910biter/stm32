#include "rtos.h"

#include <stddef.h>

volatile uint32_t rtos_debug_snapshot_count;
volatile rtos_task_snapshot_t rtos_debug_snapshots[RTOS_MAX_TASKS];

void rtos_debug_update(void)
{
    uint32_t count = rtos_task_count();

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
    }

    rtos_debug_snapshot_count = count;
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
    }
    rtos_exit_critical();

    return count;
}
