#pragma once

#include "boards/common.h"

#define SOC "v851s"

#define CONFIG_CPU_FREQ 900000000
#define SDRAM_SIZE		(64 * 1024 * 1024)
#define USART_DBG		usart_dbg

#define CONFIG_ENABLE_CPU_FREQ_DUMP
// #define CONFIG_ENABLE_CONSOLE

#define CONFIG_BOOT_FEL
// #define CONFIG_BOOT_MMC
#define CONFIG_BOOT_SDCARD
// #define CONFIG_BOOT_SPINAND
// #define CONFIG_BOOT_SPINOR
