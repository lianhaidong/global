/*
 * Copyright (c) 2014
 *	Tama Communications Corporation
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

#ifndef _REWRITE_H
#define _REWRITE_H

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "regex.h"
#include "strbuf.h"

#define REWRITE_LEFT 0
#define REWRITE_CENTER 1
#define REWRITE_RIGHT 2
#define REWRITE_PARTS 3

typedef struct _rewrite {
	char *pattern;
	char *replace;
	regex_t reg;
	char *part[REWRITE_PARTS];
	int len[REWRITE_PARTS];
} REWRITE;

REWRITE *rewrite_open(const char *, const char *, int);
int rewrite_pattern(REWRITE *, const char *, int);
const char *rewrite_string(REWRITE *, const char *, int);
void rewrite_cancel(REWRITE *);
void rewrite_dump(REWRITE *);
void rewrite_close(REWRITE *);

#endif /* ! _REWRITE_H */
