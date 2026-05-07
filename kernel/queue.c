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

static void wake_task(rtos_task_t *task)
{
    if (task != NULL) {
        task->delay_ticks = 0;
        task->wait_result = RTOS_OK;
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
    (void)rtos_object_register(queue, RTOS_OBJECT_QUEUE);
    return 0;
}

int rtos_queue_send(rtos_queue_t *queue, uint32_t value)
{
    return rtos_queue_send_timeout(queue, value, RTOS_TIMEOUT_FOREVER);
}

int rtos_queue_send_timeout(rtos_queue_t *queue, uint32_t value, uint32_t timeout_ms)
{
    rtos_task_t *task;
    int should_yield;

    if (queue == NULL) {
        return RTOS_ERR_INVALID;
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

        if (timeout_ms == 0U) {
            rtos_exit_critical();
            return RTOS_ERR_TIMEOUT;
        }

        task = rtos_current_task;
        task->state = RTOS_TASK_BLOCKED;
        task->delay_ticks = rtos_ms_to_ticks(timeout_ms);
        task->wait_result = RTOS_OK;
        wait_push(&queue->send_wait_head, &queue->send_wait_tail, task);

        rtos_exit_critical();
        rtos_yield();

        rtos_enter_critical();
        if (task->wait_result == RTOS_ERR_TIMEOUT) {
            wait_remove(&queue->send_wait_head, &queue->send_wait_tail, task);
            rtos_exit_critical();
            return RTOS_ERR_TIMEOUT;
        }
        rtos_exit_critical();
    }
}

int rtos_queue_recv(rtos_queue_t *queue, uint32_t *value)
{
    return rtos_queue_recv_timeout(queue, value, RTOS_TIMEOUT_FOREVER);
}

int rtos_queue_recv_timeout(rtos_queue_t *queue, uint32_t *value, uint32_t timeout_ms)
{
    rtos_task_t *task;
    int should_yield;

    if ((queue == NULL) || (value == NULL)) {
        return RTOS_ERR_INVALID;
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

        if (timeout_ms == 0U) {
            rtos_exit_critical();
            return RTOS_ERR_TIMEOUT;
        }

        task = rtos_current_task;
        task->state = RTOS_TASK_BLOCKED;
        task->delay_ticks = rtos_ms_to_ticks(timeout_ms);
        task->wait_result = RTOS_OK;
        wait_push(&queue->recv_wait_head, &queue->recv_wait_tail, task);

        rtos_exit_critical();
        rtos_yield();

        rtos_enter_critical();
        if (task->wait_result == RTOS_ERR_TIMEOUT) {
            wait_remove(&queue->recv_wait_head, &queue->recv_wait_tail, task);
            rtos_exit_critical();
            return RTOS_ERR_TIMEOUT;
        }
        rtos_exit_critical();
    }
}
