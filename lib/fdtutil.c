#include "main.h"

#include "libfdt.h"

static void fdt_dump_property(void *fdt, int propoffset, int depth) {
	int propsize;
	const char *propname;
	const char *value = fdt_getprop_by_offset(fdt, propoffset, &propname, &propsize);

	int is_string = 0;
	if (value[propsize - 1] == '\0') {
		for (int i = 0; i < propsize - 1; i++) {
			if (value[i] != '\0')
				is_string = 1;
			if (value[i] != '\0' && !(value[i] >= 0x20 && value[i] <= 0x7E)) {
				is_string = 0;
				break;
			}
		}
	}

	for (int i = 0; i < depth + 1; i++) {
		putchar('\t');
	}
	message("%s", propname);

	if (is_string) {
		putstr(" = \"");
		for (int i = 0; i < propsize - 1; i++) {
			if (value[i] == '\0')
				putstr("\", \"");
			else
				putchar(value[i]);
		}
		putchar('"');
	} else if (propsize && propsize % 4 == 0) {
		putstr(" = <");
		unsigned int *value32 = (void *)value;
		for (int i = 0; i < propsize / 4; i++) {
			if (i)
				putchar(' ');
			message("0x%x", swap_uint32(value32[i]));
		}
		putchar('>');
	} else if (propsize) {
		message("...size %d...", propsize);
	}
	putstr(";\r\n");
}

static void fdt_dump_undepth(int depth, int prevdepth) {
	if (depth <= prevdepth) {
		int num = prevdepth - depth + 1;
		for (int j = 0; j < num; j++) {
			for (int i = 0; i < prevdepth; i++) {
				putchar('\t');
			}
			putstr("};\r\n");
			prevdepth--;
		}
	}
}

int fdt_dump(void *fdt, int nodeoffset) {
	int depth	  = 0;
	int prevdepth = 0;

	putstr("/dts-v1/;\r\n");

	while (1) {
		const char *nodename = fdt_get_name(fdt, nodeoffset, NULL);

		putstr("\r\n");
		for (int i = 0; i < depth; i++) {
			putchar('\t');
		}
		putstr((nodename && *nodename) ? nodename : "/");
		putstr(" {\r\n");

		int propoffset;
		fdt_for_each_property_offset(propoffset, fdt, nodeoffset) {
			fdt_dump_property(fdt, propoffset, depth);
		}

		nodeoffset = fdt_next_node(fdt, nodeoffset, &depth);
		if (nodeoffset == -FDT_ERR_NOTFOUND)
			break;
		if (depth < 0)
			break;

		fdt_dump_undepth(depth, prevdepth);
		prevdepth = depth;
	}

	fdt_dump_undepth(0, prevdepth);

	return 0;
}

int fdt_dump_by_path(void *fdt, const char *path) {
	int nodeoffset = fdt_path_offset(fdt, path);
	if (nodeoffset < 0)
		return nodeoffset;
	return fdt_dump(fdt, nodeoffset);
}

int fdt_fixup_bootargs(void *fdt, const char *bootargs) {
	int ret = fdt_path_offset(fdt, "/chosen");
	if (ret < 0)
		goto err;
	int nodeoffset = ret;

	if ((ret = fdt_setprop_string(fdt, nodeoffset, "bootargs", bootargs)) != 0)
		goto err;
	return 0;

err:
	error("FDT: cannot set bootargs, ret: %d\r\n", ret);
	return ret;
}

int fdt_fixup_initrd(void *fdt, unsigned int addr, unsigned int size) {
	int ret = fdt_path_offset(fdt, "/chosen");
	if (ret < 0)
		goto err;
	int nodeoffset = ret;

	if ((ret = fdt_setprop_u32(fdt, nodeoffset, "linux,initrd-start", addr)) != 0)
		goto err;
	if ((ret = fdt_setprop_u32(fdt, nodeoffset, "linux,initrd-end", addr + size)) != 0)
		goto err;
	return 0;

err:
	error("FDT: cannot set initrd, ret: %d\r\n", ret);
	return ret;
}

int fdt_fixup_memory(void *fdt, unsigned int addr, unsigned int size) {
	int ret = fdt_path_offset(fdt, "/memory");
	if (ret < 0)
		goto err;
	int nodeoffset = ret;

	if ((ret = fdt_setprop_string(fdt, nodeoffset, "device_type", "memory")) != 0)
		goto err;

	fdt32_t data[2];
	data[0] = cpu_to_fdt32(addr);
	data[1] = cpu_to_fdt32(size);
	if ((ret = fdt_setprop(fdt, nodeoffset, "reg", data, sizeof(data))) != 0)
		goto err;
	return 0;

err:
	error("FDT: cannot set memory, ret: %d\r\n", ret);
	return ret;
}
