// Copyright (C) 2006 Microchip Technology Inc. and its subsidiaries
//
// SPDX-License-Identifier: MIT

#include "string.h"
#include "main.h"

#ifndef ULONG_MAX
#define ULONG_MAX ((unsigned long)(~0L)) /* 0xFFFFFFFF */
#endif

void *memset(void *dst, int val, unsigned long cnt) {
	char *d = (char *)dst;

	while (cnt--)
		*d++ = (char)val;

	return dst;
}

int memcmp(const void *dst, const void *src, unsigned long cnt) {
	const char *d = (const char *)dst;
	const char *s = (const char *)src;
	int r		  = 0;

	while (cnt-- && (r = *d++ - *s++) == 0)
		;

	return r;
}

unsigned long strlen(const char *str) {
	long i = 0;

	while (str[i++] != '\0')
		;

	return i - 1;
}

unsigned long strnlen(const char *str, unsigned long cnt) {
	long i = 0;
	for (i = 0; i < cnt; ++i)
		if (str[i] == '\0')
			break;
	return i;
}

char *strcpy(char *dst, const char *src) {
	char *bak = dst;

	while ((*dst++ = *src++) != '\0')
		;

	return bak;
}

char *strcat(char *dst, const char *src) {
	char *p = dst;

	while (*dst != '\0')
		dst++;

	while ((*dst++ = *src++) != '\0')
		;

	return p;
}

int strcmp(const char *p1, const char *p2) {
	unsigned char c1, c2;

	while (1) {
		c1 = *p1++;
		c2 = *p2++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
	}

	return 0;
}

int strncmp(const char *p1, const char *p2, unsigned long cnt) {
	unsigned char c1, c2;

	while (cnt--) {
		c1 = *p1++;
		c2 = *p2++;

		if (c1 != c2)
			return c1 < c2 ? -1 : 1;

		if (!c1)
			break;
	}

	return 0;
}

char *strchr(const char *s, int c) {
	for (; *s != (char)c; ++s)
		if (*s == '\0')
			return NULL;

	return (char *)s;
}

char *strrchr(const char *s, int c) {
	char *rtnval = 0;
	do {
		if (*s == c)
			rtnval = (char *)s;
	} while (*s++);
	return (rtnval);
}

/* NOTE: This is the simple-minded O(len(s1) * len(s2)) worst-case approach. */

char *strstr(const char *s1, const char *s2) {
	register const char *s = s1;
	register const char *p = s2;

	do {
		if (!*p) {
			return (char *)s1;
			;
		}
		if (*p == *s) {
			++p;
			++s;
		} else {
			p = s2;
			if (!*s) {
				return NULL;
			}
			s = ++s1;
		}
	} while (1);
}

void *memchr(const void *src, int val, unsigned long cnt) {
	char *p = NULL;
	char *s = (char *)src;

	while (cnt) {
		if (*s == val) {
			p = s;
			break;
		}
		s++;
		cnt--;
	}

	return p;
}

void *memmove(void *dst, const void *src, unsigned long cnt) {
	char *p, *s;

	if (dst <= src) {
		p = (char *)dst;
		s = (char *)src;
		while (cnt--)
			*p++ = *s++;
	} else {
		p = (char *)dst + cnt;
		s = (char *)src + cnt;
		while (cnt--)
			*--p = *--s;
	}

	return dst;
}

void *memcpy(void *dst, const void *src, unsigned long cnt) {
	char *d;
	const char *s;

	struct chunk {
		unsigned long val[2];
	};

	const struct chunk *csrc = (const struct chunk *)src;
	struct chunk *cdst		 = (struct chunk *)dst;

	if (((unsigned long)src & 0xf) == 0 && ((unsigned long)dst & 0xf) == 0) {
		while (cnt >= sizeof(struct chunk)) {
			*cdst++ = *csrc++;
			cnt -= sizeof(struct chunk);
		}
	}

	d = (char *)cdst;
	s = (const char *)csrc;

	while (cnt--)
		*d++ = *s++;

	return dst;
}

int isdigit(int c) {
	return (unsigned)c - '0' < 10;
}

int isalpha(int c) {
	return ((unsigned)c | 32) - 'a' < 26;
}

int isupper(int c) {
	return (unsigned)c - 'A' < 26;
}

int isspace(int c) {
	return c == ' ' || (unsigned)c - '\t' < 5;
}

unsigned long strtoul(const char *nptr, char **endptr, register int base) {
	register const char *s = nptr;
	register unsigned long acc;
	register int c;
	register unsigned long cutoff;
	register int neg = 0, any, cutlim;

	/*
	 * See strtol for comments as to the logic used.
	 */
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c	= *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;
	cutoff = (unsigned long)ULONG_MAX / (unsigned long)base;
	cutlim = (unsigned long)ULONG_MAX % (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = ULONG_MAX;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *)(any ? s - 1 : nptr);
	return (acc);
}
