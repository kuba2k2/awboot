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

all: begin build mkboot

begin:
	@echo "---------------------------------------------------------------"
	@echo -n "Compiler version: "
	@$(CC) -v 2>&1 | tail -1

.PHONY: tools boot.img
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
	$(SIZE) $<

$(OBJ_DIR)/$(TARGET)-boot.bin: $(OBJ_DIR)/$(TARGET)-boot.elf
	@echo OBJCOPY $@
	$(OBJCOPY) -O binary $< $@
	$(SIZE) $<

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
	rm -f $(TARGET)
	rm -f $(TARGET)-*.bin
	rm -f $(TARGET)-*.map
	rm -f boards/board.h
	rm -f *.img
	rm -f *.d
	$(MAKE) -C tools clean

format:
	find . -iname "*.h" -o -iname "*.c" | xargs clang-format --verbose -i

tools:
	$(MAKE) -C tools all

mkboot: build tools
	cp $(OBJ_DIR)/$(TARGET)-fel.bin $(TARGET)-fel.bin
	cp $(OBJ_DIR)/$(TARGET)-boot.bin $(TARGET)-boot.bin
	cp $(OBJ_DIR)/$(TARGET)-boot.bin $(TARGET)-boot-spi.bin
	cp $(OBJ_DIR)/$(TARGET)-boot.bin $(TARGET)-boot-spi-4k.bin
	cp $(OBJ_DIR)/$(TARGET)-boot.bin $(TARGET)-boot-sd.bin
	tools/mksunxi $(TARGET)-fel.bin 8192
	tools/mksunxi $(TARGET)-boot-spi.bin 8192
	tools/mksunxi $(TARGET)-boot-spi-4k.bin 8192 4096
	tools/mksunxi $(TARGET)-boot-sd.bin 512

spi-boot.img: mkboot
	rm -f spi-boot.img
	dd if=$(TARGET)-boot-spi.bin of=spi-boot.img bs=2k
	dd if=$(TARGET)-boot-spi.bin of=spi-boot.img bs=2k seek=32 # Second copy on page 32
	dd if=$(TARGET)-boot-spi.bin of=spi-boot.img bs=2k seek=64 # Third copy on page 64
	# dd if=linux/boot/$(DTB) of=spi-boot.img bs=2k seek=128 # DTB on page 128
	# dd if=linux/boot/$(KERNEL) of=spi-boot.img bs=2k seek=256 # Kernel on page 256
