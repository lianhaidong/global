/*
 * Copyright (c) 2002 Tama Communications Corporation
 *
 * This file is part of GNU GLOBAL.
 *
 * GNU GLOBAL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * GNU GLOBAL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <sys/stat.h>

#include "die.h"
#include "linetable.h"
#include "strbuf.h"

/* File buffer */
#define EXPAND 1024
static STRBUF *ib;
static char *filebuf;
static int filesize;

/* File pointer */
static char *curp;
static char *endp;

/* Index for line */
static int *linetable;
static int total_lines;
static int active_lines;

static void linetable_put(int, int);
/*
 * linetable_open: load whole of file into memory.
 *
 *	i)	path	path
 *	r)		0: normal
 *			-1: cannot open file.
 */
int
linetable_open(path)
	const char *path;
{
	FILE *ip;
	struct stat sb;
	int lineno, offset;

	if (stat(path, &sb) < 0)
		return -1;
	ib = strbuf_open(sb.st_size);
	if ((ip = fopen(path, "r")) == NULL)
		return -1;
	lineno = offset = 0;
	for (offset = 0;
		(strbuf_fgets(ib, ip, STRBUF_APPEND), offset != strbuf_getlen(ib));
		offset = strbuf_getlen(ib))
	{
		linetable_put(offset, lineno++);
	}
	fclose(ip);
	curp = filebuf = strbuf_value(ib);
	filesize = offset;
	endp = filebuf + filesize;
	/* strbuf_close(ib); */

	return 0;
}
/*
 * linetable_read: read(2) compatible routine for linetable.
 *
 *	io)	buf	read buffer
 *	i)	size	buffer size
 *	r)		==-1: end of file
 *			!=-1: number of bytes actually read
 */
int
linetable_read(buf, size)
	char *buf;
	int size;
{
	int leaved = endp - curp;

	if (leaved <= 0)
		return -1;	/* EOF */
	if (size > leaved)
		size = leaved;
	memcpy(buf, curp, size);
	curp += size;

	return size;
}
/*
 * linetable_put: put a line into table.
 *
 *	i)	offset	offset of the line
 *	i)	lineno	line number of the line
 */
static void
linetable_put(offset, lineno)
	int offset;
	int lineno;
{
	if (!total_lines) {
		total_lines = EXPAND;
		linetable = (int *)malloc(total_lines * sizeof(int));
		if (linetable == NULL)
			die("short of memory");
	}
	if (total_lines <= lineno) {
		total_lines += (lineno > EXPAND) ? lineno : EXPAND;
		linetable = (int *)realloc(linetable, total_lines * sizeof(int));
		if (linetable == NULL)
			die("short of memory");
	}
	if (lineno > active_lines)
		active_lines = lineno;
	linetable[lineno] = offset;
}
/*
 * linetable_get: get a line from table.
 *
 *	i)	lineno	line number of the line
 *	r)		line pointer
 */
char *
linetable_get(lineno)
	int lineno;
{
	if (lineno-- <= 0)
		return NULL;
	if (lineno > active_lines)
		return NULL;
	return filebuf + linetable[lineno];
}
/*
 * linetable_close: close line table.
 */
void
linetable_close()
{
	if (linetable)
		(void)free(linetable);
	linetable = NULL;
	total_lines = active_lines = 0;
	strbuf_close(ib);
}
/*
 * linetable_print: print a line.
 *
 *	i)	op	output file pointer
 *	i)	lineno	line number
 */
void
linetable_print(op, lineno)
	FILE *op;
	int lineno;
{
	char *s = linetable_get(lineno);
	if (s == NULL)
		return;
	while (*s != '\n')
		fputc(*s++, op);
	fputc('\n', op);
}
