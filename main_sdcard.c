#include "main.h"

#include "bootimg.h"
#include "sunxi_sdhci.h"
#include "ff.h"
#include "sdmmc.h"

#define CHUNK_SIZE 0x20000

static int sdcard_read_func(FIL *file, unsigned char *buf, unsigned int len)
{
	UINT				  byte_to_read;
	UINT				  byte_read;
	UINT				  total_read = 0;
	FRESULT				  fret;
	uint32_t UNUSED_DEBUG start, time;

	start = time_ms();

	do {
		byte_to_read = min(CHUNK_SIZE, len - total_read);
		byte_read	 = 0;
		fret		 = f_read(file, (void *)buf, byte_to_read, &byte_read);
		buf += byte_to_read;
		total_read += byte_read;
	} while (byte_read >= byte_to_read && total_read < len && fret == FR_OK);

	time = time_ms() - start + 1;

	if (fret != FR_OK) {
		error("FATFS: read: error %d\r\n", fret);
		f_close(file);
		return -1;
	}

	debug("FATFS: read %u bytes in %" PRIu32 "ms at %.2fMB/S\r\n", total_read, time,
		  (f32)(total_read / time) / 1024.0f);

	return total_read;
}

int load_sdcard(void *kernel_entry, void *kernel_param)
{
	int				 ret;
	FATFS			 fs;
	FIL				 file;
	FRESULT			 fret;
	u32 UNUSED_DEBUG start;

	if (sunxi_sdhci_init(&sdhci0) != 0) {
		warning("SMHC: %s controller init failed\r\n", sdhci0.name);
		return -1;
	}
	info("SMHC: %s controller v%" PRIx32 " initialized\r\n", sdhci0.name, sdhci0.reg->vers);

	if (sdmmc_init(&card0, &sdhci0) != 0) {
		warning("SMHC: init failed\r\n");
		return -2;
	}

#if defined(CONFIG_SDMMC_SPEED_TEST_SIZE) && LOG_LEVEL >= LOG_DEBUG
	u32 test_time;
	start = time_ms();
	sdmmc_blk_read(&card0, (u8 *)(SDRAM_BASE), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
	test_time = time_ms() - start;
	debug("SDMMC: speedtest %uKB in %" PRIu32 "ms at %" PRIu32 "KB/S\r\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024,
		  test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);
#endif // SDMMC_SPEED_TEST

	start = time_ms();

	// mount filesystem
	fret = f_mount(&fs, "", 1);
	if (fret != FR_OK) {
		error("FATFS: mount error: %d\r\n", fret);
		return -3;
	}
	debug("FATFS: mount OK\r\n");

	// open file
	fret = f_open(&file, CONFIG_BOOT_IMG_FILENAME, FA_OPEN_EXISTING | FA_READ);
	if (fret != FR_OK) {
		error("FATFS: open, filename: [%s]: error %d\r\n", CONFIG_BOOT_IMG_FILENAME, fret);
		return -4;
	}
	debug("FATFS: open OK\r\n");

	// load boot.img
	ret = boot_img_load((boot_img_read_func)sdcard_read_func, &file, kernel_entry, kernel_param);
	if (ret != 0) {
		warning("SMHC: loading failed\r\n");
		return -6;
	}

	// unmount filesystem
	fret = f_mount(0, "", 0);
	if (fret != FR_OK) {
		error("FATFS: unmount error %d\r\n", fret);
		return -5;
	}
	debug("FATFS: unmount OK\r\n");

	if (ret == 0)
		debug("FATFS: done in %" PRIu32 "ms\r\n", time_ms() - start);
	else
		warning("SMHC: loading failed\r\n");

	return 0;
}
