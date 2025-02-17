LIB := lib
FS_FAT := $(LIB)/fatfs
LIBFDT := $(LIB)/libfdt

CFLAGS += -I $(FS_FAT) -I $(LIBFDT) -I $(LIB)

USE_FAT = $(shell grep -E "^\#define CONFIG_BOOT_(SDCARD|MMC)" boards/$(BOARD)/board.h)

ifneq ($(USE_FAT),)
SRCS    +=  $(FS_FAT)/ff.c
SRCS    +=  $(FS_FAT)/diskio.c
SRCS    +=  $(FS_FAT)/ffsystem.c
SRCS    +=  $(FS_FAT)/ffunicode.c
endif

SRCS    +=  $(LIBFDT)/fdt_addresses.c
SRCS    +=  $(LIBFDT)/fdt_check.c
SRCS    +=  $(LIBFDT)/fdt_empty_tree.c
SRCS    +=  $(LIBFDT)/fdt_overlay.c
SRCS    +=  $(LIBFDT)/fdt_ro.c
SRCS    +=  $(LIBFDT)/fdt_rw.c
SRCS    +=  $(LIBFDT)/fdt_strerror.c
SRCS    +=  $(LIBFDT)/fdt_sw.c
SRCS    +=  $(LIBFDT)/fdt_wip.c
SRCS    +=  $(LIBFDT)/fdt.c

SRCS	+=  $(LIB)/atag.c
SRCS	+=  $(LIB)/bootimg.c
SRCS	+=  $(LIB)/debug.c
SRCS	+=  $(LIB)/console.c
SRCS	+=  $(LIB)/console_cmds.c
SRCS	+=  $(LIB)/fdtutil.c
SRCS	+=  $(LIB)/ringbuffer.c
SRCS	+=  $(LIB)/string.c
SRCS	+=  $(LIB)/strtok.c
SRCS	+=  $(LIB)/xformat.c
