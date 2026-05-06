#include "board.h"
#include "rtos.h"

static volatile uint32_t task1_count;
static volatile uint32_t task2_count;

static void led_task(void *arg)
{
    (void)arg;

    while (1) {
        board_led_on();
        board_delay(300000UL);
        task1_count++;
        rtos_yield();
    }
}

static void idle_task(void *arg)
{
    (void)arg;

    while (1) {
        board_led_off();
        board_delay(300000UL);
        task2_count++;
        rtos_yield();
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
