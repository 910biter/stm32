#ifndef RTOS_ASSERT_H
#define RTOS_ASSERT_H

#include <stdint.h>

struct rtos_task;

typedef struct rtos_assert_info {
    uint32_t active;
    const char *expression;
    const char *file;
    uint32_t line;
    uint32_t tick;
    struct rtos_task *task;
} rtos_assert_info_t;

extern volatile rtos_assert_info_t rtos_assert_info;

void rtos_assert_failed(const char *expression, const char *file, uint32_t line);

#define RTOS_ASSERT(expression) \
    do { \
        if (!(expression)) { \
            rtos_assert_failed(#expression, __FILE__, (uint32_t)__LINE__); \
        } \
    } while (0)

#endif
