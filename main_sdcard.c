#include "main.h"

#include "bootimg.h"
#include "ff.h"
#include "libfdt.h"
#include "sdmmc.h"
#include "sunxi_sdhci.h"

#define CHUNK_SIZE 0x20000

static int sdcard_read_func(FIL *file, unsigned char *buf, unsigned int len) {
	UINT byte_to_read;
	UINT byte_read;
	UINT total_read = 0;
	FRESULT fret;
	uint32_t UNUSED_DEBUG start, time;

	start = time_ms();

	do {
		byte_to_read = len ? min(CHUNK_SIZE, len - total_read) : CHUNK_SIZE;
		byte_read	 = 0;
		fret		 = f_read(file, (void *)buf, byte_to_read, &byte_read);
		buf += byte_to_read;
		total_read += byte_read;
	} while (byte_read >= byte_to_read && (len == 0 || total_read < len) && fret == FR_OK);

	time = time_ms() - start + 1;

	if (fret != FR_OK) {
		error("FATFS: read: error %d\r\n", fret);
		f_close(file);
		return -1;
	}

	debug(
		"FATFS: read %u bytes in %" PRIu32 "ms at %.2fMB/S\r\n",
		total_read,
		time,
		(f32)(total_read / time) / 1024.0f
	);

	return total_read;
}

static int sdcard_open(FIL *file, const char *filename) {
	FRESULT fret;

	fret = f_open(file, filename, FA_OPEN_EXISTING | FA_READ);
	if (fret != FR_OK) {
		warning("FATFS: open, filename: [%s]: error %d\r\n", filename, fret);
		return -1;
	}
	debug("FATFS: open OK, filename: [%s]\r\n", filename);
	return 0;
}

static int sdcard_load_boot_img(boot_info_t *boot_info) {
	int ret;
	FIL file;

	if ((ret = sdcard_open(&file, CONFIG_BOOT_IMG_FILENAME)) != 0)
		return ret;

	ret = boot_img_load((boot_img_read_func)sdcard_read_func, &file, boot_info);
	f_close(&file);
	if (ret == 0)
		info("FATFS: loaded %s\r\n", CONFIG_BOOT_IMG_FILENAME);
	return ret;
}

static int sdcard_load_kernel(boot_info_t *boot_info) {
	int ret;
	FIL file;
	if ((ret = sdcard_open(&file, CONFIG_SDMMC_KERNEL_FILENAME)) == 0) {
		unsigned int addr = CONFIG_SDMMC_KERNEL_ADDR;
		int read_len	  = sdcard_read_func(&file, (void *)addr, 0);
		f_close(&file);
		if (read_len <= 0)
			return -1;
		boot_info->kernel_addr = addr;
		boot_info->kernel_size = read_len;
		boot_info->tags_addr   = CONFIG_SDMMC_ATAG_ADDR;
		info("FATFS: loaded %s to 0x%X\r\n", CONFIG_SDMMC_KERNEL_FILENAME, addr);
	}
	return ret;
}

static int sdcard_load_ramdisk(boot_info_t *boot_info) {
	FIL file;
	if (sdcard_open(&file, CONFIG_SDMMC_RAMDISK_FILENAME) == 0) {
		unsigned int addr = boot_info->ramdisk_addr ? boot_info->ramdisk_addr : CONFIG_SDMMC_RAMDISK_ADDR;
		int read_len	  = sdcard_read_func(&file, (void *)addr, 0);
		f_close(&file);
		if (read_len <= 0)
			return -1;
		boot_info->ramdisk_addr = addr;
		boot_info->ramdisk_size = read_len;
		info("FATFS: loaded %s to 0x%X\r\n", CONFIG_SDMMC_RAMDISK_FILENAME, addr);
	}
	return 0;
}

static int sdcard_load_dtb(boot_info_t *boot_info) {
	FIL file;
	if (sdcard_open(&file, CONFIG_SDMMC_DTB_FILENAME) == 0) {
		unsigned int addr = boot_info->dtb_addr ? boot_info->dtb_addr : CONFIG_SDMMC_DTB_ADDR;
		int read_len	  = sdcard_read_func(&file, (void *)addr, 0);
		f_close(&file);
		if (read_len <= 0)
			return -1;
		boot_info->dtb_addr = addr;
		boot_info->dtb_size = read_len;
		info("FATFS: loaded %s to 0x%X\r\n", CONFIG_SDMMC_DTB_FILENAME, addr);
	}
	return 0;
}

static int sdcard_load_cmdline(boot_info_t *boot_info) {
	FIL file;
	if (sdcard_open(&file, CONFIG_SDMMC_CMDLINE_FILENAME) == 0) {
		unsigned int addr = boot_info->cmdline ? (unsigned int)boot_info->cmdline : CONFIG_SDMMC_CMDLINE_ADDR;
		int read_len	  = sdcard_read_func(&file, (void *)addr, 0);
		f_close(&file);
		if (read_len <= 0)
			return -1;
		boot_info->cmdline			 = (void *)addr;
		boot_info->cmdline[read_len] = '\0';

		char *cmdline_end = boot_info->cmdline + strlen(boot_info->cmdline) - 1;
		while (cmdline_end >= boot_info->cmdline) {
			if (*cmdline_end != ' ' && *cmdline_end != '\r' && *cmdline_end != '\n')
				break;
			*cmdline_end-- = '\0';
		}
		info("FATFS: loaded %s to 0x%X\r\n", CONFIG_SDMMC_CMDLINE_FILENAME, addr);
	}
	return 0;
}

