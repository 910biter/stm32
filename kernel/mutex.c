#include "rtos.h"

#include <stddef.h>

static void wait_push(rtos_task_t **head, rtos_task_t **tail, rtos_task_t *task)
{
    rtos_task_t *prev = NULL;
    rtos_task_t *scan = *head;

    task->wait_next = NULL;

    while ((scan != NULL) && (scan->priority >= task->priority)) {
        prev = scan;
        scan = scan->wait_next;
    }

    if (prev == NULL) {
        task->wait_next = *head;
        *head = task;
        if (*tail == NULL) {
            *tail = task;
        }
    } else {
        task->wait_next = scan;
        prev->wait_next = task;
        if (scan == NULL) {
            *tail = task;
        }
    }
}

static rtos_task_t *wait_pop(rtos_task_t **head, rtos_task_t **tail)
{
    rtos_task_t *task;

    while (*head != NULL) {
        task = *head;
        *head = task->wait_next;
        if (*head == NULL) {
            *tail = NULL;
        }
        task->wait_next = NULL;

        if ((task->state == RTOS_TASK_BLOCKED) && (task->wait_result == RTOS_OK)) {
            task->wait_type = RTOS_WAIT_NONE;
            return task;
        }
    }

    return NULL;
}

static void wait_remove(rtos_task_t **head, rtos_task_t **tail, rtos_task_t *task)
{
    rtos_task_t *prev = NULL;
    rtos_task_t *scan = *head;

    while (scan != NULL) {
        if (scan == task) {
            if (prev == NULL) {
                *head = scan->wait_next;
            } else {
                prev->wait_next = scan->wait_next;
            }

            if (*tail == scan) {
                *tail = prev;
            }

            scan->wait_next = NULL;
            return;
        }

        prev = scan;
        scan = scan->wait_next;
    }
}

static void recompute_owner_priority(rtos_mutex_t *mutex)
{
    uint32_t priority;
    rtos_task_t *scan;

    if (mutex->owner == NULL) {
        return;
    }

    priority = mutex->owner->base_priority;
    scan = mutex->wait_head;
    while (scan != NULL) {
        if ((scan->state == RTOS_TASK_BLOCKED) &&
            (scan->wait_result == RTOS_OK) &&
            (scan->priority > priority)) {
            priority = scan->priority;
        }
        scan = scan->wait_next;
    }

    mutex->owner->priority = priority;
}

int rtos_mutex_init(rtos_mutex_t *mutex)
{
    if (mutex == NULL) {
        return -1;
    }

    mutex->owner = NULL;
    mutex->lock_count = 0;
    mutex->wait_head = NULL;
    mutex->wait_tail = NULL;
    (void)rtos_object_register(mutex, RTOS_OBJECT_MUTEX);
    return 0;
}

int rtos_mutex_lock(rtos_mutex_t *mutex)
{
    return rtos_mutex_lock_timeout(mutex, RTOS_TIMEOUT_FOREVER);
}

int rtos_mutex_lock_timeout(rtos_mutex_t *mutex, uint32_t timeout_ms)
{
    rtos_task_t *task;

    if (mutex == NULL) {
        return RTOS_ERR_INVALID;
    }

    while (1) {
        rtos_enter_critical();

        if (mutex->owner == NULL) {
            mutex->owner = rtos_current_task;
            mutex->lock_count = 1;
            rtos_exit_critical();
            return RTOS_OK;
        }

        if (mutex->owner == rtos_current_task) {
            mutex->lock_count++;
            rtos_exit_critical();
            return RTOS_OK;
        }

        if (timeout_ms == 0U) {
            rtos_exit_critical();
            return RTOS_ERR_TIMEOUT;
        }

        if (rtos_current_task->priority > mutex->owner->priority) {
            mutex->owner->priority = rtos_current_task->priority;
        }

        task = rtos_current_task;
        task->state = RTOS_TASK_BLOCKED;
        task->delay_ticks = rtos_ms_to_ticks(timeout_ms);
        task->wait_type = RTOS_WAIT_MUTEX;
        task->wait_result = RTOS_OK;
        wait_push(&mutex->wait_head, &mutex->wait_tail, task);

        rtos_exit_critical();
        rtos_yield();

        rtos_enter_critical();
        if (task->wait_result == RTOS_ERR_TIMEOUT) {
            wait_remove(&mutex->wait_head, &mutex->wait_tail, task);
            task->wait_type = RTOS_WAIT_NONE;
            recompute_owner_priority(mutex);
            rtos_exit_critical();
            return RTOS_ERR_TIMEOUT;
        }
        rtos_exit_critical();
    }
}

int rtos_mutex_unlock(rtos_mutex_t *mutex)
{
    rtos_task_t *task;
    int should_yield = 0;

    if (mutex == NULL) {
        return -1;
    }

    rtos_enter_critical();

    if ((mutex->owner != rtos_current_task) || (mutex->lock_count == 0U)) {
        rtos_exit_critical();
        return -1;
    }

    mutex->lock_count--;
    if (mutex->lock_count != 0U) {
        rtos_exit_critical();
        return 0;
    }

    mutex->owner->priority = mutex->owner->base_priority;
    mutex->owner = NULL;

    task = wait_pop(&mutex->wait_head, &mutex->wait_tail);
    if (task != NULL) {
        task->delay_ticks = 0;
        task->wait_type = RTOS_WAIT_NONE;
        task->wait_result = RTOS_OK;
        task->state = RTOS_TASK_READY;
        should_yield = 1;
    }

    rtos_exit_critical();

    if (should_yield != 0) {
        rtos_yield();
    }

    return 0;
}
