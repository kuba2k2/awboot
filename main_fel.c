#include "main.h"

#include "bootimg.h"

static unsigned int fel_addr;

static int fel_read_func(unsigned int *addr, void *buf, unsigned int len) {
	memcpy(buf, (const void *)*addr, len);
	*addr += len;
	return len;
}

int load_fel(void *kernel_entry, void *kernel_param) {
	fel_addr = CONFIG_BOOT_IMG_ADDR_FEL;
	debug("FEL: boot.img address = 0x%x\r\n", fel_addr);
	return boot_img_load((boot_img_read_func)fel_read_func, &fel_addr, kernel_entry, kernel_param);
}
