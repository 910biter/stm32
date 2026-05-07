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

1. Keep the existing blink application building in the new layout. Done.
2. Add static task creation and per-task stacks. Done.
3. Implement cooperative `rtos_yield()` through PendSV. Done.
4. Add SysTick and round-robin preemption. Done.
5. Add `rtos_sleep()` so tasks block instead of spinning. Done.
6. Add nested critical sections. Done.
7. Add counting semaphores. Done.
8. Add queues, priority scheduling, mutexes, and debug task listing.

## Cortex-M context switch model

Cortex-M hardware stacks `r0-r3`, `r12`, `lr`, `pc`, and `xPSR` on exception
entry. The RTOS PendSV handler must save and restore `r4-r11`, then update the
current task stack pointer.

SysTick should request scheduling, and PendSV should perform the context
switch at the lowest interrupt priority.

The first cooperative implementation starts the initial task directly from
Thread mode using PSP, then uses PendSV for subsequent task switches. A later
milestone can replace that bootstrap with an SVC-based start path.

The first preemptive implementation configures SysTick from the default
16 MHz reset clock and requests PendSV from `SysTick_Handler`. The demo tasks
busy-loop without calling `rtos_yield()`, so round-robin progress now depends
on the tick interrupt.

`rtos_sleep()` marks the current task as blocked and stores a delay in ticks.
Each SysTick decrements blocked task delays and moves expired tasks back to the
ready state. The kernel creates an internal idle task during `rtos_start()` so
the scheduler always has a ready task while application tasks are sleeping.

Critical sections are exposed as `rtos_enter_critical()` and
`rtos_exit_critical()`. The Cortex-M4 port saves the outermost PRIMASK value,
disables interrupts, tracks nested entries, and restores the saved PRIMASK only
when the final nested critical section exits.

Counting semaphores are exposed as `rtos_sem_t`. A wait with no available count
blocks the current task and links it onto the semaphore wait FIFO. Posting wakes
the oldest waiting task, or increments the count up to `max_count` when no task
is waiting.
