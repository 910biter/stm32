#include "rtos.h"

#include <stddef.h>

static void wait_push(rtos_task_t **head, rtos_task_t **tail, rtos_task_t *task)
{
    task->wait_next = NULL;

    if (*tail == NULL) {
        *head = task;
        *tail = task;
    } else {
        (*tail)->wait_next = task;
        *tail = task;
    }
}

static rtos_task_t *wait_pop(rtos_task_t **head, rtos_task_t **tail)
{
    rtos_task_t *task = *head;

    if (task != NULL) {
        *head = task->wait_next;
        if (*head == NULL) {
            *tail = NULL;
        }
        task->wait_next = NULL;
    }

    return task;
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
    return 0;
}

int rtos_mutex_lock(rtos_mutex_t *mutex)
{
    rtos_task_t *task;

    if (mutex == NULL) {
        return -1;
    }

    while (1) {
        rtos_enter_critical();

        if (mutex->owner == NULL) {
            mutex->owner = rtos_current_task;
            mutex->lock_count = 1;
            rtos_exit_critical();
            return 0;
        }

        if (mutex->owner == rtos_current_task) {
            mutex->lock_count++;
            rtos_exit_critical();
            return 0;
        }

        task = rtos_current_task;
        task->state = RTOS_TASK_BLOCKED;
        task->delay_ticks = 0;
        wait_push(&mutex->wait_head, &mutex->wait_tail, task);

        rtos_exit_critical();
        rtos_yield();
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

    mutex->owner = NULL;

    task = wait_pop(&mutex->wait_head, &mutex->wait_tail);
    if (task != NULL) {
        task->delay_ticks = 0;
        task->state = RTOS_TASK_READY;
        should_yield = 1;
    }

    rtos_exit_critical();

    if (should_yield != 0) {
        rtos_yield();
    }

    return 0;
}
