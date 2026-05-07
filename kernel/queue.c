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

static void wake_task(rtos_task_t *task)
{
    if (task != NULL) {
        task->delay_ticks = 0;
        task->state = RTOS_TASK_READY;
    }
}

int rtos_queue_init(rtos_queue_t *queue, uint32_t *buffer, uint32_t capacity)
{
    if ((queue == NULL) || (buffer == NULL) || (capacity == 0U)) {
        return -1;
    }

    queue->buffer = buffer;
    queue->capacity = capacity;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->recv_wait_head = NULL;
    queue->recv_wait_tail = NULL;
    queue->send_wait_head = NULL;
    queue->send_wait_tail = NULL;
    return 0;
}

int rtos_queue_send(rtos_queue_t *queue, uint32_t value)
{
    rtos_task_t *task;
    int should_yield;

    if (queue == NULL) {
        return -1;
    }

    while (1) {
        should_yield = 0;

        rtos_enter_critical();

        if (queue->count < queue->capacity) {
            queue->buffer[queue->tail] = value;
            queue->tail = (queue->tail + 1U) % queue->capacity;
            queue->count++;

            task = wait_pop(&queue->recv_wait_head, &queue->recv_wait_tail);
            if (task != NULL) {
                wake_task(task);
                should_yield = 1;
            }

            rtos_exit_critical();

            if (should_yield != 0) {
                rtos_yield();
            }

            return 0;
        }

        task = rtos_current_task;
        task->state = RTOS_TASK_BLOCKED;
        task->delay_ticks = 0;
        wait_push(&queue->send_wait_head, &queue->send_wait_tail, task);

        rtos_exit_critical();
        rtos_yield();
    }
}

int rtos_queue_recv(rtos_queue_t *queue, uint32_t *value)
{
    rtos_task_t *task;
    int should_yield;

    if ((queue == NULL) || (value == NULL)) {
        return -1;
    }

    while (1) {
        should_yield = 0;

        rtos_enter_critical();

        if (queue->count > 0U) {
            *value = queue->buffer[queue->head];
            queue->head = (queue->head + 1U) % queue->capacity;
            queue->count--;

            task = wait_pop(&queue->send_wait_head, &queue->send_wait_tail);
            if (task != NULL) {
                wake_task(task);
                should_yield = 1;
            }

            rtos_exit_critical();

            if (should_yield != 0) {
                rtos_yield();
            }

            return 0;
        }

        task = rtos_current_task;
        task->state = RTOS_TASK_BLOCKED;
        task->delay_ticks = 0;
        wait_push(&queue->recv_wait_head, &queue->recv_wait_tail, task);

        rtos_exit_critical();
        rtos_yield();
    }
}
