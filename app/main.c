#include "board.h"
#include "rtos.h"

volatile uint32_t app_task1_count;
volatile uint32_t app_task2_count;

static rtos_sem_t led_sem;

static void producer_task(void *arg)
{
    (void)arg;

    while (1) {
        rtos_enter_critical();
        app_task1_count++;
        rtos_exit_critical();
        (void)rtos_sem_post(&led_sem);
        rtos_sleep(250);
    }
}

static void consumer_task(void *arg)
{
    (void)arg;

    while (1) {
        (void)rtos_sem_wait(&led_sem);
        board_led_toggle();
        rtos_enter_critical();
        app_task2_count++;
        rtos_exit_critical();
    }
}

int main(void)
{
    board_init();

    (void)rtos_sem_init(&led_sem, 0, 1);
    (void)rtos_task_create(producer_task, 0);
    (void)rtos_task_create(consumer_task, 0);
    rtos_start();

    while (1) {
    }
}
