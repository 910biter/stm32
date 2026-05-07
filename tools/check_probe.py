#!/usr/bin/env python3
import re
import sys


PAIR_RE = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)=(-?0x[0-9a-fA-F]+|-?\d+)")
TASK_RE = re.compile(r"^task(\d+):\s+(.*)$")


def parse_int(value):
    return int(value, 0)


def parse_pairs(text):
    return {key: parse_int(value) for key, value in PAIR_RE.findall(text)}


def load_probe(path):
    counters = None
    tasks = {}
    assertion = None
    fault = None

    with open(path, "r", encoding="utf-8", errors="replace") as log:
        for raw_line in log:
            line = raw_line.strip()
            if line.startswith("task counters:"):
                counters = parse_pairs(line)
                continue

            task_match = TASK_RE.match(line)
            if task_match:
                tasks[int(task_match.group(1))] = parse_pairs(task_match.group(2))
                continue

            if line.startswith("assert:"):
                assertion = parse_pairs(line)
                continue

            if line.startswith("fault:"):
                fault = parse_pairs(line)

    return counters, tasks, assertion, fault


def require(errors, condition, message):
    if not condition:
        errors.append(message)


def check_probe(counters, tasks, assertion, fault):
    errors = []

    require(errors, counters is not None, "missing task counters line")
    require(errors, assertion is not None, "missing assert line")
    require(errors, fault is not None, "missing fault line")
    require(errors, bool(tasks), "missing task snapshot lines")

    if counters is not None:
        require(errors, counters.get("produced", 0) >= 5, "producer did not run enough times")
        require(errors, counters.get("consumed", 0) >= 5, "consumer did not run enough times")
        require(errors, counters.get("consumed", 0) <= counters.get("produced", 0), "consumer count exceeded producer count")
        require(errors, counters.get("boost_count", 0) >= 4, "priority inheritance demo did not run enough times")
        require(errors, counters.get("timeout_count", 0) >= 2, "semaphore timeout demo did not run enough times")
        require(errors, counters.get("timer_count", 0) >= 3, "software timer did not fire enough times")
        require(errors, counters.get("event_count", 0) >= 3, "event flag demo did not run enough times")
        require(errors, counters.get("pool_count", 0) >= 3, "memory pool demo did not run enough times")
        require(errors, counters.get("exit_count") == 1, "one-shot task did not stop exactly once")
        require(errors, counters.get("reuse_count") == 1, "stopped task slot was not reused exactly once")
        require(errors, counters.get("queue_count", 0) <= 1, "queue retained more than one pending message")
        require(errors, counters.get("timeout_sem_count") == 0, "timeout semaphore unexpectedly has tokens")
        require(errors, (counters.get("event_flags", 0) & ~0x1) == 0, "event flags contain unexpected bits")
        require(errors, counters.get("pool_free") == 4, "memory pool did not return all blocks")
        require(errors, counters.get("mutex_lock_count", 0) <= 2, "mutex appears to be leaked")
        if counters.get("mutex_lock_count", 0) == 0:
            require(errors, counters.get("mutex_owner") == 0, "mutex has an owner with zero lock count")
        require(errors, counters.get("tick", 0) >= 1500, "tick count is lower than expected after probe run")
        require(errors, counters.get("critical_nesting") == 0, "critical section nesting was not restored")
        require(errors, counters.get("saved_primask") == 0, "saved PRIMASK was not restored")
        require(errors, counters.get("snapshot_count") == len(tasks), "snapshot count does not match task lines")

    if assertion is not None:
        require(errors, assertion.get("active") == 0, "runtime assertion is active")

    if fault is not None:
        require(errors, fault.get("active") == 0, "HardFault diagnostics are active")

    if tasks:
        require(errors, len(tasks) >= 5, "expected at least five task snapshots")
        require(errors, any(task.get("state") == 3 for task in tasks.values()), "no stopped task was observed")
        require(errors, any(task.get("run_ticks", 0) >= 1000 for task in tasks.values()), "no task accumulated run ticks")
        for index, task in sorted(tasks.items()):
            require(errors, task.get("tcb", 0) != 0, f"task{index} has no TCB pointer")
            require(errors, task.get("stack", 0) != 0, f"task{index} has no stack pointer")
            require(errors, task.get("words") == 128, f"task{index} stack size changed unexpectedly")
            require(errors, task.get("used", 0) > 0, f"task{index} stack watermark did not move")
            require(errors, task.get("guard") == 1, f"task{index} stack guard is corrupted")

    return errors


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: check_probe.py <probe.log>")

    counters, tasks, assertion, fault = load_probe(sys.argv[1])
    errors = check_probe(counters, tasks, assertion, fault)
    if errors:
        print("probe checks failed:")
        for error in errors:
            print(f"- {error}")
        raise SystemExit(1)

    print("probe checks passed")


if __name__ == "__main__":
    main()
