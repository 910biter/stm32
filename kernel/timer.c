#include "rtos.h"

#include <stddef.h>

static rtos_timer_t *timer_list;

static void link_timer(rtos_timer_t *timer)
{
    if (timer->linked != 0U) {
        return;
    }

    timer->next = timer_list;
    timer_list = timer;
    timer->linked = 1;
}

int rtos_timer_init(rtos_timer_t *timer,
                    uint32_t period_ms,
                    uint32_t reload,
                    rtos_timer_callback_t callback,
                    void *arg)
{
    uint32_t period_ticks;

    if ((timer == NULL) || (period_ms == 0U) || (callback == NULL)) {
        return RTOS_ERR_INVALID;
    }

    period_ticks = rtos_ms_to_ticks(period_ms);
    if (period_ticks == 0U) {
        return RTOS_ERR_INVALID;
    }

    rtos_enter_critical();
    timer->period_ticks = period_ticks;
    timer->remaining_ticks = period_ticks;
    timer->reload = reload;
    timer->active = 0;
    timer->callback = callback;
    timer->arg = arg;
    link_timer(timer);
    rtos_exit_critical();

    return RTOS_OK;
}

int rtos_timer_start(rtos_timer_t *timer)
{
    if ((timer == NULL) || (timer->callback == NULL)) {
        return RTOS_ERR_INVALID;
    }

    rtos_enter_critical();
    link_timer(timer);
    timer->remaining_ticks = timer->period_ticks;
    timer->active = 1;
    rtos_exit_critical();

    return RTOS_OK;
}

int rtos_timer_stop(rtos_timer_t *timer)
{
    if (timer == NULL) {
        return RTOS_ERR_INVALID;
    }

    rtos_enter_critical();
    timer->active = 0;
    rtos_exit_critical();

    return RTOS_OK;
}

void rtos_timer_tick(void)
{
    rtos_timer_t *timer = timer_list;

    while (timer != NULL) {
        if ((timer->active != 0U) && (timer->remaining_ticks > 0U)) {
            timer->remaining_ticks--;

            if (timer->remaining_ticks == 0U) {
                timer->callback(timer->arg);

                if (timer->reload != 0U) {
                    timer->remaining_ticks = timer->period_ticks;
                } else {
                    timer->active = 0;
                }
            }
        }

        timer = timer->next;
    }
}
