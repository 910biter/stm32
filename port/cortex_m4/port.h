#ifndef PORT_H
#define PORT_H

void port_start_first_task(void);
void port_init_scheduler(void);
void port_trigger_pendsv(void);
void port_enter_critical(void);
void port_exit_critical(void);

#endif
