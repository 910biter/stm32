#include <stddef.h>

#include "rtos.h"
#include "rtos_config.h"

#define INITIAL_XPSR        0x01000000UL
#define EXC_RETURN_THREAD_PSP 0xFFFFFFFDUL
#define STACK_FILL_PATTERN  0xA5A5A5A5UL
#define STACK_GUARD_WORDS   8U

static rtos_task_t tasks[RTOS_MAX_TASKS];
static uint32_t task_stacks[RTOS_MAX_TASKS][RTOS_TASK_STACK_WORDS];
static uint32_t task_count;
static uint32_t stack_guard_check_index;
static rtos_idle_hook_t idle_hook;
static void *idle_hook_arg;

rtos_task_t *rtos_current_task;

static void idle_task(void *arg)
{
    (void)arg;

    while (1) {
        if (idle_hook != NULL) {
            idle_hook(idle_hook_arg);
        }
        __asm volatile ("wfi");
    }
}

static uint32_t *align_stack(uint32_t *sp)
{
    return (uint32_t *)((uintptr_t)sp & ~((uintptr_t)0x7U));
}

static rtos_task_t *find_task_slot(uint32_t *slot_index, int *is_new_slot)
{
    if (task_count < RTOS_MAX_TASKS) {
        *slot_index = task_count;
        *is_new_slot = 1;
        task_count++;
        return &tasks[*slot_index];
    }

    for (uint32_t i = 0; i < task_count; ++i) {
        if (tasks[i].state == RTOS_TASK_STOPPED) {
            *slot_index = i;
            *is_new_slot = 0;
            return &tasks[i];
        }
    }

    return NULL;
}

int rtos_task_create_named_handle(rtos_task_entry_t entry,
                                  void *arg,
                                  uint32_t priority,
                                  const char *name,
                                  rtos_task_t **task_out)
{
    uint32_t slot_index;
    int is_new_slot;

    if (entry == NULL) {
        return -1;
    }

    rtos_enter_critical();
    rtos_task_t *task = find_task_slot(&slot_index, &is_new_slot);
    if (task == NULL) {
        rtos_exit_critical();
        return -1;
    }

    uint32_t *stack = task_stacks[slot_index];
    uint32_t *sp = align_stack(stack + RTOS_TASK_STACK_WORDS);

    RTOS_ASSERT((((uintptr_t)sp) & 0x7U) == 0U);

    for (uint32_t i = 0; i < RTOS_TASK_STACK_WORDS; ++i) {
        stack[i] = STACK_FILL_PATTERN;
    }

    *--sp = INITIAL_XPSR;
    *--sp = ((uint32_t)entry) | 1UL;
    *--sp = (uint32_t)rtos_task_exit;
    *--sp = 0x12121212UL;
    *--sp = 0x03030303UL;
    *--sp = 0x02020202UL;
    *--sp = 0x01010101UL;
    *--sp = (uint32_t)arg;

    *--sp = 0x11111111UL;
    *--sp = 0x10101010UL;
    *--sp = 0x09090909UL;
    *--sp = 0x08080808UL;
    *--sp = 0x07070707UL;
    *--sp = 0x06060606UL;
    *--sp = 0x05050505UL;
    *--sp = 0x04040404UL;

    task->name = name;
    task->sp = sp;
    task->stack_base = stack;
    task->stack_words = RTOS_TASK_STACK_WORDS;
    task->delay_ticks = 0;
    task->base_priority = priority;
    task->priority = priority;
    task->state = RTOS_TASK_READY;
    task->wait_next = NULL;
    task->wait_type = RTOS_WAIT_NONE;
    task->wait_result = RTOS_OK;
    task->wait_flags = 0;
    task->wait_flags_result = 0;
    task->wait_flags_all = 0;
    task->wait_flags_clear = 0;
    task->wait_object_result = NULL;
    task->notify_value = 0;
    task->notify_pending = 0;
    task->switch_count = 0;
    task->run_ticks = 0;

    if (is_new_slot != 0) {
        if (task_count == 1U) {
            task->next = task;
            rtos_current_task = task;
        } else {
            task->next = rtos_current_task->next;
            rtos_current_task->next = task;
        }
    }

    if (task_out != NULL) {
        *task_out = task;
    }

    rtos_exit_critical();
    return 0;
}

int rtos_task_create_named(rtos_task_entry_t entry, void *arg, uint32_t priority, const char *name)
{
    return rtos_task_create_named_handle(entry, arg, priority, name, NULL);
}

int rtos_task_create_with_priority(rtos_task_entry_t entry, void *arg, uint32_t priority)
{
    return rtos_task_create_named(entry, arg, priority, NULL);
}

int rtos_task_create(rtos_task_entry_t entry, void *arg)
{
    return rtos_task_create_with_priority(entry, arg, RTOS_DEFAULT_PRIORITY);
}

uint32_t rtos_current_priority(void)
{
    return rtos_current_task->priority;
}

uint32_t rtos_current_base_priority(void)
{
    return rtos_current_task->base_priority;
}

int rtos_create_idle_task(void)
{
    return rtos_task_create_named(idle_task, NULL, RTOS_IDLE_PRIORITY, "idle");
}

uint32_t rtos_task_count(void)
{
    return task_count;
}

rtos_task_t *rtos_task_self(void)
{
    return rtos_current_task;
}

rtos_task_t *rtos_task_at(uint32_t index)
{
    if (index >= task_count) {
        return NULL;
    }

    return &tasks[index];
}

