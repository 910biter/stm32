#ifndef RTOS_TASK_H
#define RTOS_TASK_H

#include <stdint.h>

typedef void (*rtos_task_entry_t)(void *arg);
typedef void (*rtos_idle_hook_t)(void *arg);

typedef enum {
    RTOS_TASK_READY = 0,
    RTOS_TASK_RUNNING,
    RTOS_TASK_BLOCKED,
    RTOS_TASK_STOPPED,
} rtos_task_state_t;

typedef enum {
    RTOS_WAIT_NONE = 0,
    RTOS_WAIT_SLEEP,
    RTOS_WAIT_SEM,
    RTOS_WAIT_QUEUE,
    RTOS_WAIT_MUTEX,
    RTOS_WAIT_EVENT,
    RTOS_WAIT_MEMPOOL,
    RTOS_WAIT_NOTIFICATION,
} rtos_wait_type_t;

typedef struct rtos_task {
    uint32_t *sp;
    uint32_t *stack_base;
    uint32_t stack_words;
    uint32_t delay_ticks;
    uint32_t base_priority;
    uint32_t priority;
    rtos_task_state_t state;
    struct rtos_task *next;
    struct rtos_task *wait_next;
    const char *name;
    rtos_wait_type_t wait_type;
    int32_t wait_result;
    uint32_t wait_flags;
    uint32_t wait_flags_result;
    uint32_t wait_flags_all;
    uint32_t wait_flags_clear;
    void *wait_object_result;
    uint32_t notify_value;
    uint32_t notify_pending;
    uint32_t switch_count;
    uint32_t run_ticks;
} rtos_task_t;

extern rtos_task_t *rtos_current_task;

int rtos_task_create_named(rtos_task_entry_t entry, void *arg, uint32_t priority, const char *name);
int rtos_create_idle_task(void);
uint32_t rtos_task_count(void);
rtos_task_t *rtos_task_at(uint32_t index);
uint32_t rtos_task_stack_used_words(const rtos_task_t *task);
rtos_task_t *rtos_task_self(void);
int rtos_task_stack_guard_ok(const rtos_task_t *task);
void rtos_check_stack_guards(void);
void rtos_schedule_next(void);
void rtos_task_tick(void);
void rtos_task_account_tick(void);
void rtos_task_delete_self(void);
void rtos_task_exit(void);
int rtos_idle_set_hook(rtos_idle_hook_t hook, void *arg);
int rtos_task_notify(rtos_task_t *task, uint32_t value);
int rtos_task_notify_isr(rtos_task_t *task, uint32_t value);
int rtos_task_notify_wait(uint32_t clear_on_entry,
                          uint32_t clear_on_exit,
                          uint32_t *value,
                          uint32_t timeout_ms);

#endif
