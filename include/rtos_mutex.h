#ifndef RTOS_MUTEX_H
#define RTOS_MUTEX_H

#include <stdint.h>

struct rtos_task;

typedef struct rtos_mutex {
    struct rtos_task *owner;
    uint32_t lock_count;
    struct rtos_task *wait_head;
    struct rtos_task *wait_tail;
} rtos_mutex_t;

int rtos_mutex_init(rtos_mutex_t *mutex);
int rtos_mutex_lock(rtos_mutex_t *mutex);
int rtos_mutex_lock_timeout(rtos_mutex_t *mutex, uint32_t timeout_ms);
int rtos_mutex_unlock(rtos_mutex_t *mutex);

#endif
