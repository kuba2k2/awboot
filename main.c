#include "main.h"
#include "fdt.h"
#include "ff.h"
#include "sunxi_gpio.h"
#include "sunxi_sdhci.h"
#include "sunxi_spi.h"
#include "sunxi_clk.h"
#include "sunxi_dma.h"
#include "sdmmc.h"
#include "arm32.h"
#include "debug.h"
#include "board.h"
#include "console.h"
#include "barrier.h"

unsigned long sdram_size;

/* Linux zImage Header */
#define LINUX_ZIMAGE_MAGIC 0x016f2818
typedef struct {
	unsigned int code[9];
	unsigned int magic;
	unsigned int start;
	unsigned int end;
} linux_zimage_header_t;

static int boot_image_setup(unsigned char *addr, unsigned int *entry)
{
	linux_zimage_header_t *zimage_header = (linux_zimage_header_t *)addr;

	if (zimage_header->magic == LINUX_ZIMAGE_MAGIC) {
		*entry = ((unsigned int)addr + zimage_header->start);
		return 0;
	}

	error("unsupported kernel image\r\n");

	return -1;
}

int main(void)
{
	linux_zimage_header_t *zimage_header;

	board_init();
	sunxi_clk_init();

	message("\r\n");
	info("AWBoot %s starting...\r\n", BUILD_REVISION);
	info("Built on "__DATE__
		 " at "__TIME__
		 "\r\n");

	sdram_size = sunxi_dram_init();
	info("DRAM size: %lu MiB\r\n", sdram_size >> 20);

#ifdef CONFIG_ENABLE_CONSOLE
	extern sunxi_usart_t USART_DBG;

	console_init(&USART_DBG);
	console_handler(CONSOLE_NO_TIMEOUT);
#endif

	void (*kernel_entry)(int zero, int arch, unsigned int params);
	unsigned int kernel_param;

#ifdef CONFIG_ENABLE_CPU_FREQ_DUMP
	sunxi_clk_dump();
#endif

#ifdef CONFIG_BOOT_FEL
	if (load_fel(&kernel_entry, &kernel_param) == 0)
		goto _boot;
	else
		warning("BOOT: FEL boot failed\r\n");
#endif

#ifdef CONFIG_BOOT_SDCARD
	if (load_sdcard(&kernel_entry, &kernel_param) == 0)
		goto _boot;
	else
		warning("BOOT: SD card boot failed\r\n");
#endif

#ifdef CONFIG_BOOT_SPINAND
	if (load_spinand(&kernel_entry, &kernel_param) == 0)
		goto _boot;
	else
		warning("BOOT: SPI-NAND boot failed\r\n");
#endif

#ifdef CONFIG_BOOT_SPINOR
	if (load_spinor(&kernel_entry, &kernel_param) == 0)
		goto _boot;
	else
		warning("BOOT: SPI-NOR boot failed\r\n");
#endif

	fatal("BOOT: all boot options failed\r\n");

_boot:

	zimage_header = (linux_zimage_header_t *)kernel_entry;
	if (zimage_header->magic != LINUX_ZIMAGE_MAGIC) {
		fatal("unsupported kernel image\r\n");
	}

#ifdef CONFIG_CMD_LINE_ARGS
	info("setup bootargs: %s\r\n", CONFIG_CMD_LINE_ARGS);
	fixup_chosen_node(image.of_dest, CONFIG_CMD_LINE_ARGS);
#endif

	kernel_entry = (void *)((unsigned int)kernel_entry + zimage_header->start);

	info("BOOT: booting Linux @ 0x%x\r\n", (unsigned int)kernel_entry);

	arm32_mmu_disable();
	arm32_dcache_disable();
	arm32_icache_disable();
	arm32_interrupt_disable();

	kernel_entry(0, 0x0, kernel_param);

	return 0;
}
