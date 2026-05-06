TARGET := blink
BUILD_DIR := build

CC := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE := arm-none-eabi-size
GDB := arm-none-eabi-gdb
OPENOCD := openocd

CPU := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS := $(CPU) -std=c11 -Wall -Wextra -Werror -O0 -g3 -ffunction-sections -fdata-sections
LDFLAGS := $(CPU) -T linker.ld -nostartfiles -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

SOURCES := startup_stm32f4.c main.c
OBJECTS := $(SOURCES:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean flash debug openocd

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) linker.ld
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: all
	$(OPENOCD) -f openocd.cfg -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

openocd:
	$(OPENOCD) -f openocd.cfg

debug: all
	$(GDB) $(BUILD_DIR)/$(TARGET).elf -ex "target extended-remote localhost:3333"

clean:
	rm -rf $(BUILD_DIR)
