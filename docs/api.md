# Mini RTOS API notes

This project is still a toy RTOS, but the public API is now stable enough to
use as a learning surface. All application code should include `rtos.h`.

## Tasks

- `rtos_task_create_named(entry, arg, priority, name)` creates a static task.
- `rtos_task_create_named_handle(..., task_out)` also returns the TCB handle.
- `rtos_task_self()` returns the current task handle.
- `rtos_sleep(ms)` blocks the current task for at least the requested time.
- `rtos_yield()` requests a PendSV context switch.
- `rtos_task_delete_self()` stops the current task; stopped slots can be reused.

Priorities are integer values. Larger values run before smaller values, while
same-priority ready tasks round-robin from the current position in the circular
task list.

## Scheduler And Interrupt Context

- `rtos_sched_lock()` and `rtos_sched_unlock()` form a nestable preemption lock.
- `rtos_sched_lock_depth()` exposes the current lock depth.
- `rtos_in_isr()` reports whether the caller is running in exception context.

Scheduler locking does not disable interrupts. It only defers task switching
while the current task is still runnable. If the task blocks or stops, the
scheduler is allowed to switch so the kernel cannot continue running a blocked
task.

## Synchronization Objects

- Semaphores: `rtos_sem_init`, `rtos_sem_wait`, `rtos_sem_wait_timeout`,
  `rtos_sem_post`, `rtos_sem_post_isr`.
- Queues: `rtos_queue_init`, `rtos_queue_send`, `rtos_queue_send_timeout`,
  `rtos_queue_send_isr`, `rtos_queue_recv`, `rtos_queue_recv_timeout`.
- Mutexes: `rtos_mutex_init`, `rtos_mutex_lock`, `rtos_mutex_lock_timeout`,
  `rtos_mutex_unlock`.
- Event flags: `rtos_event_flags_init`, `rtos_event_flags_set`,
  `rtos_event_flags_set_isr`, `rtos_event_flags_clear`,
  `rtos_event_flags_wait`.
- Memory pools: `rtos_mempool_init`, `rtos_mempool_alloc`,
  `rtos_mempool_alloc_timeout`, `rtos_mempool_free`.

Blocking APIs must be called from task context. `_isr` APIs are non-blocking and
are safe for interrupt callbacks such as software timer callbacks.

## Task Notifications

Task notifications are the lightest signaling primitive in the kernel. The
notification value lives directly in the target TCB.

- `rtos_task_notify(task, bits)` ORs bits into a task notification value.
- `rtos_task_notify_isr(task, bits)` is the ISR-safe form.
- `rtos_task_notify_wait(clear_on_entry, clear_on_exit, value, timeout_ms)`
  waits on the current task's notification state.

Use notifications when one producer is signaling one known task and no separate
kernel object is needed.

## Timers And Deferred Work

- `rtos_timer_init`, `rtos_timer_start`, and `rtos_timer_stop` manage software
  timers.
- Timer callbacks run in SysTick interrupt context and must not block.
- `rtos_work_init()` creates the internal deferred work task.
- `rtos_work_submit()` and `rtos_work_submit_isr()` enqueue callbacks for that
  worker task to run in Thread mode.

Deferred work is the preferred way to keep interrupt handlers short while still
doing follow-up processing.

## Debugging

- `rtos_debug_snapshot()` copies task snapshots.
- `rtos_debug_cpu_usage_snapshot()` returns coarse tick-sampled CPU usage.
- `rtos_object_debug_snapshot()` copies the kernel object registry.
- `rtos_trace_record()` and `rtos_trace_record_isr()` append trace entries.

The normal board-side validation loop is:

```sh
make clean
make
make flash
make check-probe
```

`make check-probe` generates an OpenOCD script from the ELF symbol table, lets
the firmware run, captures SRAM debug state, and validates the demo counters,
task snapshots, object registry, CPU usage snapshot, trace ring, assertions,
and HardFault record.
