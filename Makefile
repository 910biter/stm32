TARGET := blink
BUILD_DIR := build

CC := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE := arm-none-eabi-size
GDB := arm-none-eabi-gdb
OPENOCD := openocd
PYTHON := python3

CPU := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS := $(CPU) -std=c11 -Wall -Wextra -Werror -O0 -g3 -ffunction-sections -fdata-sections
CFLAGS += -Iinclude -Iboard -Iport/cortex_m4
ASFLAGS := $(CPU) -g3
LDSCRIPT := ld/stm32f446re.ld
LDFLAGS := $(CPU) -T $(LDSCRIPT) -nostartfiles -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map
PROBE_SCRIPT := $(BUILD_DIR)/probe_counters.openocd
PROBE_LOG := $(BUILD_DIR)/probe.log

C_SOURCES := \
	app/main.c \
	board/board.c \
	kernel/critical.c \
	kernel/debug.c \
	kernel/event.c \
	kernel/list.c \
	kernel/mempool.c \
	kernel/mutex.c \
	kernel/object.c \
	kernel/queue.c \
	kernel/sched.c \
	kernel/sem.c \
	kernel/task.c \
	kernel/tick.c \
	kernel/timer.c \
	kernel/trace.c \
	kernel/work.c \
	kernel/assert.c \
	port/cortex_m4/port.c \
	port/cortex_m4/fault.c \
	startup/startup_stm32f4.c

ASM_SOURCES := \
	port/cortex_m4/port_asm.S

OBJECTS := $(C_SOURCES:%.c=$(BUILD_DIR)/%.o)
OBJECTS += $(ASM_SOURCES:%.S=$(BUILD_DIR)/%.o)

.PHONY: all clean flash debug openocd probe probe-script check-probe

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LDSCRIPT)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: all
	$(OPENOCD) -f openocd.cfg -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

openocd:
	$(OPENOCD) -f openocd.cfg

$(PROBE_SCRIPT): $(BUILD_DIR)/$(TARGET).elf tools/gen_probe.py include/rtos_config.h | $(BUILD_DIR)
	$(PYTHON) tools/gen_probe.py $(BUILD_DIR)/$(TARGET).elf include/rtos_config.h > $@

probe-script: $(PROBE_SCRIPT)

probe: $(PROBE_SCRIPT)
	$(OPENOCD) -f openocd.cfg -f $(PROBE_SCRIPT)

check-probe: $(PROBE_SCRIPT)
	bash -o pipefail -c '$(OPENOCD) -f openocd.cfg -f $(PROBE_SCRIPT) 2>&1 | tee $(PROBE_LOG)'
	$(PYTHON) tools/check_probe.py $(PROBE_LOG)

debug: all
	$(GDB) $(BUILD_DIR)/$(TARGET).elf -ex "target extended-remote localhost:3333"

clean:
	rm -rf $(BUILD_DIR)
