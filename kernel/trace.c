#include "rtos.h"

volatile uint32_t rtos_trace_write_index;
volatile uint32_t rtos_trace_count;
volatile uint32_t rtos_trace_total_count;
volatile rtos_trace_entry_t rtos_trace_entries[RTOS_TRACE_LENGTH];

static void record_common(rtos_trace_event_t event, uint32_t arg0, uint32_t arg1)
{
    uint32_t index;

    if (event == RTOS_TRACE_NONE) {
        return;
    }

    rtos_enter_critical();
    index = rtos_trace_write_index;
    rtos_trace_entries[index].tick = rtos_tick_count();
    rtos_trace_entries[index].event = (uint32_t)event;
    rtos_trace_entries[index].arg0 = arg0;
    rtos_trace_entries[index].arg1 = arg1;

    index = (index + 1U) % RTOS_TRACE_LENGTH;
    rtos_trace_write_index = index;
    if (rtos_trace_count < RTOS_TRACE_LENGTH) {
        rtos_trace_count++;
    }
    rtos_trace_total_count++;
    rtos_exit_critical();
}

void rtos_trace_record(rtos_trace_event_t event, uint32_t arg0, uint32_t arg1)
{
    record_common(event, arg0, arg1);
}

void rtos_trace_record_isr(rtos_trace_event_t event, uint32_t arg0, uint32_t arg1)
{
    record_common(event, arg0, arg1);
}
