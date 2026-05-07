#include "rtos.h"

#include <stddef.h>

typedef struct rtos_object_record {
    rtos_object_type_t type;
    void *object;
    const char *name;
} rtos_object_record_t;

static rtos_object_record_t object_records[RTOS_MAX_OBJECTS];

volatile uint32_t rtos_object_snapshot_count;
volatile rtos_object_snapshot_t rtos_object_snapshots[RTOS_MAX_OBJECTS];

static void update_snapshots_locked(void)
{
    uint32_t count = 0;

    for (uint32_t i = 0; i < RTOS_MAX_OBJECTS; ++i) {
        if (object_records[i].type == RTOS_OBJECT_NONE) {
            continue;
        }

        rtos_object_snapshots[count].type = object_records[i].type;
        rtos_object_snapshots[count].object = object_records[i].object;
        rtos_object_snapshots[count].name = object_records[i].name;
        count++;
    }

    rtos_object_snapshot_count = count;
}

int rtos_object_register(void *object, rtos_object_type_t type)
{
    return rtos_object_register_named(object, type, NULL);
}

int rtos_object_register_named(void *object, rtos_object_type_t type, const char *name)
{
    int result = RTOS_OK;
    int free_slot = -1;

    if ((object == NULL) || (type == RTOS_OBJECT_NONE)) {
        return RTOS_ERR_INVALID;
    }

    rtos_enter_critical();

    for (uint32_t i = 0; i < RTOS_MAX_OBJECTS; ++i) {
        if (object_records[i].object == object) {
            object_records[i].type = type;
            if (name != NULL) {
                object_records[i].name = name;
            }
            update_snapshots_locked();
            rtos_exit_critical();
            return RTOS_OK;
        }

        if ((free_slot < 0) && (object_records[i].type == RTOS_OBJECT_NONE)) {
            free_slot = (int)i;
        }
    }

    if (free_slot >= 0) {
        object_records[free_slot].type = type;
        object_records[free_slot].object = object;
        object_records[free_slot].name = name;
        update_snapshots_locked();
    } else {
        result = RTOS_ERR_FULL;
    }

    rtos_exit_critical();

    return result;
}

int rtos_object_set_name(void *object, const char *name)
{
    if ((object == NULL) || (name == NULL)) {
        return RTOS_ERR_INVALID;
    }

    rtos_enter_critical();

    for (uint32_t i = 0; i < RTOS_MAX_OBJECTS; ++i) {
        if (object_records[i].object == object) {
            object_records[i].name = name;
            update_snapshots_locked();
            rtos_exit_critical();
            return RTOS_OK;
        }
    }

    rtos_exit_critical();

    return RTOS_ERR_INVALID;
}

void rtos_object_debug_update(void)
{
    rtos_enter_critical();
    update_snapshots_locked();
    rtos_exit_critical();
}

uint32_t rtos_object_debug_snapshot(rtos_object_snapshot_t *out, uint32_t max_count)
{
    uint32_t count;

    if ((out == NULL) || (max_count == 0U)) {
        return 0;
    }

    rtos_enter_critical();
    update_snapshots_locked();
    count = rtos_object_snapshot_count;
    if (count > max_count) {
        count = max_count;
    }

    for (uint32_t i = 0; i < count; ++i) {
        out[i].type = rtos_object_snapshots[i].type;
        out[i].object = rtos_object_snapshots[i].object;
        out[i].name = rtos_object_snapshots[i].name;
    }
    rtos_exit_critical();

    return count;
}
