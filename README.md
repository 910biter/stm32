# NUCLEO-F446RE bare-metal blink

This is a minimal register-level STM32F446RE program for the NUCLEO board.
It blinks the onboard LD2 LED on PA5.

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

## Bare-metal structure

- `startup_stm32f4.c` defines the vector table and `Reset_Handler`.
- `Reset_Handler` copies `.data` from flash to RAM, clears `.bss`, then calls `main`.
- `linker.ld` places the vector table and code at `0x08000000`, and RAM at `0x20000000`.
- `main.c` enables the GPIOA peripheral clock, configures PA5 as output, then toggles it through `GPIOA_BSRR`.

## Typical edit loop

1. Change `main.c`.
2. Run `make`.
3. Run `make flash`.
4. For debugging, keep `make openocd` running and attach with `make debug`.
