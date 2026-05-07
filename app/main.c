#include "board.h"
#include "rtos.h"

volatile uint32_t app_task1_count;
volatile uint32_t app_task2_count;

static void led_task(void *arg)
{
    (void)arg;

    while (1) {
        board_led_on();
        rtos_enter_critical();
        rtos_enter_critical();
        app_task1_count++;
        rtos_exit_critical();
        rtos_exit_critical();
        rtos_sleep(250);
    }
}

static void led_off_task(void *arg)
{
    (void)arg;

    while (1) {
        board_led_off();
        rtos_enter_critical();
        rtos_enter_critical();
        app_task2_count++;
        rtos_exit_critical();
        rtos_exit_critical();
        rtos_sleep(250);
    }
}

int main(void)
{
    board_init();

    (void)rtos_task_create(led_task, 0);
    (void)rtos_task_create(led_off_task, 0);
    rtos_start();

    while (1) {
    }
}
