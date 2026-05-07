#ifndef RTOS_MEMPOOL_H
#define RTOS_MEMPOOL_H

#include <stdint.h>

struct rtos_task;

typedef struct rtos_mempool {
    void *storage;
    void *free_list;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t free_count;
    struct rtos_task *wait_head;
    struct rtos_task *wait_tail;
} rtos_mempool_t;

int rtos_mempool_init(rtos_mempool_t *pool, void *storage, uint32_t block_size, uint32_t block_count);
void *rtos_mempool_alloc(rtos_mempool_t *pool);
void *rtos_mempool_alloc_timeout(rtos_mempool_t *pool, uint32_t timeout_ms);
int rtos_mempool_free(rtos_mempool_t *pool, void *block);

#endif
