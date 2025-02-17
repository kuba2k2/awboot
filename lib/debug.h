#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "xformat.h"

#define LOG_ERROR 10
#define LOG_WARN  20
#define LOG_INFO  30
#define LOG_DEBUG 40
#define LOG_TRACE 50

#define ANSI_RESET	 "\x1B[0m"
#define ANSI_CYAN	 "\x1B[36m"
#define ANSI_BLUE	 "\x1B[34m"
#define ANSI_GREEN	 "\x1B[32m"
#define ANSI_YELLOW	 "\x1B[33m"
#define ANSI_RED	 "\x1B[31m"
#define ANSI_MAGENTA "\x1B[35m"

#if LOG_LEVEL >= LOG_TRACE
#define trace(fmt, ...) message(ANSI_CYAN "[T] " ANSI_RESET fmt, ##__VA_ARGS__)
#define UNUSED_TRACE
#else
#define trace(...)
#define UNUSED_TRACE __attribute__((__unused__))
#endif

#if LOG_LEVEL >= LOG_DEBUG
#define debug(fmt, ...) message(ANSI_BLUE "[D] " ANSI_RESET fmt, ##__VA_ARGS__)
#define UNUSED_DEBUG
#else
#define debug(...)
#define UNUSED_DEBUG __attribute__((__unused__))
#endif

#if LOG_LEVEL >= LOG_INFO
#define info(fmt, ...) message(ANSI_GREEN "[I] " ANSI_RESET fmt, ##__VA_ARGS__)
#define UNUSED_INFO
#else
#define info(...)
#define UNUSED_INFO __attribute__((__unused__))
#endif

#if LOG_LEVEL >= LOG_WARNING
#define warning(fmt, ...) message(ANSI_YELLOW "[W] " ANSI_RESET fmt, ##__VA_ARGS__)
#define UNUSED_WARNING
#else
#define warning(...)
#define UNUSED_WARNING __attribute__((__unused__))
#endif

#if LOG_LEVEL >= LOG_ERROR
#define error(fmt, ...) message(ANSI_RED "[E] " ANSI_RESET fmt, ##__VA_ARGS__)
#define UNUSED_ERROR
#else
#define error(...)
#define UNUSED_ERROR __attribute__((__unused__))
#endif

#define fatal(fmt, ...)                                                                 \
	{                                                                                   \
		message(ANSI_MAGENTA "[F] " ANSI_RESET fmt "restarting...\r\n", ##__VA_ARGS__); \
		mdelay(100);                                                                    \
		reset_cpu();                                                                    \
	}

void __attribute__((format(printf, 1, 2))) message(const char *fmt, ...);
void putchar(char c);
void putstr(const char *s);

#endif
