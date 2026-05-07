#include "board.h"
#include "rtos.h"

volatile uint32_t app_task1_count;
volatile uint32_t app_task2_count;
volatile uint32_t app_priority_boost_count;
volatile uint32_t app_timeout_count;
volatile uint32_t app_timer_count;
volatile uint32_t app_event_count;
volatile uint32_t app_pool_count;
volatile uint32_t app_exit_count;
volatile uint32_t app_reuse_count;
volatile uint32_t app_sched_lock_count;
volatile uint32_t app_isr_sem_count;
volatile uint32_t app_idle_hook_count;
volatile uint32_t app_notify_count;
volatile uint32_t app_deferred_count;

static rtos_queue_t led_queue;
static uint32_t led_queue_storage[4];
static rtos_mutex_t app_mutex;
static rtos_sem_t timeout_sem;
static rtos_sem_t isr_sem;
static rtos_timer_t demo_timer;
static rtos_event_flags_t demo_events;
static rtos_mempool_t demo_pool;
static uint32_t demo_pool_storage[4][4];
static rtos_task_t *timeout_task_handle;

static void one_shot_task(void *arg);

static void idle_hook(void *arg)
{
    (void)arg;

    app_idle_hook_count++;
}

static void deferred_callback(void *arg)
{
    (void)arg;

    app_deferred_count++;
}

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

static void timeout_task(void *arg)
{
    static uint32_t reuse_created;
    uint32_t notify_value;

    (void)arg;

    while (1) {
        if (timeout_task_handle == 0) {
            timeout_task_handle = rtos_task_self();
        }
        if (rtos_sem_wait_timeout(&timeout_sem, 100) == RTOS_ERR_TIMEOUT) {
            app_timeout_count++;
        }
        if (rtos_event_flags_wait(&demo_events, 0x1U, 0, 0, 1, 1000) == RTOS_OK) {
            app_event_count++;
        }
        void *block = rtos_mempool_alloc_timeout(&demo_pool, 10);
        if (block != 0) {
            app_pool_count++;
            (void)rtos_mempool_free(&demo_pool, block);
        }
        if (rtos_sem_wait_timeout(&isr_sem, 0) == RTOS_OK) {
            app_isr_sem_count++;
        }
        if (rtos_task_notify_wait(0, 0xFFFFFFFFU, &notify_value, 1000) == RTOS_OK) {
            if ((notify_value & 0x1U) != 0U) {
                app_notify_count++;
            }
        }
        if ((app_exit_count != 0U) && (reuse_created == 0U)) {
            if (rtos_task_create_named(one_shot_task, 0, 1, "reuse") == RTOS_OK) {
                reuse_created = 1;
            }
        }
        rtos_sched_lock();
        rtos_yield();
        app_sched_lock_count++;
        rtos_sched_unlock();
        rtos_sleep(50);
    }
}

static void timer_callback(void *arg)
{
    (void)arg;

    app_timer_count++;
    if (rtos_in_isr() != 0U) {
        (void)rtos_event_flags_set_isr(&demo_events, 0x1U);
        (void)rtos_sem_post_isr(&isr_sem);
        (void)rtos_task_notify_isr(timeout_task_handle, 0x1U);
        (void)rtos_work_submit_isr(deferred_callback, 0);
    }
}

static void one_shot_task(void *arg)
{
    (void)arg;

    if (app_exit_count == 0U) {
        app_exit_count++;
    } else {
        app_reuse_count++;
    }
}

int main(void)
{
    board_init();

    (void)rtos_queue_init(&led_queue, led_queue_storage, 4);
    (void)rtos_mutex_init(&app_mutex);
    (void)rtos_sem_init(&timeout_sem, 0, 1);
    (void)rtos_sem_init(&isr_sem, 0, 1);
    (void)rtos_event_flags_init(&demo_events);
    (void)rtos_mempool_init(&demo_pool, demo_pool_storage, sizeof(demo_pool_storage[0]), 4);
    (void)rtos_timer_init(&demo_timer, 500, 1, timer_callback, 0);
    (void)rtos_object_set_name(&led_queue, "led_queue");
    (void)rtos_object_set_name(&app_mutex, "app_mutex");
    (void)rtos_object_set_name(&timeout_sem, "timeout_sem");
    (void)rtos_object_set_name(&isr_sem, "isr_sem");
    (void)rtos_object_set_name(&demo_events, "demo_events");
    (void)rtos_object_set_name(&demo_pool, "demo_pool");
    (void)rtos_object_set_name(&demo_timer, "demo_timer");
    (void)rtos_idle_set_hook(idle_hook, 0);
    (void)rtos_work_init();
    (void)rtos_timer_start(&demo_timer);
    (void)rtos_task_create_named(producer_task, 0, 1, "producer");
    (void)rtos_task_create_named(consumer_task, 0, 2, "consumer");
    (void)rtos_task_create_named(timeout_task, 0, 1, "timeout");
    (void)rtos_task_create_named(one_shot_task, 0, 1, "one-shot");
    rtos_start();

    while (1) {
    }
}
