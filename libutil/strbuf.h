/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2002
 *	Tama Communications Corporation
 *
 * This file is part of GNU GLOBAL.
 *
 * GNU GLOBAL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GNU GLOBAL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _STRBUF_H
#define _STRBUF_H

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef HAVE_STDARG_H 
#include <stdarg.h>
#else
#include <varargs.h>
#endif

/* #define STRBUF_LINK */

#define INITIALSIZE 80
#define EXPANDSIZE 80

/* for strbuf_fgets() */
#define STRBUF_APPEND	1
#define STRBUF_NOCRLF	2

typedef struct _strbuf {
#ifdef STRBUF_LINK
	struct _strbuf *next;
	struct _strbuf *prev;
#endif
	char *name;
	char *sbuf;
	char *endp;
	char *curp;
	int sbufsize;
	int alloc_failed;
} STRBUF;

#define strbuf_putc(sb, c)	do {\
	if (!sb->alloc_failed) {\
		if (sb->curp >= sb->endp)\
			__strbuf_expandbuf(sb, 0);\
		*sb->curp++ = c;\
	}\
} while (0)
#define strbuf_nputs(sb, s, len) do {\
	unsigned int _length = len;\
	if (!sb->alloc_failed) {\
		if (sb->curp + _length > sb->endp)\
			__strbuf_expandbuf(sb, _length);\
		strncpy(sb->curp, s, _length);\
		sb->curp += _length;\
	}\
} while (0)
#define strbuf_puts(sb, s) strbuf_nputs(sb, s, strlen(s))
#define strbuf_puts0(sb, s) strbuf_nputs(sb, s, strlen(s) + 1)

#define strbuf_reset(sb) do {\
	sb->curp = sb->sbuf;\
	sb->alloc_failed = 0;\
} while (0)

#define strbuf_getlen(sb) (sb->curp - sb->sbuf)
#define strbuf_setlen(sb, len) do {\
	unsigned int _length = len;\
	if (!sb->alloc_failed) {\
		if (_length < strbuf_getlen(sb))\
			sb->curp = sb->sbuf + _length;\
		else if (_length > strbuf_getlen(sb))\
			__strbuf_expandbuf(sb, _length - strbuf_getlen(sb));\
	}\
} while (0)
#define strbuf_lastchar(sb) (*(sb->curp - 1))

#ifdef DEBUG
void strbuf_dump(char *);
#endif
void __strbuf_expandbuf(STRBUF *, int);
STRBUF *strbuf_open(int);
void strbuf_putn(STRBUF *, int);
int strbuf_unputc(STRBUF *, int);
char *strbuf_value(STRBUF *);
void strbuf_trim(STRBUF *);
void strbuf_close(STRBUF *);
char *strbuf_fgets(STRBUF *, FILE *, int);
#ifdef HAVE_STDARG_H
void strbuf_sprintf(STRBUF *sb, const char *s, ...);
#else
void strbuf_sprintf();
#endif
#ifdef STRBUF_LINK
void strbuf_setname(STRBUF *, char *);
STRBUF *strbuf_getbuf(char *);
void strbuf_closeall();
#endif

#endif /* ! _STRBUF_H */
