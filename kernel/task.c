#include <stddef.h>

#include "rtos.h"
#include "rtos_config.h"

#define INITIAL_XPSR        0x01000000UL
#define EXC_RETURN_THREAD_PSP 0xFFFFFFFDUL

static rtos_task_t tasks[RTOS_MAX_TASKS];
static uint32_t task_stacks[RTOS_MAX_TASKS][RTOS_TASK_STACK_WORDS];
static uint32_t task_count;

rtos_task_t *rtos_current_task;

static void idle_task(void *arg)
{
    (void)arg;

    while (1) {
        __asm volatile ("wfi");
    }
}

static uint32_t *align_stack(uint32_t *sp)
{
    return (uint32_t *)((uintptr_t)sp & ~((uintptr_t)0x7U));
}

int rtos_task_create(rtos_task_entry_t entry, void *arg)
{
    if ((entry == NULL) || (task_count >= RTOS_MAX_TASKS)) {
        return -1;
    }

    rtos_task_t *task = &tasks[task_count];
    uint32_t *stack = task_stacks[task_count];
    uint32_t *sp = align_stack(stack + RTOS_TASK_STACK_WORDS);

    *--sp = INITIAL_XPSR;
    *--sp = ((uint32_t)entry) | 1UL;
    *--sp = (uint32_t)rtos_task_exit;
    *--sp = 0x12121212UL;
    *--sp = 0x03030303UL;
    *--sp = 0x02020202UL;
    *--sp = 0x01010101UL;
    *--sp = (uint32_t)arg;

    *--sp = 0x11111111UL;
    *--sp = 0x10101010UL;
    *--sp = 0x09090909UL;
    *--sp = 0x08080808UL;
    *--sp = 0x07070707UL;
    *--sp = 0x06060606UL;
    *--sp = 0x05050505UL;
    *--sp = 0x04040404UL;

    task->sp = sp;
    task->stack_base = stack;
    task->stack_words = RTOS_TASK_STACK_WORDS;
    task->delay_ticks = 0;
    task->state = RTOS_TASK_READY;

    if (task_count == 0U) {
        task->next = task;
        rtos_current_task = task;
    } else {
        task->next = rtos_current_task->next;
        rtos_current_task->next = task;
    }

    task_count++;
    return 0;
}

int rtos_create_idle_task(void)
{
    return rtos_task_create(idle_task, NULL);
}

void rtos_task_tick(void)
{
    for (uint32_t i = 0; i < task_count; ++i) {
        rtos_task_t *task = &tasks[i];

        if ((task->state == RTOS_TASK_BLOCKED) && (task->delay_ticks > 0U)) {
            task->delay_ticks--;

            if (task->delay_ticks == 0U) {
                task->state = RTOS_TASK_READY;
            }
        }
    }
}

void rtos_task_exit(void)
{
    while (1) {
        rtos_yield();
    }
}
