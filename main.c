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

#ifdef CONFIG_BOOT_FEL
	if (load_fel(&boot_info) == 0)
		goto _boot;
	else
		warning("BOOT: FEL boot failed (0x%08X)\r\n", CONFIG_BOOT_IMG_ADDR_FEL);
#endif

#ifdef CONFIG_BOOT_SDCARD
	if (load_sdcard(&boot_info) == 0)
		goto _boot;
	else
		warning("BOOT: SD card boot failed (%s)\r\n", CONFIG_BOOT_IMG_FILENAME);
#endif

#ifdef CONFIG_BOOT_SPINAND
	if (load_spinand(&boot_info) == 0)
		goto _boot;
	else
		warning("BOOT: SPI-NAND boot failed (0x%X)\r\n", CONFIG_BOOT_IMG_ADDR_SPINAND);
#endif

#ifdef CONFIG_BOOT_SPINOR
	if (load_spinor(&boot_info) == 0)
		goto _boot;
	else
		warning("BOOT: SPI-NOR boot failed (0x%X)\r\n", CONFIG_BOOT_IMG_ADDR_SPINOR);
#endif

	fatal("BOOT: all boot options failed\r\n");

_boot:

	if (boot_info.kernel_addr == 0)
		fatal("BOOT: kernel address is NULL\r\n");
	info("BOOT: kernel @ 0x%X, ramdisk @ 0x%X\r\n", boot_info.kernel_addr, boot_info.ramdisk_addr);

	linux_zimage_header_t *zimage_header = (linux_zimage_header_t *)boot_info.kernel_addr;
	if (zimage_header->magic != LINUX_ZIMAGE_MAGIC)
		fatal("BOOT: invalid kernel image magic\r\n");

	kernel_entry_t kernel_entry = (void *)((unsigned int)zimage_header + zimage_header->start);
	unsigned int kernel_param	= 0;

	if (boot_info.dtb_addr) {
		info("BOOT: setting up DTB @ 0x%X\r\n", boot_info.dtb_addr);
		// setup device tree
		fixup_chosen_node((void *)boot_info.dtb_addr, (char *)boot_info.cmdline);
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
