/*
 * Copyright(C) Caldera International Inc.  2001-2002.  All rights reserved.
 *
 * See 'Caldera Ancient Unix License' in 'LICENSE' file.
 *
 * Copyright (c) 2012 Tama Communications Corporation
 *
 * This file is part of GNU GLOBAL.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * This code was derived from /usr/src/usr.bin/fgrep.c in 4.3 Berkeley
 * Software Distribution (4.3BSD) by the University of California.
 * Since GNU grep is very strong, I would like to include its code into
 * global command in near future.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_MMAP
#include <sys/mman.h>
#elif _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "format.h"
#include "convert.h"
#include "die.h"
#include "strlimcpy.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

void overflo(void);
void cgotofn(const char *);
void cfail(void);

extern int iflag;
extern int Vflag;
extern void encode(char *, int, const char *);

#define	MAXSIZ 6000
#define QSIZE 400
static struct words {
	char 	inp;
	char	out;
	struct	words *nst;
	struct	words *link;
	struct	words *fail;
} w[MAXSIZ], *smax, *q;

static char pattern[IDENTLEN];
static char encoded_pattern[IDENTLEN];

/**
 * literal_comple: compile literal for search.
 *
 *	@param[in]	pattern	literal string
 *
 * Literal string is treated as is.
 */
void
literal_comple(const char *pat)
{
	/*
	 * convert spaces into %FF format.
	 */
	encode(encoded_pattern, sizeof(encoded_pattern), pat);
	strlimcpy(pattern, pat, sizeof(pattern));
	/*
	 * construct a goto table.
	 */
	cgotofn(pattern);
	/*
	 * construct fail links.
	 */
	cfail();
}
/**
 * literal_search: execute literal search
 *
 *	@param[in]	file	file to search
 *	@return		0: normal, -1: error
 */
# define ccomp(a,b) (iflag ? lca(a)==lca(b) : a==b)
# define lca(x) (isupper(x) ? tolower(x) : x)
int
literal_search(CONVERT *cv, const char *file)
{
	struct words *c;
	int ccount;
	char *p;
	char *buf;
	struct stat stb;
	char *linep;
	long lineno;
	int f;
	int count = 0;

	if ((f = open(file, O_BINARY)) < 0) {
		warning("cannot open '%s'.", file);
		return -1;
	}
	if (fstat(f, &stb) < 0) {
		warning("cannot fstat '%s'.", file);
		goto skip_empty_file;
	}
	if (stb.st_size == 0)
		goto skip_empty_file;
#ifdef HAVE_MMAP
	buf = mmap(0, stb.st_size, PROT_READ, MAP_SHARED, f, 0);
	if (buf == MAP_FAILED)
		die("mmap failed (%s).", file);
#elif _WIN32
	{
	HANDLE hMap = CreateFileMapping((HANDLE)_get_osfhandle(f), NULL, PAGE_READONLY, 0, stb.st_size, NULL);
	if (hMap == NULL)
		die("CreateFileMapping failed (%s).", file);
	buf = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if (buf == NULL)
		die("MapViewOfFile failed (%s).", file);
#else
#ifdef HAVE_ALLOCA
	buf = (char *)alloca(stb.st_size);
#else
	buf = (char *)malloc(stb.st_size);
#endif
	if (buf == NULL)
		die("short of memory.");
	if (read(f, buf, stb.st_size) < stb.st_size)
		die("read failed (%s).", file);
#endif
	linep = p = buf;
	ccount = stb.st_size;
	lineno = 1;
	c = w;
	for (;;) {
		if (--ccount < 0)
			break;
		nstate:
			if (ccomp(c->inp, *p)) {
				c = c->nst;
			}
			else if (c->link != 0) {
				c = c->link;
				goto nstate;
			}
			else {
				c = c->fail;
				if (c==0) {
					c = w;
					istate:
					if (ccomp(c->inp , *p)) {
						c = c->nst;
					}
					else if (c->link != 0) {
						c = c->link;
						goto istate;
					}
				}
				else goto nstate;
			}
		if (c->out) {
			while (*p++ != '\n') {
				if (--ccount < 0)
					break;
			}
			if (Vflag)
				goto nomatch;
	succeed:	if (cv->format == FORMAT_PATH) {
				convert_put_path(cv, NULL, file);
				count++;
				goto finish;
			} else {
				STATIC_STRBUF(sb);

				strbuf_clear(sb);
				strbuf_nputs(sb, linep, p - linep);
				strbuf_unputc(sb, '\n');
				strbuf_unputc(sb, '\r');
				convert_put_using(cv, pattern, file, lineno, strbuf_value(sb), NULL);
				count++;
			}
	nomatch:	lineno++;
			linep = p;
			c = w;
			continue;
		}
		if (*p++ == '\n' || ccount == 0) {
			if (Vflag)
				goto succeed;
			else {
				lineno++;
				linep = p;
				c = w;
			}
		}
	}
finish:
#ifdef HAVE_MMAP
	munmap(buf, stb.st_size);
#elif _WIN32
	UnmapViewOfFile(buf);
	CloseHandle(hMap);
	}
#elif HAVE_ALLOCA
#else
	free(buf);
#endif
skip_empty_file:
	close(f);
	return count;
}
/**
 * make automaton.
 */
void
cgotofn(const char *pattern) {
	int c;
	struct words *s;

	s = smax = w;
nword:	for(;;) {
		c = *pattern++;
		if (c==0)
			return;
		if (c == '\n') {
			s->out = 1;
			s = w;
		} else {
		loop:	if (s->inp == c) {
				s = s->nst;
				continue;
			}
			if (s->inp == 0) goto enter;
			if (s->link == 0) {
				if (smax >= &w[MAXSIZ - 1])
					overflo();
				s->link = ++smax;
				s = smax;
				goto enter;
			}
			s = s->link;
			goto loop;
		}
	}
	enter:
	do {
		s->inp = c;
		if (smax >= &w[MAXSIZ - 1])
			overflo();
		s->nst = ++smax;
		s = smax;
	} while ((c = *pattern++) != '\n' && c!=0);
	smax->out = 1;
	s = w;
	if (c != 0)
		goto nword;
}

void
cfail() {
	struct words *queue[QSIZE];
	struct words **front, **rear;
	struct words *state;
	int bstart;
	char c;
	struct words *s;

	s = w;
	front = rear = queue;
init:	if ((s->inp) != 0) {
		*rear++ = s->nst;
		if (rear >= &queue[QSIZE - 1])
			overflo();
	}
	if ((s = s->link) != 0) {
		goto init;
	}

	while (rear != front) {
		s = *front;
		if (front == &queue[QSIZE-1])
			front = queue;
		else front++;
	cloop:	if ((c = s->inp) != 0) {
			bstart = 0;
			*rear = (q = s->nst);
			if (front < rear)
				if (rear >= &queue[QSIZE-1])
					if (front == queue) overflo();
					else rear = queue;
				else rear++;
			else
				if (++rear == front) overflo();
			state = s->fail;
		floop:	if (state == 0) {
				state = w;
				bstart = 1;
			}
			if (state->inp == c) {
			qloop:	q->fail = state->nst;
				if ((state->nst)->out == 1)
					q->out = 1;
				if ((q = q->link) != 0)
					goto qloop;
			}
			else if ((state = state->link) != 0)
				goto floop;
			else if (bstart == 0){
				state = 0;
				goto floop;
			}
		}
		if ((s = s->link) != 0)
			goto cloop;
	}
}
void
overflo() {
	die("wordlist too large.");
}
