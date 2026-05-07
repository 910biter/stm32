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
    rtos_task_t *task;

    if (sem == NULL) {
        return -1;
    }

    rtos_enter_critical();

    if (sem->count > 0U) {
        sem->count--;
        rtos_exit_critical();
        return 0;
    }

    task = rtos_current_task;
    task->state = RTOS_TASK_BLOCKED;
    task->delay_ticks = 0;
    task->wait_next = NULL;

    if (sem->wait_tail == NULL) {
        sem->wait_head = task;
        sem->wait_tail = task;
    } else {
        sem->wait_tail->wait_next = task;
        sem->wait_tail = task;
    }

    rtos_exit_critical();
    rtos_yield();
    return 0;
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

    task = sem->wait_head;
    if (task != NULL) {
        sem->wait_head = task->wait_next;
        if (sem->wait_head == NULL) {
            sem->wait_tail = NULL;
        }

        task->wait_next = NULL;
        task->delay_ticks = 0;
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
