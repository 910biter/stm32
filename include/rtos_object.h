#ifndef RTOS_OBJECT_H
#define RTOS_OBJECT_H

#include <stdint.h>

#include "rtos_config.h"

typedef enum rtos_object_type {
    RTOS_OBJECT_NONE = 0,
    RTOS_OBJECT_QUEUE = 1,
    RTOS_OBJECT_SEM = 2,
    RTOS_OBJECT_MUTEX = 3,
    RTOS_OBJECT_TIMER = 4,
    RTOS_OBJECT_EVENT_FLAGS = 5,
    RTOS_OBJECT_MEMPOOL = 6,
} rtos_object_type_t;

typedef struct rtos_object_snapshot {
    rtos_object_type_t type;
    void *object;
    const char *name;
} rtos_object_snapshot_t;

extern volatile uint32_t rtos_object_snapshot_count;
extern volatile rtos_object_snapshot_t rtos_object_snapshots[RTOS_MAX_OBJECTS];

int rtos_object_register(void *object, rtos_object_type_t type);
int rtos_object_register_named(void *object, rtos_object_type_t type, const char *name);
int rtos_object_set_name(void *object, const char *name);
void rtos_object_debug_update(void);
uint32_t rtos_object_debug_snapshot(rtos_object_snapshot_t *out, uint32_t max_count);

#endif
