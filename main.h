#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "types.h"

#include "board.h"
#include "debug.h"
#include "io.h"
#include "string.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#define ALIGN(size, align) (((size) + (align) - 1) & (~((align) - 1)))
#define OF_ALIGN(size)	   ALIGN(size, 4)

#define STRINGIFY(x)	   #x
#define STRINGIFY_MACRO(x) STRINGIFY(x)

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#ifndef NULL
#define NULL 0
#endif

#define FALSE 0
#define TRUE  1

extern unsigned long sdram_size;

static inline unsigned int swap_uint32(unsigned int data) {
	volatile unsigned int a, b, c, d;

	a = ((data) & 0xff000000) >> 24;
	b = ((data) & 0x00ff0000) >> 8;
	c = ((data) & 0x0000ff00) << 8;
	d = ((data) & 0x000000ff) << 24;

	return a | b | c | d;
}

/* Linux zImage Header */
#define LINUX_ZIMAGE_MAGIC 0x016f2818

typedef struct {
	unsigned int code[9];
	unsigned int magic;
	unsigned int start;
	unsigned int end;
} linux_zimage_header_t;

typedef struct boot_info_t {
	unsigned int kernel_addr;
	unsigned int kernel_size;
	unsigned int ramdisk_addr;
	unsigned int ramdisk_size;
	unsigned int dtb_addr;
	unsigned int dtb_size;
	unsigned int tags_addr;
	char *cmdline;
} boot_info_t;

typedef void (*kernel_entry_t)(int zero, int arch, unsigned int params);

void udelay(uint64_t us);
void mdelay(uint32_t ms);
void sdelay(uint32_t loops);
uint32_t time_ms(void);
uint64_t time_us(void);
void reset();
void reset_cpu();

int load_fel(boot_info_t *boot_info);
int load_sdcard(boot_info_t *boot_info);
int load_spinand(boot_info_t *boot_info);
int load_spinor(boot_info_t *boot_info);

#endif
