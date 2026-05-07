#!/usr/bin/env python3
import re
import subprocess
import sys


SNAPSHOT_WORDS = 14
OBJECT_SNAPSHOT_WORDS = 3


def load_symbols(elf_path):
    output = subprocess.check_output(["arm-none-eabi-nm", "-n", elf_path], text=True)
    symbols = {}
    for line in output.splitlines():
        parts = line.split()
        if len(parts) == 3:
            address, _kind, name = parts
            symbols[name] = int(address, 16)
    return symbols


def load_config_define(config_path, define_name):
    pattern = re.compile(rf"^\s*#define\s+{re.escape(define_name)}\s+(\d+)U?\s*$")
    with open(config_path, "r", encoding="utf-8") as config:
        for line in config:
            match = pattern.match(line)
            if match:
                return int(match.group(1))
    raise SystemExit(f"{define_name} not found")


def symbol(symbols, name):
    try:
        return symbols[name]
    except KeyError:
        raise SystemExit(f"symbol not found: {name}") from None


def emit_mem(name, address, count):
    print(f"mem2array {name} 32 0x{address:08x} {count}")


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: gen_probe.py <elf> <rtos_config.h>")

    symbols = load_symbols(sys.argv[1])
    max_tasks = load_config_define(sys.argv[2], "RTOS_MAX_TASKS")
    max_objects = load_config_define(sys.argv[2], "RTOS_MAX_OBJECTS")

    counter_names = [
        "app_task1_count",
        "app_task2_count",
        "app_priority_boost_count",
        "app_timeout_count",
        "app_timer_count",
        "app_event_count",
        "app_pool_count",
        "app_exit_count",
        "app_reuse_count",
        "app_sched_lock_count",
        "app_isr_sem_count",
        "app_idle_hook_count",
        "app_notify_count",
        "app_deferred_count",
    ]

    print("init")
    print("reset run")
    print("sleep 2000")
    print("halt")
    emit_mem("counters", symbol(symbols, counter_names[0]), len(counter_names))
    emit_mem("queue_state", symbol(symbols, "led_queue"), 5)
    emit_mem("mutex_state", symbol(symbols, "app_mutex"), 2)
    emit_mem("sem_state", symbol(symbols, "timeout_sem"), 4)
    emit_mem("event_state", symbol(symbols, "demo_events"), 3)
    emit_mem("pool_state", symbol(symbols, "demo_pool"), 7)
    emit_mem("debug_count", symbol(symbols, "rtos_debug_snapshot_count"), 1)
    emit_mem("snapshots", symbol(symbols, "rtos_debug_snapshots"), max_tasks * SNAPSHOT_WORDS)
    emit_mem("cpu_usage", symbol(symbols, "rtos_cpu_usage_snapshot"), 5)
    emit_mem("object_count", symbol(symbols, "rtos_object_snapshot_count"), 1)
    emit_mem("object_snapshots", symbol(symbols, "rtos_object_snapshots"), max_objects * OBJECT_SNAPSHOT_WORDS)
    emit_mem("tick_state", symbol(symbols, "tick_count"), 1)
    emit_mem("assert_info", symbol(symbols, "rtos_assert_info"), 6)
    emit_mem("kernel_state", symbol(symbols, "critical_nesting"), 2)
    emit_mem("scheduler_state", symbol(symbols, "rtos_scheduler_lock_count"), 2)
    emit_mem("work_state", symbol(symbols, "rtos_work_pending_items"), 2)
    emit_mem("fault_info", symbol(symbols, "rtos_fault_info"), 17)

    print('echo [format "task counters: produced=%u consumed=%u boost_count=%u timeout_count=%u timer_count=%u event_count=%u pool_count=%u exit_count=%u reuse_count=%u sched_lock_count=%u isr_sem_count=%u idle_hook_count=%u notify_count=%u deferred_count=%u queue_count=%u timeout_sem_count=%u event_flags=0x%08x pool_free=%u mutex_owner=0x%08x mutex_lock_count=%u tick=%u critical_nesting=%u saved_primask=%u scheduler_lock=%u scheduler_pending=%u work_pending=%u work_drops=%u snapshot_count=%u" $counters(0) $counters(1) $counters(2) $counters(3) $counters(4) $counters(5) $counters(6) $counters(7) $counters(8) $counters(9) $counters(10) $counters(11) $counters(12) $counters(13) $queue_state(2) $sem_state(0) $event_state(0) $pool_state(4) $mutex_state(0) $mutex_state(1) $tick_state(0) $kernel_state(0) $kernel_state(1) $scheduler_state(0) $scheduler_state(1) $work_state(0) $work_state(1) $debug_count(0)]')

    for task_index in range(max_tasks):
        base = task_index * SNAPSHOT_WORDS
        args = " ".join(f"$snapshots({base + field})" for field in range(SNAPSHOT_WORDS))
        print(f'echo [format "task{task_index}: name=0x%08x tcb=0x%08x sp=0x%08x stack=0x%08x words=%u used=%u delay=%u base=%u effective=%u state=%u wait=%d guard=%u switches=%u run_ticks=%u" {args}]')

    print('echo [format "cpu: total=%u idle=%u active=%u idle_permille=%u active_permille=%u" $cpu_usage(0) $cpu_usage(1) $cpu_usage(2) $cpu_usage(3) $cpu_usage(4)]')
    print('echo [format "object_count=%u" $object_count(0)]')
    for object_index in range(max_objects):
        base = object_index * OBJECT_SNAPSHOT_WORDS
        args = " ".join(f"$object_snapshots({base + field})" for field in range(OBJECT_SNAPSHOT_WORDS))
        print(f'echo [format "object{object_index}: type=%u object=0x%08x name=0x%08x" {args}]')

    print('echo [format "assert: active=%u expression=0x%08x file=0x%08x line=%u tick=%u task=0x%08x" $assert_info(0) $assert_info(1) $assert_info(2) $assert_info(3) $assert_info(4) $assert_info(5)]')
    print('echo [format "fault: active=%u exc_return=0x%08x sp=0x%08x pc=0x%08x lr=0x%08x xpsr=0x%08x cfsr=0x%08x hfsr=0x%08x bfar=0x%08x mmfar=0x%08x" $fault_info(0) $fault_info(1) $fault_info(2) $fault_info(9) $fault_info(8) $fault_info(10) $fault_info(11) $fault_info(12) $fault_info(15) $fault_info(16)]')
    print("shutdown")


if __name__ == "__main__":
    main()
