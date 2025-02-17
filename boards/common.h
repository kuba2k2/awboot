#pragma once

#include "dram.h"

#define CONFIG_BOOT_IMG_ADDR_FEL	 (SDRAM_BASE + SDRAM_SIZE - (16 * 1024 * 1024)) // 0x47000000
#define CONFIG_BOOT_IMG_FILENAME	 "boot.img"
#define CONFIG_BOOT_IMG_ADDR_SPINAND (128 * 1024) // 0x020000
#define CONFIG_BOOT_IMG_ADDR_SPINOR	 (128 * 1024) // 0x020000

#define CONFIG_FATFS_CACHE_SIZE		 (16 * 1024 * 1024)
#define CONFIG_FATFS_CACHE_ADDR		 (CONFIG_BOOT_IMG_ADDR_FEL - CONFIG_FATFS_CACHE_SIZE)
#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024 // (unit: 512B sectors)

#define CONFIG_LOAD_BUF_SIZE (0x10000)
#define CONFIG_LOAD_BUF_ADDR (CONFIG_FATFS_CACHE_ADDR - CONFIG_LOAD_BUF_SIZE)

// board configuration - these must be defined in board.h!
// #define CONFIG_CPU_FREQ ...
// #define SDRAM_SIZE ...
// #define USART_DBG ...

// enabled features
// #define CONFIG_ENABLE_CPU_FREQ_DUMP
// #define CONFIG_ENABLE_CONSOLE

// boot methods
// #define CONFIG_BOOT_FEL
// #define CONFIG_BOOT_MMC
// #define CONFIG_BOOT_SDCARD
// #define CONFIG_BOOT_SPINAND
// #define CONFIG_BOOT_SPINOR

// used for SD card boot only
#define CONFIG_SDMMC_KERNEL_FILENAME  "zImage"
#define CONFIG_SDMMC_RAMDISK_FILENAME "initrd.img"
#define CONFIG_SDMMC_DTB_FILENAME	  "awboot.dtb"
#define CONFIG_SDMMC_CMDLINE_FILENAME "cmdline.txt"
#define CONFIG_SDMMC_DTBO_FILENAME	  "overlays.txt"
#define CONFIG_SDMMC_DTBO_DIRNAME	  "overlays"
// file load memory addresses
#define CONFIG_SDMMC_ATAG_ADDR		  0x40100000 /* 512 KiB */
#define CONFIG_SDMMC_CMDLINE_ADDR	  0x40180000 /* 512 KiB */
#define CONFIG_SDMMC_KERNEL_ADDR	  0x40200000 /* 16,384 KiB */
#define CONFIG_SDMMC_RAMDISK_ADDR	  0x41200000 /* 16,384 KiB */
#define CONFIG_SDMMC_DTB_ADDR		  0x42200000
// override parts of boot.img with standalone files
// #define CONFIG_SDMMC_OVERRIDE_RAMDISK
// #define CONFIG_SDMMC_OVERRIDE_DTB
// #define CONFIG_SDMMC_OVERRIDE_CMDLINE
// support applying device tree overlays
// #define CONFIG_SDMMC_OVERLAY_SUPPORT

// only used in dma_test()
#define CONFIG_DTB_LOAD_ADDR	(CONFIG_FATFS_CACHE_ADDR - (512 * 1024))
#define CONFIG_KERNEL_LOAD_ADDR (CONFIG_DTB_LOAD_ADDR - (512 * 1024))

extern void board_init(void);
extern int board_sdhci_init(void);
