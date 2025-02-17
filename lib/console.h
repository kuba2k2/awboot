#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "board.h"
#include "main.h"
#include "sunxi_usart.h"

#define CONSOLE_BUFFER_SIZE 128
#define CONSOLE_COMMAND_NUM 16
#define CONSOLE_ARGS_MAX	16

#define CONSOLE_NO_TIMEOUT 0

#define ASCII_CTRL_C  0x03
#define ASCII_BKSPACE '\b'
#define ASCII_DEL	  0x7f
#define ASCII_CR	  '\r'
#define ASCII_LF	  '\n'

typedef struct {
	char *cmd;
	void (*func)(unsigned char argc, char **argv);
	char *args;
	char *help;
} cmd_t;

typedef struct {
	sunxi_usart_t *usart;
	char cmd[CONSOLE_BUFFER_SIZE];
	char *cmd_ptr;
	cmd_t cmd_tbl[CONSOLE_COMMAND_NUM];
} console_t;

void console_init(sunxi_usart_t *usart);
void console_handler(uint32_t timeout);
void console_add_command(char *cmd, void (*func)(unsigned char argc, char **argv), char *args, char *help);

#endif
