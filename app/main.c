#include "board.h"
#include "rtos.h"

volatile uint32_t app_task1_count;
volatile uint32_t app_task2_count;
volatile uint32_t app_priority_boost_count;

static rtos_queue_t led_queue;
static uint32_t led_queue_storage[4];
static rtos_mutex_t app_mutex;

static void producer_task(void *arg)
{
    uint32_t message = 0;

    (void)arg;

    while (1) {
        (void)rtos_mutex_lock(&app_mutex);
        (void)rtos_mutex_lock(&app_mutex);
        app_task1_count++;
        (void)rtos_queue_send(&led_queue, message++);
        if (rtos_current_priority() > rtos_current_base_priority()) {
            app_priority_boost_count++;
        }
        (void)rtos_mutex_unlock(&app_mutex);
        (void)rtos_mutex_unlock(&app_mutex);
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
        (void)rtos_mutex_lock(&app_mutex);
        (void)rtos_mutex_lock(&app_mutex);
        board_led_toggle();
        app_task2_count++;
        (void)rtos_mutex_unlock(&app_mutex);
        (void)rtos_mutex_unlock(&app_mutex);
    }
}

int main(void)
{
    board_init();

    (void)rtos_queue_init(&led_queue, led_queue_storage, 4);
    (void)rtos_mutex_init(&app_mutex);
    (void)rtos_task_create_with_priority(producer_task, 0, 1);
    (void)rtos_task_create_with_priority(consumer_task, 0, 2);
    rtos_start();

    while (1) {
    }
}
