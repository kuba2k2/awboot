# Target
CROSS_COMPILE ?= arm-none-eabi-

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE = $(CROSS_COMPILE)size

# Log level defaults to info
LOG_LEVEL ?= 30
BOARD ?= mangopi-dual
SOC := $(shell grep "\#define SOC" boards/$(BOARD)/board.h | cut -d \" -f2)
TARGET := awboot-$(BOARD)
BUILD_REVISION ?= $(shell git describe --tags --abbrev=7 --always || echo unknown)

CFLAGS += -DLOG_LEVEL=$(LOG_LEVEL)
CFLAGS += -DBOARD=$(BOARD)
CFLAGS += -DBUILD_REVISION=\"$(BUILD_REVISION)\"
CFLAGS += -I . -I lib -I boards/$(BOARD)

SRCS := main.c main_fel.c main_sdcard.c main_spinor.c boards/$(BOARD)/board.c
ASRCS :=

include	arch/arm32/arm32.mk
include	lib/lib.mk

CFLAGS += -ffunction-sections -fdata-sections -ffast-math
CFLAGS += -fno-stack-protector -fno-builtin -ffreestanding
CFLAGS += -Os -std=gnu99 -Wall -Werror -Wno-unused-function -Wno-format
CFLAGS += -g -MMD

ASFLAGS += $(CFLAGS) -Wl,--entry=reset
LDFLAGS += $(CFLAGS) -Wl,--gc-sections -nostdlib -lgcc

# Objects
OBJ_DIR = build-$(BOARD)
BUILD_OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)
BUILD_OBJSA = $(ASRCS:%.S=$(OBJ_DIR)/%.o)
OBJS = $(BUILD_OBJSA) $(BUILD_OBJS)

all: begin mkboot

begin:
	@echo "---------------------------------------------------------------"
	@echo -n "Compiler version: "
	@$(CC) -v 2>&1 | tail -1

.PHONY: all clean tools
.SILENT:

build: $(OBJ_DIR)/$(TARGET)-boot.bin $(OBJ_DIR)/$(TARGET)-fel.bin

.SECONDARY : $(TARGET)
.PRECIOUS : $(OBJS)
$(OBJ_DIR)/$(TARGET)-fel.elf: $(OBJS)
	echo "  LD    $@"
	$(CC) -E -P -x c -D__RAM_BASE=0x00030000 ./arch/arm32/sunxi/$(SOC)/link.ld > $(OBJ_DIR)/link-fel.ld
	$(CC) $^ -o $@ $(LIB_DIR) -T $(OBJ_DIR)/link-fel.ld $(LDFLAGS) -Wl,-Map,$(OBJ_DIR)/$(TARGET)-fel.map

$(OBJ_DIR)/$(TARGET)-boot.elf: $(OBJS)
	echo "  LD    $@"
	$(CC) -E -P -x c -D__RAM_BASE=0x00020000 ./arch/arm32/sunxi/$(SOC)/link.ld > $(OBJ_DIR)/link-boot.ld
	$(CC) $^ -o $@ $(LIB_DIR) -T $(OBJ_DIR)/link-boot.ld $(LDFLAGS) -Wl,-Map,$(OBJ_DIR)/$(TARGET)-boot.map

$(OBJ_DIR)/$(TARGET)-fel.bin: $(OBJ_DIR)/$(TARGET)-fel.elf
	@echo OBJCOPY $@
	$(OBJCOPY) -O binary $< $@

$(OBJ_DIR)/$(TARGET)-boot.bin: $(OBJ_DIR)/$(TARGET)-boot.elf
	@echo OBJCOPY $@
	$(OBJCOPY) -O binary $< $@

$(OBJ_DIR)/%.o : %.c
	echo "  CC    $@"
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(OBJ_DIR)/%.o : %.S
	echo "  CC    $@"
	mkdir -p $(@D)
	$(CC) $(ASFLAGS) $(INCLUDE_DIRS) -c $< -o $@

-include $(patsubst %.o,%.d,$(OBJS))

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)-*.bin
	rm -f *.d
	$(MAKE) -C tools clean

format:
	find . -iname "*.h" -o -iname "*.c" | xargs clang-format --verbose -i

tools:
	$(MAKE) -C tools all

mkboot: build tools
	$(SIZE) $(OBJ_DIR)/$(TARGET)-boot.elf
	cp -f $(OBJ_DIR)/$(TARGET)-fel.bin $(TARGET)-fel.bin
	cp -f $(OBJ_DIR)/$(TARGET)-boot.bin $(TARGET)-boot-sd.bin
	cp -f $(OBJ_DIR)/$(TARGET)-boot.bin $(TARGET)-boot-spi.bin
	tools/mksunxi $(TARGET)-fel.bin 8192
	tools/mksunxi $(TARGET)-boot-sd.bin 512
	tools/mksunxi $(TARGET)-boot-spi.bin 8192

boot: mkboot
	xfel ddr $(SOC)
	xfel write 0x30000 $(TARGET)-fel.bin
	xfel write 0x47000000 boot.img || true
	xfel exec 0x30000

flash-spinor: mkboot
	xfel spinor
	xfel spinor erase 0 0x8000
	xfel spinor write 0 $(TARGET)-boot-spi.bin
