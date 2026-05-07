#ifndef RTOS_TIMER_H
#define RTOS_TIMER_H

#include <stdint.h>

typedef void (*rtos_timer_callback_t)(void *arg);

typedef struct rtos_timer {
    uint32_t period_ticks;
    uint32_t remaining_ticks;
    uint32_t reload;
    uint32_t active;
    uint32_t linked;
    rtos_timer_callback_t callback;
    void *arg;
    struct rtos_timer *next;
} rtos_timer_t;

int rtos_timer_init(rtos_timer_t *timer,
                    uint32_t period_ms,
                    uint32_t reload,
                    rtos_timer_callback_t callback,
                    void *arg);
int rtos_timer_start(rtos_timer_t *timer);
int rtos_timer_stop(rtos_timer_t *timer);
void rtos_timer_tick(void);

#endif
