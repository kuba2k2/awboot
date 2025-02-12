#include "bootimg.h"

#include "main.h"
#include "atag.h"
#include "dram.h"

#define READ(buf, len)                            \
	do {                                          \
		if (func(param, (buf), (len)) != (len)) { \
			debug("BOOTIMG: read failed\r\n");    \
			return -1;                            \
		}                                         \
	} while (0)

int boot_img_load(boot_img_read_func func, void *param, unsigned int *kernel_entry, unsigned int *kernel_param)
{
	boot_img_hdr   hdr;
	unsigned char *buf;
	unsigned int   page_size = CONFIG_BOOT_IMG_BUF_SIZE;
	unsigned int   kernel_pages, ramdisk_pages, second_pages;

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
	buf		  = (void *)CONFIG_BOOT_IMG_BUF_ADDR;
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

	// setup atags
	setup_core_tag((void *)hdr.tags_addr, 4096);
	setup_mem_tag(SDRAM_BASE, sdram_size);
	setup_ramdisk_tag(16 * 1024);
	setup_initrd2_tag(hdr.ramdisk_addr, hdr.ramdisk_size);
	setup_cmdline_tag((const char *)hdr.cmdline);
	setup_end_tag();

	// write outputs
	*kernel_entry = hdr.kernel_addr;
	*kernel_param = hdr.tags_addr;

	info("BOOTIMG: kernel @ 0x%x, atags @ 0x%x\r\n", *kernel_entry, *kernel_param);

	return 0;
}
