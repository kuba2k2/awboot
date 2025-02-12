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

#if defined(CONFIG_BOOT_SDCARD) || defined(CONFIG_BOOT_MMC)
#define CHUNK_SIZE 0x20000

static int fatfs_loadimage(char *filename, BYTE *dest)
{
	FIL					  file;
	UINT				  byte_to_read = CHUNK_SIZE;
	UINT				  byte_read;
	UINT				  total_read = 0;
	FRESULT				  fret;
	int					  ret;
	uint32_t UNUSED_DEBUG start, time;

	fret = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
	if (fret != FR_OK) {
		error("FATFS: open, filename: [%s]: error %d\r\n", filename, fret);
		ret = -1;
		goto open_fail;
	}

	start = time_ms();

	do {
		byte_read = 0;
		fret	  = f_read(&file, (void *)(dest), byte_to_read, &byte_read);
		dest += byte_to_read;
		total_read += byte_read;
	} while (byte_read >= byte_to_read && fret == FR_OK);

	time = time_ms() - start + 1;

	if (fret != FR_OK) {
		error("FATFS: read: error %d\r\n", fret);
		ret = -1;
		goto read_fail;
	}
	ret = 0;

read_fail:
	fret = f_close(&file);

	debug("FATFS: read in %" PRIu32 "ms at %.2fMB/S\r\n", time, (f32)(total_read / time) / 1024.0f);

open_fail:
	return ret;
}

static int load_sdcard(image_info_t *image)
{
	FATFS			 fs;
	FRESULT			 fret;
	int				 ret;
	u32 UNUSED_DEBUG start;

#if defined(CONFIG_SDMMC_SPEED_TEST_SIZE) && LOG_LEVEL >= LOG_DEBUG
	u32 test_time;
	start = time_ms();
	sdmmc_blk_read(&card0, (u8 *)(SDRAM_BASE), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
	test_time = time_ms() - start;
	debug("SDMMC: speedtest %uKB in %" PRIu32 "ms at %" PRIu32 "KB/S\r\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024,
		  test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);
#endif // SDMMC_SPEED_TEST

	start = time_ms();
	/* mount fs */
	fret = f_mount(&fs, "", 1);
	if (fret != FR_OK) {
		error("FATFS: mount error: %d\r\n", fret);
		return -1;
	} else {
		debug("FATFS: mount OK\r\n");
	}

	info("FATFS: read %s addr=%x\r\n", image->of_filename, (unsigned int)image->of_dest);
	ret = fatfs_loadimage(image->of_filename, image->of_dest);
	if (ret)
		return ret;

	info("FATFS: read %s addr=%x\r\n", image->filename, (unsigned int)image->dest);
	ret = fatfs_loadimage(image->filename, image->dest);
	if (ret)
		return ret;

	/* umount fs */
	fret = f_mount(0, "", 0);
	if (fret != FR_OK) {
		error("FATFS: unmount error %d\r\n", fret);
		return -1;
	} else {
		debug("FATFS: unmount OK\r\n");
	}
	debug("FATFS: done in %" PRIu32 "ms\r\n", time_ms() - start);

	return 0;
}

#endif

#ifdef CONFIG_BOOT_SPINAND
int load_spi_nand(sunxi_spi_t *spi, image_info_t *image)
{
	linux_zimage_header_t *hdr;
	unsigned int		   size;
	uint64_t UNUSED_DEBUG  start, time;

	if (spi_nand_detect(spi) != 0)
		return -1;

	/* get dtb size and read */
	spi_nand_read(spi, image->of_dest, CONFIG_SPINAND_DTB_ADDR, (uint32_t)sizeof(boot_param_header_t));

	if (of_get_magic_number(image->of_dest) != OF_DT_MAGIC) {
		error("SPI-NAND: DTB verification failed\r\n");
		return -1;
	}

	size = of_get_dt_total_size(image->of_dest);
	debug("SPI-NAND: dt blob: Copy from 0x%08x to 0x%08lx size:0x%08x\r\n", CONFIG_SPINAND_DTB_ADDR,
		  (uint32_t)image->of_dest, size);
	start = time_us();
	spi_nand_read(spi, image->of_dest, CONFIG_SPINAND_DTB_ADDR, (uint32_t)size);

	time = time_us() - start;
	info("SPI-NAND: read dt blob of size %u at %.2fMB/S\r\n", size, (f32)(size / time));

	/* get kernel size and read */
	spi_nand_read(spi, image->dest, CONFIG_SPINAND_KERNEL_ADDR, (uint32_t)sizeof(linux_zimage_header_t));

	hdr = (linux_zimage_header_t *)image->dest;
	if (hdr->magic != LINUX_ZIMAGE_MAGIC) {
		debug("SPI-NAND: zImage verification failed\r\n");
		return -1;
	}
	size = hdr->end - hdr->start;
	debug("SPI-NAND: Image: Copy from 0x%08x to 0x%08lx size:0x%08x\r\n", CONFIG_SPINAND_KERNEL_ADDR,
		  (uint32_t)image->dest, size);
	start = time_us();
	spi_nand_read(spi, image->dest, CONFIG_SPINAND_KERNEL_ADDR, (uint32_t)size);

	time = time_us() - start;
	info("SPI-NAND: read Image of size %u at %.2fMB/S\r\n", size, (f32)(size / time));

	return 0;
}
#endif

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
