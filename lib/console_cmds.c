#include "main.h"

#include "arm32.h"
#include "console.h"

#ifdef CONFIG_ENABLE_CONSOLE

extern console_t console;

static void cmd_echo(unsigned char argc, char **argv) {
	message("%s\r\n", argv[1]);
}

static void cmd_help(unsigned char argc, char **argv) {
	int i;
	message("Supported commands:\r\n");
	for (i = 0; i < CONSOLE_COMMAND_NUM && console.cmd_tbl[i].cmd != NULL; i++) {
		message(" - %s %s - %s\r\n", console.cmd_tbl[i].cmd, console.cmd_tbl[i].args, console.cmd_tbl[i].help);
	}
}

static void cmd_go(unsigned char argc, char **argv) {
	argc--, argv++;

	unsigned int addr = strtoul(argv[0], NULL, 0);
	argc--, argv++;

	unsigned int r[4] = {0, 0, 0, 0};
	for (int i = 0; i < min(argc, 4); i++) {
		r[i] = strtoul(argv[i], NULL, 0);
	}

	info("Starting application @ 0x%X\r\n", addr);
	info("r0 = 0x%X, r1 = 0x%X, r2 = 0x%X, r3 = 0x%X\r\n", r[0], r[1], r[2], r[3]);

	arm32_mmu_disable();
	arm32_dcache_disable();
	arm32_icache_disable();
	arm32_interrupt_disable();

	((void (*)(unsigned int, unsigned int, unsigned int, unsigned int))addr)(r[0], r[1], r[2], r[3]);
}

void console_register_commands() {
	console_add_command("echo", cmd_echo, "<text>", "echo text");
	console_add_command("help", cmd_help, "", "show this help");
	console_add_command("go", cmd_go, "<addr> [r0] [r1] [r2] [r3]", "execute code at address");
}

#endif
