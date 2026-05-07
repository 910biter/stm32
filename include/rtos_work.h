#ifndef RTOS_WORK_H
#define RTOS_WORK_H

#include <stdint.h>

typedef void (*rtos_work_handler_t)(void *arg);

extern volatile uint32_t rtos_work_pending_items;
extern volatile uint32_t rtos_work_drop_count;

int rtos_work_init(void);
int rtos_work_submit(rtos_work_handler_t handler, void *arg);
int rtos_work_submit_isr(rtos_work_handler_t handler, void *arg);

#endif
