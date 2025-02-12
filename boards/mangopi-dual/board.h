#pragma once

#include "boards/common.h"

#define SOC "t113-s3"

#define CONFIG_CPU_FREQ 1200000000
#define SDRAM_SIZE		(128 * 1024 * 1024)
#define USART_DBG		usart5_dbg

#define CONFIG_ENABLE_CPU_FREQ_DUMP
// #define CONFIG_ENABLE_CONSOLE

#define CONFIG_BOOT_FEL
// #define CONFIG_BOOT_MMC
// #define CONFIG_BOOT_SDCARD
// #define CONFIG_BOOT_SPINAND
// #define CONFIG_BOOT_SPINOR
