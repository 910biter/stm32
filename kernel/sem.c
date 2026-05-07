#include "rtos.h"

#include <stddef.h>

int rtos_sem_init(rtos_sem_t *sem, uint32_t initial_count, uint32_t max_count)
{
    if ((sem == NULL) || (max_count == 0U) || (initial_count > max_count)) {
        return -1;
    }

    sem->count = initial_count;
    sem->max_count = max_count;
    sem->wait_head = NULL;
    sem->wait_tail = NULL;
    return 0;
}

int rtos_sem_wait(rtos_sem_t *sem)
{
    return rtos_sem_wait_timeout(sem, RTOS_TIMEOUT_FOREVER);
}

static void wait_push(rtos_sem_t *sem, rtos_task_t *task)
{
    rtos_task_t *prev = NULL;
    rtos_task_t *scan = sem->wait_head;

    task->wait_next = NULL;

    while ((scan != NULL) && (scan->priority >= task->priority)) {
        prev = scan;
        scan = scan->wait_next;
    }

    if (prev == NULL) {
        task->wait_next = sem->wait_head;
        sem->wait_head = task;
        if (sem->wait_tail == NULL) {
            sem->wait_tail = task;
        }
    } else {
        task->wait_next = scan;
        prev->wait_next = task;
        if (scan == NULL) {
            sem->wait_tail = task;
        }
    }
}


static void wait_remove(rtos_sem_t *sem, rtos_task_t *task)
{
    rtos_task_t *prev = NULL;
    rtos_task_t *scan = sem->wait_head;

    while (scan != NULL) {
        if (scan == task) {
            if (prev == NULL) {
                sem->wait_head = scan->wait_next;
            } else {
                prev->wait_next = scan->wait_next;
            }

            if (sem->wait_tail == scan) {
                sem->wait_tail = prev;
            }

            scan->wait_next = NULL;
            return;
        }

        prev = scan;
        scan = scan->wait_next;
    }
}

static rtos_task_t *wait_pop(rtos_sem_t *sem)
{
    rtos_task_t *task;

    while (sem->wait_head != NULL) {
        task = sem->wait_head;
        sem->wait_head = task->wait_next;
        if (sem->wait_head == NULL) {
            sem->wait_tail = NULL;
        }
        task->wait_next = NULL;

        if ((task->state == RTOS_TASK_BLOCKED) && (task->wait_result == RTOS_OK)) {
            return task;
        }
    }

    return NULL;
}

int rtos_sem_wait_timeout(rtos_sem_t *sem, uint32_t timeout_ms)
{
    rtos_task_t *task;

    if (sem == NULL) {
        return RTOS_ERR_INVALID;
    }

    rtos_enter_critical();

    if (sem->count > 0U) {
        sem->count--;
        rtos_exit_critical();
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
    wait_push(sem, task);

    rtos_exit_critical();
    rtos_yield();

    rtos_enter_critical();
    if (task->wait_result == RTOS_ERR_TIMEOUT) {
        wait_remove(sem, task);
        rtos_exit_critical();
        return RTOS_ERR_TIMEOUT;
    }
    rtos_exit_critical();

    return RTOS_OK;
}

int rtos_sem_post(rtos_sem_t *sem)
{
    rtos_task_t *task;
    int should_yield = 0;
    int result = 0;

    if (sem == NULL) {
        return -1;
    }

    rtos_enter_critical();

    task = wait_pop(sem);
    if (task != NULL) {
        task->delay_ticks = 0;
        task->wait_result = RTOS_OK;
        task->state = RTOS_TASK_READY;
        should_yield = 1;
    } else if (sem->count < sem->max_count) {
        sem->count++;
    } else {
        result = -1;
    }

    rtos_exit_critical();

    if (should_yield != 0) {
        rtos_yield();
    }

    return result;
}
