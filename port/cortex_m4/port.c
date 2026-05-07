#include "port.h"

#include "rtos.h"

#define SCB_ICSR        (*(volatile uint32_t *)0xE000ED04UL)
#define SCB_SHPR3       (*(volatile uint32_t *)0xE000ED20UL)
#define SYST_CSR        (*(volatile uint32_t *)0xE000E010UL)
#define SYST_RVR        (*(volatile uint32_t *)0xE000E014UL)
#define SYST_CVR        (*(volatile uint32_t *)0xE000E018UL)

#define SCB_ICSR_PENDSVSET  (1UL << 28)
#define SYST_CSR_ENABLE     (1UL << 0)
#define SYST_CSR_TICKINT    (1UL << 1)
#define SYST_CSR_CLKSOURCE  (1UL << 2)

static uint32_t critical_nesting;
static uint32_t saved_primask;

static uint32_t read_primask(void)
{
    uint32_t primask;

    __asm volatile ("mrs %0, primask" : "=r" (primask) :: "memory");
    return primask;
}

static uint32_t read_ipsr(void)
{
    uint32_t ipsr;

    __asm volatile ("mrs %0, ipsr" : "=r" (ipsr) :: "memory");
    return ipsr;
}

static void write_primask(uint32_t primask)
{
    __asm volatile ("msr primask, %0" :: "r" (primask) : "memory");
}

void port_init_scheduler(void)
{
    SCB_SHPR3 |= (0xFFUL << 16) | (0xFFUL << 24);
}

void port_setup_systick(uint32_t cpu_hz, uint32_t tick_hz)
{
    uint32_t reload = (cpu_hz / tick_hz) - 1UL;

    SYST_CSR = 0;
    SYST_RVR = reload;
    SYST_CVR = 0;
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;
}

void port_trigger_pendsv(void)
{
    SCB_ICSR = SCB_ICSR_PENDSVSET;
    __asm volatile ("dsb" ::: "memory");
    __asm volatile ("isb");
}

void port_enter_critical(void)
{
    uint32_t primask = read_primask();

    __asm volatile ("cpsid i" ::: "memory");

    if (critical_nesting == 0U) {
        saved_primask = primask;
    }

    critical_nesting++;
}

void port_exit_critical(void)
{
    if (critical_nesting == 0U) {
        return;
    }

    critical_nesting--;

    if (critical_nesting == 0U) {
        write_primask(saved_primask);
    }
}

uint32_t port_in_isr(void)
{
    return read_ipsr() != 0U;
}

void SysTick_Handler(void)
{
    rtos_tick_handler();
}
