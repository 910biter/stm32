#include "board.h"
#include "rtos.h"

volatile uint32_t app_task1_count;
volatile uint32_t app_task2_count;

static rtos_queue_t led_queue;
static uint32_t led_queue_storage[4];

static void producer_task(void *arg)
{
    uint32_t message = 0;

    (void)arg;

    while (1) {
        rtos_enter_critical();
        app_task1_count++;
        rtos_exit_critical();
        (void)rtos_queue_send(&led_queue, message++);
        rtos_sleep(250);
    }
}

static void consumer_task(void *arg)
{
    (void)arg;

    while (1) {
        uint32_t message;

        (void)rtos_queue_recv(&led_queue, &message);
        (void)message;
        board_led_toggle();
        rtos_enter_critical();
        app_task2_count++;
        rtos_exit_critical();
    }
}

int main(void)
{
    board_init();

    (void)rtos_queue_init(&led_queue, led_queue_storage, 4);
    (void)rtos_task_create(producer_task, 0);
    (void)rtos_task_create(consumer_task, 0);
    rtos_start();

    while (1) {
    }
}