uint32_t rtos_task_stack_used_words(const rtos_task_t *task)
{
    uint32_t unused = 0;

    if ((task == NULL) || (task->stack_base == NULL)) {
        return 0;
    }

    while ((unused < task->stack_words) && (task->stack_base[unused] == STACK_FILL_PATTERN)) {
        unused++;
    }

    return task->stack_words - unused;
}

int rtos_task_stack_guard_ok(const rtos_task_t *task)
{
    uint32_t guard_words = STACK_GUARD_WORDS;

    if ((task == NULL) || (task->stack_base == NULL)) {
        return 0;
    }

    if (guard_words > task->stack_words) {
        guard_words = task->stack_words;
    }

    for (uint32_t i = 0; i < guard_words; ++i) {
        if (task->stack_base[i] != STACK_FILL_PATTERN) {
            return 0;
        }
    }

    return 1;
}

void rtos_check_stack_guards(void)
{
    if (task_count == 0U) {
        return;
    }

    if (stack_guard_check_index >= task_count) {
        stack_guard_check_index = 0;
    }

    RTOS_ASSERT(rtos_task_stack_guard_ok(&tasks[stack_guard_check_index]) != 0);

    stack_guard_check_index++;
}

void rtos_task_tick(void)
{
    for (uint32_t i = 0; i < task_count; ++i) {
        rtos_task_t *task = &tasks[i];

        if ((task->state == RTOS_TASK_BLOCKED) && (task->delay_ticks > 0U)) {
            task->delay_ticks--;

            if (task->delay_ticks == 0U) {
                task->wait_result = RTOS_ERR_TIMEOUT;
                task->state = RTOS_TASK_READY;
            }
        }
    }
}

void rtos_task_account_tick(void)
{
    if ((rtos_current_task != NULL) && (rtos_current_task->state == RTOS_TASK_RUNNING)) {
        rtos_current_task->run_ticks++;
    }
}

void rtos_task_delete_self(void)
{
    rtos_enter_critical();
    rtos_current_task->delay_ticks = 0;
    rtos_current_task->wait_type = RTOS_WAIT_NONE;
    rtos_current_task->wait_result = RTOS_OK;
    rtos_current_task->state = RTOS_TASK_STOPPED;
    rtos_exit_critical();

    rtos_yield();

    while (1) {
        __asm volatile ("wfi");
    }
}

void rtos_task_exit(void)
{
    rtos_task_delete_self();
}

int rtos_idle_set_hook(rtos_idle_hook_t hook, void *arg)
{
    rtos_enter_critical();
    idle_hook = hook;
    idle_hook_arg = arg;
    rtos_exit_critical();

    return RTOS_OK;
}

static int notify_common(rtos_task_t *task, uint32_t value)
{
    int should_yield = 0;

    if ((task == NULL) || (value == 0U)) {
        return RTOS_ERR_INVALID;
    }

    rtos_enter_critical();
    task->notify_value |= value;
    task->notify_pending = 1;
    rtos_trace_record(RTOS_TRACE_NOTIFY, (uint32_t)task, value);

    if ((task->state == RTOS_TASK_BLOCKED) &&
        (task->wait_type == RTOS_WAIT_NOTIFICATION) &&
        (task->wait_result == RTOS_OK)) {
        task->delay_ticks = 0;
        task->wait_result = RTOS_OK;
        task->state = RTOS_TASK_READY;
        should_yield = 1;
    }
    rtos_exit_critical();

    if (should_yield != 0) {
        rtos_yield();
    }

    return RTOS_OK;
}

int rtos_task_notify(rtos_task_t *task, uint32_t value)
{
    return notify_common(task, value);
}

int rtos_task_notify_isr(rtos_task_t *task, uint32_t value)
{
    return notify_common(task, value);
}

int rtos_task_notify_wait(uint32_t clear_on_entry,
                          uint32_t clear_on_exit,
                          uint32_t *value,
                          uint32_t timeout_ms)
{
    rtos_task_t *task = rtos_current_task;
    uint32_t notified_value;

    rtos_enter_critical();
    task->notify_value &= ~clear_on_entry;

    if (task->notify_pending != 0U) {
        notified_value = task->notify_value;
        task->notify_value &= ~clear_on_exit;
        if (task->notify_value == 0U) {
            task->notify_pending = 0;
        }
        rtos_exit_critical();
        if (value != NULL) {
            *value = notified_value;
        }
        return RTOS_OK;
    }

    if (timeout_ms == 0U) {
        rtos_exit_critical();
        return RTOS_ERR_TIMEOUT;
    }

    task->state = RTOS_TASK_BLOCKED;
    task->delay_ticks = rtos_ms_to_ticks(timeout_ms);
    task->wait_type = RTOS_WAIT_NOTIFICATION;
    task->wait_result = RTOS_OK;

    rtos_exit_critical();
    rtos_yield();

    rtos_enter_critical();
    if (task->wait_result == RTOS_ERR_TIMEOUT) {
        task->wait_type = RTOS_WAIT_NONE;
        rtos_exit_critical();
        return RTOS_ERR_TIMEOUT;
    }

    notified_value = task->notify_value;
    task->notify_value &= ~clear_on_exit;
    if (task->notify_value == 0U) {
        task->notify_pending = 0;
    }
    task->wait_type = RTOS_WAIT_NONE;
    rtos_exit_critical();

    if (value != NULL) {
        *value = notified_value;
    }

    return RTOS_OK;
}
