#include "rtos.h"

#include <stddef.h>
#include <stdint.h>

static uint32_t align_word(uint32_t value)
{
    return (value + 3U) & ~3U;
}

static void wait_push(rtos_mempool_t *pool, rtos_task_t *task)
{
    task->wait_next = NULL;

    if (pool->wait_tail == NULL) {
        pool->wait_head = task;
        pool->wait_tail = task;
    } else {
        pool->wait_tail->wait_next = task;
        pool->wait_tail = task;
    }
}

static void wait_remove(rtos_mempool_t *pool, rtos_task_t *task)
{
    rtos_task_t *prev = NULL;
    rtos_task_t *scan = pool->wait_head;

    while (scan != NULL) {
        if (scan == task) {
            if (prev == NULL) {
                pool->wait_head = scan->wait_next;
            } else {
                prev->wait_next = scan->wait_next;
            }

            if (pool->wait_tail == scan) {
                pool->wait_tail = prev;
            }

            scan->wait_next = NULL;
            return;
        }

        prev = scan;
        scan = scan->wait_next;
    }
}

static rtos_task_t *wait_pop(rtos_mempool_t *pool)
{
    rtos_task_t *task;

    while (pool->wait_head != NULL) {
        task = pool->wait_head;
        pool->wait_head = task->wait_next;
        if (pool->wait_head == NULL) {
            pool->wait_tail = NULL;
        }
        task->wait_next = NULL;

        if ((task->state == RTOS_TASK_BLOCKED) && (task->wait_result == RTOS_OK)) {
            return task;
        }
    }

    return NULL;
}

static void *pop_free_block(rtos_mempool_t *pool)
{
    void *block = pool->free_list;

    if (block != NULL) {
        pool->free_list = *(void **)block;
        pool->free_count--;
    }

    return block;
}

static void push_free_block(rtos_mempool_t *pool, void *block)
{
    *(void **)block = pool->free_list;
    pool->free_list = block;
    pool->free_count++;
}

int rtos_mempool_init(rtos_mempool_t *pool, void *storage, uint32_t block_size, uint32_t block_count)
{
    uint8_t *bytes;

    if ((pool == NULL) || (storage == NULL) || (block_count == 0U) || (block_size < sizeof(void *))) {
        return RTOS_ERR_INVALID;
    }

    pool->storage = storage;
    pool->free_list = NULL;
    pool->block_size = align_word(block_size);
    pool->block_count = block_count;
    pool->free_count = 0;
    pool->wait_head = NULL;
    pool->wait_tail = NULL;

    bytes = (uint8_t *)storage;
    for (uint32_t i = 0; i < block_count; ++i) {
        push_free_block(pool, &bytes[i * pool->block_size]);
    }

    return RTOS_OK;
}

void *rtos_mempool_alloc(rtos_mempool_t *pool)
{
    return rtos_mempool_alloc_timeout(pool, 0);
}

void *rtos_mempool_alloc_timeout(rtos_mempool_t *pool, uint32_t timeout_ms)
{
    rtos_task_t *task;
    void *block;

    if (pool == NULL) {
        return NULL;
    }

    while (1) {
        rtos_enter_critical();

        block = pop_free_block(pool);
        if (block != NULL) {
            rtos_exit_critical();
            return block;
        }

        if (timeout_ms == 0U) {
            rtos_exit_critical();
            return NULL;
        }

        task = rtos_current_task;
        task->state = RTOS_TASK_BLOCKED;
        task->delay_ticks = rtos_ms_to_ticks(timeout_ms);
        task->wait_result = RTOS_OK;
        task->wait_object_result = NULL;
        wait_push(pool, task);

        rtos_exit_critical();
        rtos_yield();

        rtos_enter_critical();
        if (task->wait_result == RTOS_ERR_TIMEOUT) {
            wait_remove(pool, task);
            rtos_exit_critical();
            return NULL;
        }
        block = task->wait_object_result;
        task->wait_object_result = NULL;
        rtos_exit_critical();

        if (block != NULL) {
            return block;
        }
    }
}

int rtos_mempool_free(rtos_mempool_t *pool, void *block)
{
    rtos_task_t *task;
    int should_yield = 0;

    if ((pool == NULL) || (block == NULL)) {
        return RTOS_ERR_INVALID;
    }

    rtos_enter_critical();

    task = wait_pop(pool);
    if (task != NULL) {
        task->wait_object_result = block;
        task->delay_ticks = 0;
        task->wait_result = RTOS_OK;
        task->state = RTOS_TASK_READY;
        should_yield = 1;
    } else {
        push_free_block(pool, block);
    }

    rtos_exit_critical();

    if (should_yield != 0) {
        rtos_yield();
    }

    return RTOS_OK;
}
