# STM32F446RE mini RTOS lab

This repository starts as a minimal register-level STM32F446RE program for the
NUCLEO-F446RE board. The current milestone keeps the onboard LD2 LED blink
working while the project is being shaped into a small RTOS.

## Build

```sh
cd /home/biter/st/stm32
make
```

Outputs:

- `build/blink.elf`: ELF with symbols, used for flashing and debugging.
- `build/blink.bin`: raw binary image.
- `build/blink.map`: linker map file.

## Flash

```sh
make flash
```

This runs OpenOCD through the onboard ST-LINK:

```sh
openocd -f openocd.cfg -c "program build/blink.elf verify reset exit"
```

## Debug

Open terminal 1:

```sh
cd /home/biter/st/stm32
make openocd
```

Open terminal 2:

```sh
cd /home/biter/st/stm32
make debug
```

Useful GDB commands:

```gdb
monitor reset halt
break main
continue
next
step
info registers
x/16wx 0x40020000
monitor reset run
quit
```

## Probe task switching

After flashing, run:

```sh
make probe
```

The probe script lets the firmware run for two seconds, halts the MCU, and
prints the producer/consumer task counters, queue state, RTOS tick count, and
task debug snapshots
from SRAM. The producer sends a queue message every 250 ms, and the consumer
blocks on that queue before toggling LD2. The consumer runs at a higher
priority than the producer, so queue send should promptly wake it. The demo
also protects the producer/consumer handoff with a recursive mutex and counts
producer priority inheritance events. A third task waits on an empty semaphore
with a 100 ms timeout so timeout wakeups are visible in SRAM. The debug
snapshots include each task's TCB pointer, PSP, stack base, stack use estimate,
delay, state, wait result, and effective priority.

## Bare-metal structure

- `app/main.c` contains the current blink application.
- `board/board.c` owns NUCLEO-F446RE board setup and LD2 control.
- `board/stm32f446re.h` contains the small register map used so far.
- `startup/startup_stm32f4.c` defines the vector table and `Reset_Handler`.
- `Reset_Handler` copies `.data` from flash to RAM, clears `.bss`, then calls `main`.
- `ld/stm32f446re.ld` places the vector table and code at `0x08000000`, and RAM at `0x20000000`.
- `kernel/` owns task, scheduler, tick, sync, and debug snapshot code.
- `port/cortex_m4/` is reserved for Cortex-M4 context switching, PendSV, SysTick, and critical sections.

## RTOS milestones

1. Keep blink working in the new project layout. Done.
2. Add cooperative task switching with independent task stacks and PendSV. Done.
3. Add SysTick-based preemption. Done.
4. Add `rtos_sleep()` and blocked task wakeup. Done.
5. Add nested critical sections. Done.
6. Add counting semaphores. Done.
7. Add fixed-size message queues. Done.
8. Add priority scheduling with same-priority round-robin. Done.
9. Add recursive mutexes. Done.
10. Add mutex priority inheritance. Done.
11. Add debug task listing. Done.
12. Add an SVC-based scheduler start path. Done.
13. Add basic fault diagnostics. Done.
14. Add timeouts for blocking primitives. Done.
15. Add runtime assertions. Done.
16. Add stack overflow checks. Done.
17. Add software timers.

## Typical edit loop

1. Change code under `app/`, `board/`, `kernel/`, or `port/`.
2. Run `make`.
3. Run `make flash`.
4. For debugging, keep `make openocd` running and attach with `make debug`.
