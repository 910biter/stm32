#include "board.h"
#include "rtos.h"

volatile uint32_t app_task1_count;
volatile uint32_t app_task2_count;

static void led_task(void *arg)
{
    (void)arg;

    while (1) {
        board_led_on();
        board_delay(300000UL);
        app_task1_count++;
    }
}

static void idle_task(void *arg)
{
    (void)arg;

    while (1) {
        board_led_off();
        board_delay(300000UL);
        app_task2_count++;
    }
}

int main(void)
{
    board_init();

    (void)rtos_task_create(led_task, 0);
    (void)rtos_task_create(idle_task, 0);
    rtos_start();

    while (1) {
    }
}
