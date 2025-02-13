#include "main.h"

#include "board.h"
#include "debug.h"
#include "sunxi_usart.h"

extern sunxi_usart_t USART_DBG;

void message(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	xvformat(sunxi_usart_putc, &USART_DBG, fmt, args);
	va_end(args);
}
