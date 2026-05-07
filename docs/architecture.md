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
14. Add basic fault diagnostics. Done.
15. Add timeouts for blocking primitives. Done.
16. Add runtime assertions. Done.
17. Add stack overflow checks. Done.
18. Add software timers. Done.
19. Add event flags. Done.
20. Add memory pools. Done.
21. Add task lifecycle management. Done.
22. Add task slot reuse. Done.
23. Add priority-aware wait queues. Done.
24. Add per-task statistics. Done.
25. Add probe symbol generation. Done.
26. Add automated probe checks. Done.
27. Add kernel object registry. Done.
28. Add scheduler lock. Done.
29. Add ISR-safe wake APIs. Done.
30. Add idle hooks. Done.
31. Add CPU usage snapshots. Done.
32. Add task notifications. Done.
33. Add deferred interrupt work. Done.
34. Add trace ring buffer. Done.
35. Add kernel API cleanup docs.

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

HardFault diagnostics are captured in `rtos_fault_info`. The assembly wrapper
selects MSP or PSP from the exception return value, then the C handler records
the stacked core registers plus CFSR, HFSR, DFSR, AFSR, BFAR, and MMFAR before
parking the CPU. This gives OpenOCD a stable SRAM record to inspect after a
crash.

Blocking primitives have timeout variants. A blocked task records a wait result
and an optional delay. The tick handler marks timed-out tasks ready with
`RTOS_ERR_TIMEOUT`; the primitive then removes that task from its wait FIFO
before returning. Wake paths also skip waiters that already timed out, which
keeps object queues consistent when a timeout races with a post, send, receive,
or unlock.

Runtime assertions use `RTOS_ASSERT()`. An assertion failure disables
interrupts, records the expression, file, line, tick count, and current task in
`rtos_assert_info`, then parks the CPU. The scheduler now asserts that a ready
task was found, which should always hold because `rtos_start()` creates an idle
task.

Stack overflow checks reserve the low end of each downward-growing task stack
as guard words. The guard uses the same fill pattern as the stack watermark.
Each tick checks one task guard, rotating through the task table; a corrupted
guard trips `RTOS_ASSERT()` and leaves the failed expression in SRAM for
OpenOCD. Debug snapshots are refreshed at a lower rate than the scheduler tick
because stack-watermark scans are useful for inspection but too heavy to run in
every 1 kHz interrupt.

Software timers are lightweight `rtos_timer_t` objects linked into a kernel
timer list. SysTick decrements active timers and invokes the callback when the
period expires. Callbacks run in interrupt context, so they must stay short and
must not call blocking RTOS APIs. The demo uses a 500 ms periodic timer that
increments a SRAM counter.

Event flags are `uint32_t` bit sets with wait-any and wait-all modes. Waiting
tasks can optionally clear the matched bits on exit and can use the same timeout
path as queues, semaphores, and mutexes. Setting flags wakes every waiter whose
condition is satisfied.

Memory pools provide fixed-size block allocation without heap fragmentation.
Each pool owns caller-provided storage and links free blocks through the first
word of each block. Allocation can return immediately or block with the common
timeout path. Freeing a block hands it directly to the oldest valid waiter when
one exists, otherwise the block returns to the free list.

Task lifecycle management adds a `STOPPED` task state. A task that returns from
its entry function reaches `rtos_task_exit()`, marks itself stopped, and yields.
The scheduler skips stopped tasks. This first lifecycle step does not reclaim
the task slot or stack yet; it only makes task completion explicit and safe.

Task slot reuse lets `rtos_task_create_named()` recycle a stopped task control
block and its stack once the fixed task table is full. Reused tasks stay in the
same circular scheduler list, so creation only rebuilds the initial stack frame
and marks the task ready again. This keeps allocation static while allowing
short-lived tasks to be relaunched.

Wait queues are priority-aware. Semaphores, queues, mutexes, event flags, and
memory pools insert blocked tasks by effective priority, highest first. Tasks
with equal priority keep FIFO order. This makes wakeups line up with the
scheduler's priority model instead of allowing an older low-priority waiter to
run ahead of a newer high-priority waiter.

Per-task statistics track how often each task is selected by the scheduler and
how many SysTick intervals it has spent running. These counters live in the TCB
and are exported through debug snapshots, giving a simple view of scheduler
activity without needing trace hardware.

The OpenOCD probe script is generated from the ELF symbol table. `make probe`
builds the firmware, runs `tools/gen_probe.py`, and uses the generated script
under `build/`. This removes the brittle hand-maintained SRAM addresses from
the normal debug loop when `.bss` layout changes.

Automated probe checks turn that debug output into a hardware smoke test.
`make check-probe` records the OpenOCD output in `build/probe.log`, then
`tools/check_probe.py` verifies that the demo tasks made progress, stack guards
are intact, and neither runtime assertions nor HardFault diagnostics are active.

The kernel object registry records initialized queues, semaphores, mutexes,
timers, event flag groups, and memory pools in a fixed table. Objects register
themselves during initialization, and applications can attach debug names with
`rtos_object_set_name()`. The registry is exported through SRAM snapshots so
OpenOCD can show object addresses and types alongside task snapshots.

The scheduler lock is a nestable preemption lock. While locked, PendSV requests
from ticks, wakeups, or explicit yields are remembered in
`rtos_scheduler_pending` instead of switching away from a still-running task.
If the current task blocks or stops, the scheduler is still allowed to select a
runnable task, which prevents a blocked task from continuing in Thread mode.

ISR-safe wake APIs provide non-blocking entry points for interrupt context:
`rtos_sem_post_isr()`, `rtos_event_flags_set_isr()`, and
`rtos_queue_send_isr()`. They may wake tasks and request PendSV, but they do not
wait. The demo timer callback runs in the SysTick path and uses these APIs to
set an event flag and post a semaphore.

Idle hooks let the application run a short callback whenever the idle task is
selected. The hook runs in Thread mode at idle priority, before the idle task
executes `wfi`. It must not block, but it can maintain low-cost counters or
enter board-specific low-power preparation code.

CPU usage snapshots summarize scheduler activity with tick totals, idle ticks,
active ticks, and idle/active permille values. They are computed during debug
snapshot refreshes from each task's accumulated `run_ticks`, giving OpenOCD a
quick tick-sampled view of whether the system is mostly idle or busy. Very
short tasks can finish between SysTick interrupts, so this is a coarse system
health metric rather than a cycle-accurate profiler.

Task notifications provide a lightweight per-task signaling path. Each TCB owns
a pending flag and a 32-bit notification value. `rtos_task_notify()` and
`rtos_task_notify_isr()` OR bits into the target task, while
`rtos_task_notify_wait()` lets the current task consume the value or block with
a timeout. Notification wakeups check the task's wait type, so they do not
accidentally release tasks blocked on semaphores, queues, mutexes, events, or
memory pools.

Deferred interrupt work adds a fixed-size work queue and an internal worker
task. ISRs can call `rtos_work_submit_isr()` to enqueue a callback, then return
quickly. The worker task consumes pending work in Thread mode, making it the
right place for follow-up code that is too expensive or too stateful for an
interrupt handler.

The trace ring buffer records compact `{tick, event, arg0, arg1}` entries in
SRAM. It currently captures scheduler switches, task notifications, deferred
work submissions, and deferred work execution. OpenOCD reads the ring alongside
the task and object snapshots, giving a small flight recorder without UART or
semihosting.
