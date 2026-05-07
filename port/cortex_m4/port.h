#ifndef PORT_H
#define PORT_H

#include <stdint.h>

void port_start_first_task(void);
void port_init_scheduler(void);
void port_setup_systick(uint32_t cpu_hz, uint32_t tick_hz);
void port_trigger_pendsv(void);
void port_enter_critical(void);
void port_exit_critical(void);
uint32_t port_in_isr(void);

#endif
