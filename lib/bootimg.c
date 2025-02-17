#include "bootimg.h"

#include "main.h"

#include "dram.h"

#define READ(buf, len)                            \
	do {                                          \
		if (func(param, (buf), (len)) != (len)) { \
			debug("BOOTIMG: read failed\r\n");    \
			return -1;                            \
		}                                         \
	} while (0)

int boot_img_load(boot_img_read_func func, void *param, boot_info_t *boot_info) {
	boot_img_hdr hdr;
	unsigned char *buf;
	unsigned int page_size = CONFIG_LOAD_BUF_SIZE;
	unsigned int kernel_pages, ramdisk_pages, second_pages;

	READ(&hdr, sizeof(hdr));

	if (memcmp(hdr.magic, BOOT_MAGIC, sizeof(hdr.magic)) != 0) {
		debug("BOOTIMG: header magic invalid\r\n");
		return -2;
	}

	if (hdr.page_size > page_size) {
		debug("BOOTIMG: page size too large\r\n");
		return -3;
	}

	// skip the 1st page
	buf		  = (void *)CONFIG_LOAD_BUF_ADDR;
	page_size = hdr.page_size;
	memset(buf, 0, page_size);
	READ(buf, page_size - sizeof(hdr));

	kernel_pages  = (hdr.kernel_size + page_size - 1) / page_size;
	ramdisk_pages = (hdr.ramdisk_size + page_size - 1) / page_size;
	second_pages  = (hdr.second_size + page_size - 1) / page_size;

	info("BOOTIMG: found '%s'\r\n", hdr.name);
	info(" - kernel: 0x%x (0x%x)\r\n", hdr.kernel_addr, hdr.kernel_size);
	info(" - ramdisk: 0x%x (0x%x)\r\n", hdr.ramdisk_addr, hdr.ramdisk_size);
	info(" - second: 0x%x (0x%x)\r\n", hdr.second_addr, hdr.second_size);
	info(" - tags: 0x%x\r\n", hdr.tags_addr);
	info(" - page: 0x%x\r\n", hdr.page_size);
	info(" - cmdline: %s\r\n", hdr.cmdline);

	// read kernel, ramdisk, second
	if (kernel_pages)
		READ((void *)hdr.kernel_addr, kernel_pages * page_size);
	if (ramdisk_pages)
		READ((void *)hdr.ramdisk_addr, ramdisk_pages * page_size);
	if (second_pages)
		READ((void *)hdr.second_addr, second_pages * page_size);
	// copy command line
	memcpy((void *)CONFIG_SDMMC_CMDLINE_ADDR, hdr.cmdline, sizeof(hdr.cmdline));

	info("BOOTIMG: load successful\r\n");

	// write outputs
	if (kernel_pages)
		boot_info->kernel_addr = hdr.kernel_addr;
	if (ramdisk_pages)
		boot_info->ramdisk_addr = hdr.ramdisk_addr;
	if (second_pages)
		boot_info->dtb_addr = hdr.second_addr;
	boot_info->kernel_size	= hdr.kernel_size;
	boot_info->ramdisk_size = hdr.ramdisk_size;
	boot_info->dtb_size		= hdr.second_size;
	boot_info->tags_addr	= hdr.tags_addr;
	boot_info->cmdline		= (void *)CONFIG_SDMMC_CMDLINE_ADDR;

	return 0;
}
