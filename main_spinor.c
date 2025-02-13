#include "main.h"

#include "bootimg.h"
#include "sunxi_dma.h"
#include "sunxi_spi.h"

extern sunxi_spi_t sunxi_spi0;
static unsigned int spinor_addr;

static int spinor_read_func(unsigned int *addr, void *buf, unsigned int len) {
	uint64_t UNUSED_DEBUG start, time;

	start = time_us();
	spi_nor_read(&sunxi_spi0, buf, *addr, len);
	time = time_us() - start;
	info("SPI-NOR: read %u bytes at %.2fMB/S\r\n", len, (f32)(len / time));

	*addr += len;
	return len;
}

int load_spinor(void *kernel_entry, void *kernel_param) {
	dma_init();
	dma_test();
	debug("SPI: init\r\n");
	if (sunxi_spi_init(&sunxi_spi0) != 0) {
		fatal("SPI: init failed\r\n");
	}

	debug("SPI-NOR: detect...\r\n");
	if (spi_nor_detect(&sunxi_spi0) != 0)
		return -1;
	debug("SPI-NOR: detect OK\r\n");

	spinor_addr = CONFIG_BOOT_IMG_ADDR_SPINOR;
	debug("SPI-NOR: boot.img address = 0x%x\r\n", spinor_addr);
	return boot_img_load((boot_img_read_func)spinor_read_func, &spinor_addr, kernel_entry, kernel_param);
}