#ifdef CONFIG_SDMMC_OVERLAY_SUPPORT
static int sdcard_load_overlay(boot_info_t *boot_info, const char *filename, void *buf) {
	void *fdt = (void *)boot_info->dtb_addr;
	FIL file;
	int ret;
	if (sdcard_open(&file, filename) == 0) {
		int read_len = sdcard_read_func(&file, buf, 0);
		f_close(&file);
		if (read_len <= 0)
			return -1;

		if ((ret = fdt_check_header(buf)) != 0)
			goto err;

		if ((ret = fdt_overlay_apply(fdt, buf)) != 0)
			goto err;
	}
	return 0;

err:
	error("FDT: cannot apply overlay, ret: %d\r\n", ret);
	return ret;
}

static int sdcard_load_overlays(boot_info_t *boot_info) {
	void *fdt = (void *)boot_info->dtb_addr;
	FIL file;
	if (sdcard_open(&file, CONFIG_SDMMC_DTBO_FILENAME) == 0) {
		char *overlays = (void *)CONFIG_LOAD_BUF_ADDR;
		int read_len   = sdcard_read_func(&file, (void *)overlays, 0);
		f_close(&file);
		if (read_len <= 0)
			return -1;
		overlays[read_len] = '\0';
		char *filename	   = overlays + read_len + 1;
		void *buf		   = (void *)((((unsigned int)filename + 256) / 8 + 1) * 8);

		fdt_set_totalsize(fdt, fdt_totalsize(fdt) + 0x1000); // resize to add new nodes
		memcpy(filename, CONFIG_SDMMC_DTBO_DIRNAME "/", sizeof(CONFIG_SDMMC_DTBO_DIRNAME));

		char *overlay = overlays;
		do {
			char *next = strchr(overlay, '\n');
			if (next) {
				if (*(next - 1) == '\r')
					*(next - 1) = '\0';
				*next++ = '\0';
			}
			if (*overlay == '\0' || *overlay == '#') {
				if (!next)
					break;
				overlay = next;
				continue;
			}

			strcpy(filename + sizeof(CONFIG_SDMMC_DTBO_DIRNAME), overlay);
			info("FDT: applying device tree overlay '%s'\r\n", filename);

			int ret;
			if ((ret = sdcard_load_overlay(boot_info, filename, buf)) != 0)
				return ret;

			overlay = next;
		} while (overlay);

		fdt_pack(fdt);
	}
	return 0;
}
#endif

int load_sdcard(boot_info_t *boot_info) {
	int ret;
	FATFS fs;
	FRESULT fret;
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
	debug(
		"SDMMC: speedtest %uKB in %" PRIu32 "ms at %" PRIu32 "KB/S\r\n",
		(CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024,
		test_time,
		(CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time
	);
#endif // SDMMC_SPEED_TEST

	start = time_ms();

	// mount filesystem
	fret = f_mount(&fs, "", 1);
	if (fret != FR_OK) {
		error("FATFS: mount error: %d\r\n", fret);
		return -3;
	}
	debug("FATFS: mount OK\r\n");

	// try boot.img first
	if ((ret = sdcard_load_boot_img(boot_info)) == 0)
		goto _load_ok;

	// try zImage if boot.img fails
	if ((ret = sdcard_load_kernel(boot_info)) == 0)
		goto _load_ok;

	// exit if all load methods fail
	goto _out;

_load_ok:
#ifndef CONFIG_SDMMC_OVERRIDE_RAMDISK
	if (boot_info->ramdisk_addr == 0)
#endif
		// override ramdisk from file
		if ((ret = sdcard_load_ramdisk(boot_info)) != 0)
			goto _out;

#ifndef CONFIG_SDMMC_OVERRIDE_DTB
	if (boot_info->dtb_addr == 0)
#endif
		// override dtb from file
		if ((ret = sdcard_load_dtb(boot_info)) != 0)
			goto _out;

#ifndef CONFIG_SDMMC_OVERRIDE_CMDLINE
	if (boot_info->cmdline == NULL)
#endif
		// override cmdline from file
		if ((ret = sdcard_load_cmdline(boot_info)) != 0)
			goto _out;

#ifdef CONFIG_SDMMC_OVERLAY_SUPPORT
	if (boot_info->dtb_addr != 0)
		// load device tree overlays
		if ((ret = sdcard_load_overlays(boot_info)) != 0)
			goto _out;
#endif

_out:
	// unmount filesystem
	fret = f_mount(0, "", 0);
	if (fret != FR_OK) {
		error("FATFS: unmount error %d\r\n", fret);
		return -5;
	}
	debug("FATFS: unmount OK\r\n");

	if (ret == 0)
		info("FATFS: done in %" PRIu32 "ms\r\n", time_ms() - start);
	else
		warning("FATFS: loading failed\r\n");

	return ret;
}
