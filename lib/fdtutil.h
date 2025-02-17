#pragma once

#include "main.h"

int fdt_dump(void *fdt, int nodeoffset);
int fdt_dump_by_path(void *fdt, const char *path);
int fdt_fixup_bootargs(void *blob, const char *bootargs);
int fdt_fixup_memory(void *blob, unsigned int addr, unsigned int size);
int fdt_fixup_initrd(void *blob, unsigned int addr, unsigned int size);
