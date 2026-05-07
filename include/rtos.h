#ifndef RTOS_H
#define RTOS_H

#include "rtos_sem.h"
#include "rtos_task.h"

int rtos_task_create(rtos_task_entry_t entry, void *arg);
void rtos_start(void);
void rtos_yield(void);
void rtos_sleep(uint32_t ms);
void rtos_enter_critical(void);
void rtos_exit_critical(void);
void rtos_tick_handler(void);
uint32_t rtos_tick_count(void);

#endif
