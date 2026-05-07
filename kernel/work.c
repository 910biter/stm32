#include "rtos.h"

#include <stddef.h>

typedef struct rtos_work_item {
    rtos_work_handler_t handler;
    void *arg;
} rtos_work_item_t;

static rtos_work_item_t work_queue[RTOS_WORK_QUEUE_LENGTH];
static uint32_t work_head;
static uint32_t work_tail;
static rtos_task_t *work_task_handle;

volatile uint32_t rtos_work_pending_items;
volatile uint32_t rtos_work_drop_count;

static int pop_work(rtos_work_item_t *item)
{
    if (rtos_work_pending_items == 0U) {
        return 0;
    }

    *item = work_queue[work_head];
    work_head = (work_head + 1U) % RTOS_WORK_QUEUE_LENGTH;
    rtos_work_pending_items--;
    return 1;
}

static void work_task(void *arg)
{
    rtos_work_item_t item;

    (void)arg;

    while (1) {
        (void)rtos_task_notify_wait(0, 0xFFFFFFFFU, NULL, RTOS_TIMEOUT_FOREVER);

        while (1) {
            rtos_enter_critical();
            if (pop_work(&item) == 0) {
                rtos_exit_critical();
                break;
            }
            rtos_exit_critical();

            item.handler(item.arg);
        }
    }
}

int rtos_work_init(void)
{
    return rtos_task_create_named_handle(work_task, NULL, RTOS_DEFAULT_PRIORITY, "work", &work_task_handle);
}

static int submit_common(rtos_work_handler_t handler, void *arg)
{
    if (handler == NULL) {
        return RTOS_ERR_INVALID;
    }

    rtos_enter_critical();
    if (rtos_work_pending_items >= RTOS_WORK_QUEUE_LENGTH) {
        rtos_work_drop_count++;
        rtos_exit_critical();
        return RTOS_ERR_FULL;
    }

    work_queue[work_tail].handler = handler;
    work_queue[work_tail].arg = arg;
    work_tail = (work_tail + 1U) % RTOS_WORK_QUEUE_LENGTH;
    rtos_work_pending_items++;
    rtos_exit_critical();

    if (work_task_handle != NULL) {
        (void)rtos_task_notify(work_task_handle, 0x1U);
    }

    return RTOS_OK;
}

int rtos_work_submit(rtos_work_handler_t handler, void *arg)
{
    return submit_common(handler, arg);
}

int rtos_work_submit_isr(rtos_work_handler_t handler, void *arg)
{
    return submit_common(handler, arg);
}
