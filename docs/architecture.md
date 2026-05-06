# Mini RTOS architecture

The first goal is a small Cortex-M4 RTOS for the NUCLEO-F446RE. The code is
split so the kernel can grow without mixing board-specific GPIO code with
context switching and scheduling.

## Layers

```text
app/        Demo tasks and application entry point.
board/      NUCLEO-F446RE setup, GPIO, UART, and board-level helpers.
kernel/     Task objects, scheduler, tick handling, delays, and sync objects.
port/       Cortex-M4 CPU port: PSP/MSP setup, PendSV, SysTick, SVC, critical sections.
startup/    Vector table and Reset_Handler.
ld/         STM32F446RE linker script.
include/    Public headers used across layers.
```

## Ownership

`kernel/` should not know STM32 peripheral register addresses. It owns task
state and scheduling decisions.

`port/cortex_m4/` should not know application tasks or board LEDs. It owns the
Cortex-M mechanism for switching CPU context.

`board/` should not know scheduler internals. It owns concrete hardware setup
for the NUCLEO-F446RE board.

`app/` should use public APIs from `board.h` and, once implemented, `rtos.h`.

## First milestones

1. Keep the existing blink application building in the new layout.
2. Add static task creation and per-task stacks.
3. Implement cooperative `rtos_yield()` through PendSV.
4. Add SysTick and round-robin preemption.
5. Add `rtos_sleep()` so tasks block instead of spinning.
6. Add critical sections and basic synchronization primitives.

## Cortex-M context switch model

Cortex-M hardware stacks `r0-r3`, `r12`, `lr`, `pc`, and `xPSR` on exception
entry. The RTOS PendSV handler must save and restore `r4-r11`, then update the
current task stack pointer.

SysTick should request scheduling, and PendSV should perform the context
switch at the lowest interrupt priority.
