#ifndef RTOS_H
#define RTOS_H

#include "rtos_task.h"

int rtos_task_create(rtos_task_entry_t entry, void *arg);
void rtos_start(void);
void rtos_yield(void);

#endif
