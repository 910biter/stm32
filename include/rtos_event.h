#ifndef RTOS_EVENT_H
#define RTOS_EVENT_H

#include <stdint.h>

struct rtos_task;

typedef struct rtos_event_flags {
    uint32_t flags;
    struct rtos_task *wait_head;
    struct rtos_task *wait_tail;
} rtos_event_flags_t;

int rtos_event_flags_init(rtos_event_flags_t *events);
int rtos_event_flags_set(rtos_event_flags_t *events, uint32_t flags);
int rtos_event_flags_clear(rtos_event_flags_t *events, uint32_t flags);
int rtos_event_flags_wait(rtos_event_flags_t *events,
                          uint32_t flags,
                          uint32_t *matched_flags,
                          uint32_t wait_all,
                          uint32_t clear_on_exit,
                          uint32_t timeout_ms);

#endif
