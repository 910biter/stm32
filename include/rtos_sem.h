#ifndef RTOS_SEM_H
#define RTOS_SEM_H

#include <stdint.h>

struct rtos_task;

typedef struct rtos_sem {
    uint32_t count;
    uint32_t max_count;
    struct rtos_task *wait_head;
    struct rtos_task *wait_tail;
} rtos_sem_t;

int rtos_sem_init(rtos_sem_t *sem, uint32_t initial_count, uint32_t max_count);
int rtos_sem_wait(rtos_sem_t *sem);
int rtos_sem_post(rtos_sem_t *sem);

#endif
