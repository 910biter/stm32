#ifndef RTOS_DEBUG_H
#define RTOS_DEBUG_H

#include <stdint.h>

#include "rtos_config.h"
#include "rtos_task.h"

typedef struct rtos_task_snapshot {
    const char *name;
    rtos_task_t *task;
    uint32_t *sp;
    uint32_t *stack_base;
    uint32_t stack_words;
    uint32_t stack_used_words;
    uint32_t delay_ticks;
    uint32_t base_priority;
    uint32_t priority;
    rtos_task_state_t state;
    int32_t wait_result;
    uint32_t stack_guard_ok;
    uint32_t switch_count;
    uint32_t run_ticks;
} rtos_task_snapshot_t;

typedef struct rtos_cpu_usage_snapshot {
    uint32_t total_ticks;
    uint32_t idle_ticks;
    uint32_t active_ticks;
    uint32_t idle_permille;
    uint32_t active_permille;
} rtos_cpu_usage_snapshot_t;

extern volatile uint32_t rtos_debug_snapshot_count;
extern volatile rtos_task_snapshot_t rtos_debug_snapshots[RTOS_MAX_TASKS];
extern volatile rtos_cpu_usage_snapshot_t rtos_cpu_usage_snapshot;

void rtos_debug_update(void);
uint32_t rtos_debug_snapshot(rtos_task_snapshot_t *out, uint32_t max_count);
rtos_cpu_usage_snapshot_t rtos_debug_cpu_usage_snapshot(void);

#endif
