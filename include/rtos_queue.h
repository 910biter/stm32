#ifndef RTOS_QUEUE_H
#define RTOS_QUEUE_H

#include <stdint.h>

struct rtos_task;

typedef struct rtos_queue {
    uint32_t *buffer;
    uint32_t capacity;
    uint32_t count;
    uint32_t head;
    uint32_t tail;
    struct rtos_task *recv_wait_head;
    struct rtos_task *recv_wait_tail;
    struct rtos_task *send_wait_head;
    struct rtos_task *send_wait_tail;
} rtos_queue_t;

int rtos_queue_init(rtos_queue_t *queue, uint32_t *buffer, uint32_t capacity);
int rtos_queue_send(rtos_queue_t *queue, uint32_t value);
int rtos_queue_recv(rtos_queue_t *queue, uint32_t *value);

#endif
