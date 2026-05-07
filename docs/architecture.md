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
8. Add fixed-size message queues. Done.
9. Add priority scheduling with same-priority round-robin. Done.
10. Add recursive mutexes. Done.
11. Add mutex priority inheritance. Done.
12. Add debug task listing. Done.
13. Add an SVC-based scheduler start path. Done.
14. Add basic fault diagnostics.

## Cortex-M context switch model

Cortex-M hardware stacks `r0-r3`, `r12`, `lr`, `pc`, and `xPSR` on exception
entry. The RTOS PendSV handler must save and restore `r4-r11`, then update the
current task stack pointer.

SysTick should request scheduling, and PendSV should perform the context
switch at the lowest interrupt priority.

The first cooperative implementation started the initial task directly from
Thread mode using PSP, then used PendSV for subsequent task switches. The
current startup path requests SVC from `rtos_start()`. `SVC_Handler` restores
the first task's callee-saved registers, points PSP at the synthetic exception
frame, selects PSP for Thread mode, and returns through `0xFFFFFFFD` so the
Cortex-M exception-return hardware enters the first task.

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

Queues are exposed as `rtos_queue_t` and currently carry `uint32_t` messages.
Receiving from an empty queue blocks on a receive wait FIFO. Sending to a full
queue blocks on a send wait FIFO. Send and receive operations wake the oldest
opposite-side waiter when progress becomes possible.

Tasks have integer priorities where larger values run first. The scheduler
scans from the current task's successor and chooses the highest-priority ready
task, preserving round-robin behavior among tasks with equal priority.

Mutexes are exposed as `rtos_mutex_t`. A mutex tracks an owner task, recursive
lock count, and wait FIFO. Unlocking the outermost lock releases ownership and
wakes the oldest waiter.

Mutex priority inheritance temporarily raises the owner's effective priority
when a higher-priority task blocks on the mutex. The owner returns to its base
priority when it releases the outermost recursive lock. This first version is
intentionally simple and assumes a task is not simultaneously inheriting through
multiple mutexes.

Debug snapshots expose the current task table without needing a C library or
UART. Each task carries an optional name pointer, and each stack is filled with
`0xA5A5A5A5` before the initial exception frame is built. The debug module scans
for the first overwritten word to estimate stack use, then publishes a fixed
array of snapshots that OpenOCD can read from SRAM.
