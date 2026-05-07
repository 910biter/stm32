#include "rtos.h"

#include <stddef.h>

static void wait_push(rtos_event_flags_t *events, rtos_task_t *task)
{
    rtos_task_t *prev = NULL;
    rtos_task_t *scan = events->wait_head;

    task->wait_next = NULL;

    while ((scan != NULL) && (scan->priority >= task->priority)) {
        prev = scan;
        scan = scan->wait_next;
    }

    if (prev == NULL) {
        task->wait_next = events->wait_head;
        events->wait_head = task;
        if (events->wait_tail == NULL) {
            events->wait_tail = task;
        }
    } else {
        task->wait_next = scan;
        prev->wait_next = task;
        if (scan == NULL) {
            events->wait_tail = task;
        }
    }
}

static void wait_remove(rtos_event_flags_t *events, rtos_task_t *task)
{
    rtos_task_t *prev = NULL;
    rtos_task_t *scan = events->wait_head;

    while (scan != NULL) {
        if (scan == task) {
            if (prev == NULL) {
                events->wait_head = scan->wait_next;
            } else {
                prev->wait_next = scan->wait_next;
            }

            if (events->wait_tail == scan) {
                events->wait_tail = prev;
            }

            scan->wait_next = NULL;
            return;
        }

        prev = scan;
        scan = scan->wait_next;
    }
}

static int flags_match(uint32_t current, uint32_t flags, uint32_t wait_all, uint32_t *matched)
{
    uint32_t selected = current & flags;

    if (wait_all != 0U) {
        if (selected == flags) {
            *matched = selected;
            return 1;
        }
    } else if (selected != 0U) {
        *matched = selected;
        return 1;
    }

    *matched = 0;
    return 0;
}

int rtos_event_flags_init(rtos_event_flags_t *events)
{
    if (events == NULL) {
        return RTOS_ERR_INVALID;
    }

    events->flags = 0;
    events->wait_head = NULL;
    events->wait_tail = NULL;
    (void)rtos_object_register(events, RTOS_OBJECT_EVENT_FLAGS);
    return RTOS_OK;
}

int rtos_event_flags_set(rtos_event_flags_t *events, uint32_t flags)
{
    rtos_task_t *prev;
    rtos_task_t *scan;
    int should_yield = 0;

    if ((events == NULL) || (flags == 0U)) {
        return RTOS_ERR_INVALID;
    }

    rtos_enter_critical();

    events->flags |= flags;
    prev = NULL;
    scan = events->wait_head;
    while (scan != NULL) {
        rtos_task_t *next = scan->wait_next;
        uint32_t matched;

        if ((scan->state == RTOS_TASK_BLOCKED) &&
            (scan->wait_result == RTOS_OK) &&
            (flags_match(events->flags, scan->wait_flags, scan->wait_flags_all, &matched) != 0)) {
            if (prev == NULL) {
                events->wait_head = next;
            } else {
                prev->wait_next = next;
            }

            if (events->wait_tail == scan) {
                events->wait_tail = prev;
            }

            scan->wait_next = NULL;
            scan->wait_flags_result = matched;
            scan->delay_ticks = 0;
            scan->wait_result = RTOS_OK;
            scan->state = RTOS_TASK_READY;
            should_yield = 1;

            if (scan->wait_flags_clear != 0U) {
                events->flags &= ~matched;
            }
        } else {
            prev = scan;
        }

        scan = next;
    }

    rtos_exit_critical();

    if (should_yield != 0) {
        rtos_yield();
    }

    return RTOS_OK;
}

int rtos_event_flags_clear(rtos_event_flags_t *events, uint32_t flags)
{
    if (events == NULL) {
        return RTOS_ERR_INVALID;
    }

    rtos_enter_critical();
    events->flags &= ~flags;
    rtos_exit_critical();

    return RTOS_OK;
}

int rtos_event_flags_wait(rtos_event_flags_t *events,
                          uint32_t flags,
                          uint32_t *matched_flags,
                          uint32_t wait_all,
                          uint32_t clear_on_exit,
                          uint32_t timeout_ms)
{
    rtos_task_t *task;
    uint32_t matched;

    if ((events == NULL) || (flags == 0U)) {
        return RTOS_ERR_INVALID;
    }

    rtos_enter_critical();

    if (flags_match(events->flags, flags, wait_all, &matched) != 0) {
        if (clear_on_exit != 0U) {
            events->flags &= ~matched;
        }
        rtos_exit_critical();
        if (matched_flags != NULL) {
            *matched_flags = matched;
        }
        return RTOS_OK;
    }

    if (timeout_ms == 0U) {
        rtos_exit_critical();
        return RTOS_ERR_TIMEOUT;
    }

    task = rtos_current_task;
    task->state = RTOS_TASK_BLOCKED;
    task->delay_ticks = rtos_ms_to_ticks(timeout_ms);
    task->wait_result = RTOS_OK;
    task->wait_flags = flags;
    task->wait_flags_result = 0;
    task->wait_flags_all = wait_all;
    task->wait_flags_clear = clear_on_exit;
    wait_push(events, task);

    rtos_exit_critical();
    rtos_yield();

    rtos_enter_critical();
    if (task->wait_result == RTOS_ERR_TIMEOUT) {
        wait_remove(events, task);
        rtos_exit_critical();
        return RTOS_ERR_TIMEOUT;
    }
    matched = task->wait_flags_result;
    rtos_exit_critical();

    if (matched_flags != NULL) {
        *matched_flags = matched;
    }

    return RTOS_OK;
}
