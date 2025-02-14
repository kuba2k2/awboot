#include "main.h"

#include "arm32.h"
#include "atag.h"
#include "barrier.h"
#include "board.h"
#include "console.h"
#include "debug.h"
#include "fdt.h"
#include "sunxi_clk.h"

unsigned long sdram_size;

static int boot_image_setup(unsigned char *addr, unsigned int *entry) {
	linux_zimage_header_t *zimage_header = (linux_zimage_header_t *)addr;

	if (zimage_header->magic == LINUX_ZIMAGE_MAGIC) {
		*entry = ((unsigned int)addr + zimage_header->start);
		return 0;
	}

	error("unsupported kernel image\r\n");

	return -1;
}

int main(void) {
	board_init();
	sunxi_clk_init();

	message("\r\n");
	info("AWBoot starting...\r\n");
	info("Build date: " __DATE__ " at " __TIME__ "\r\n");
	info("Revision:   " BUILD_REVISION "\r\n");
	info("Board:      " STRINGIFY_MACRO(BOARD) "\r\n");
	info("SoC:        " SOC " @ %d MHz\r\n", CONFIG_CPU_FREQ / 1000000);

	sdram_size = sunxi_dram_init();
	info("DRAM size:  %lu MiB\r\n", sdram_size >> 20);
	info("\r\n");

#ifdef CONFIG_ENABLE_CONSOLE
	extern sunxi_usart_t USART_DBG;

	console_init(&USART_DBG);
	console_handler(CONSOLE_NO_TIMEOUT);
#endif

#ifdef CONFIG_ENABLE_CPU_FREQ_DUMP
	sunxi_clk_dump();
#endif

	boot_info_t boot_info;
	memset(&boot_info, 0, sizeof(boot_info));

	// boot from FEL
#ifdef CONFIG_BOOT_FEL
	if (load_fel(&boot_info) == 0)
		goto _boot;
	else
		warning("BOOT: FEL boot failed (0x%08X)\r\n", CONFIG_BOOT_IMG_ADDR_FEL);
#endif

	// boot from SD card
#ifdef CONFIG_BOOT_SDCARD
	if (load_sdcard(&boot_info) == 0)
		goto _boot;
	else
		warning("BOOT: SD card boot failed (%s)\r\n", CONFIG_BOOT_IMG_FILENAME);
#endif

	// boot from SPI NAND
#ifdef CONFIG_BOOT_SPINAND
	if (load_spinand(&boot_info) == 0)
		goto _boot;
	else
		warning("BOOT: SPI-NAND boot failed (0x%X)\r\n", CONFIG_BOOT_IMG_ADDR_SPINAND);
#endif

	// boot from SPI NOR
#ifdef CONFIG_BOOT_SPINOR
	if (load_spinor(&boot_info) == 0)
		goto _boot;
	else
		warning("BOOT: SPI-NOR boot failed (0x%X)\r\n", CONFIG_BOOT_IMG_ADDR_SPINOR);
#endif

	// fail if all boot options failed
	fatal("BOOT: all boot options failed\r\n");
	while (1) {}

_boot:
	// check if kernel address was set
	if (boot_info.kernel_addr == 0)
		fatal("BOOT: kernel address is NULL\r\n");
	info("BOOT: kernel @ 0x%X, ramdisk @ 0x%X\r\n", boot_info.kernel_addr, boot_info.ramdisk_addr);

	// verify zImage header
	linux_zimage_header_t *zimage_header = (linux_zimage_header_t *)boot_info.kernel_addr;
	if (zimage_header->magic != LINUX_ZIMAGE_MAGIC)
		fatal("BOOT: invalid kernel image magic\r\n");

	// calculate kernel entry point
	kernel_entry_t kernel_entry = (void *)((unsigned int)zimage_header + zimage_header->start);
	unsigned int kernel_param	= 0;

	// check if mem= is present in cmdline
	if (boot_info.cmdline == NULL || strstr(boot_info.cmdline, "mem=") == NULL) {
		warning("BOOT: 'mem=' missing from cmdline, fixing\r\n");
		unsigned int size_mb = sdram_size >> 20;
		if (boot_info.cmdline == NULL) {
			boot_info.cmdline	 = (void *)CONFIG_SDMMC_CMDLINE_ADDR;
			boot_info.cmdline[0] = '\0';
		}
		char *cmdline = boot_info.cmdline + strlen(boot_info.cmdline);
		if (cmdline != boot_info.cmdline)
			*cmdline++ = ' ';

		memcpy(cmdline, "mem=", 4);
		cmdline += 4;
		if (size_mb >= 1000)
			*cmdline++ = '0' + (size_mb / 1000) % 10;
		if (size_mb >= 100)
			*cmdline++ = '0' + (size_mb / 100) % 10;
		*cmdline++ = '0' + (size_mb / 10) % 10;
		*cmdline++ = '0' + size_mb % 10;
		*cmdline++ = 'M';
		*cmdline   = '\0';
	}

	info("BOOT: command line: %s\r\n", boot_info.cmdline);

	if (boot_info.dtb_addr) {
		if (check_dt_blob_valid((void *)boot_info.dtb_addr) != 0)
			warning("BOOT: invalid DTB found @ 0x%X\r\n", boot_info.dtb_addr);
		else
			info("BOOT: setting up DTB @ 0x%X\r\n", boot_info.dtb_addr);
		// setup device tree
		unsigned int mem_bank  = SDRAM_BASE;
		unsigned int mem_bank2 = 0;
		unsigned int mem_size  = sdram_size;
		fixup_chosen_node((void *)boot_info.dtb_addr, boot_info.cmdline);
		fixup_memory_node((void *)boot_info.dtb_addr, &mem_bank, &mem_bank2, &mem_size);
		kernel_param = boot_info.dtb_addr;
	} else if (boot_info.tags_addr) {
		info("BOOT: setting up legacy ATAGs @ 0x%X\r\n", boot_info.tags_addr);
		// setup atags
		setup_core_tag((void *)boot_info.tags_addr, 4096);
		setup_mem_tag(SDRAM_BASE, sdram_size);
		setup_ramdisk_tag(16 * 1024);
		if (boot_info.ramdisk_addr)
			setup_initrd2_tag(boot_info.ramdisk_addr, boot_info.ramdisk_size);
		if (boot_info.cmdline)
			setup_cmdline_tag(boot_info.cmdline);
		setup_end_tag();
		kernel_param = boot_info.tags_addr;
	}

	info("BOOT: booting Linux @ 0x%x\r\n", (unsigned int)kernel_entry);

	arm32_mmu_disable();
	arm32_dcache_disable();
	arm32_icache_disable();
	arm32_interrupt_disable();

	kernel_entry(0, 0x0, kernel_param);

	return 0;
}
