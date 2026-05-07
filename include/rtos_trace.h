#ifndef RTOS_TRACE_H
#define RTOS_TRACE_H

#include <stdint.h>

#include "rtos_config.h"

typedef enum rtos_trace_event {
    RTOS_TRACE_NONE = 0,
    RTOS_TRACE_SWITCH = 1,
    RTOS_TRACE_NOTIFY = 2,
    RTOS_TRACE_WORK_SUBMIT = 3,
    RTOS_TRACE_WORK_RUN = 4,
} rtos_trace_event_t;

typedef struct rtos_trace_entry {
    uint32_t tick;
    uint32_t event;
    uint32_t arg0;
    uint32_t arg1;
} rtos_trace_entry_t;

extern volatile uint32_t rtos_trace_write_index;
extern volatile uint32_t rtos_trace_count;
extern volatile uint32_t rtos_trace_total_count;
extern volatile rtos_trace_entry_t rtos_trace_entries[RTOS_TRACE_LENGTH];

void rtos_trace_record(rtos_trace_event_t event, uint32_t arg0, uint32_t arg1);
void rtos_trace_record_isr(rtos_trace_event_t event, uint32_t arg0, uint32_t arg1);

#endif
