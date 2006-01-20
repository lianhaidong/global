/*
 * Copyright (c) 2005, 2006 Tama Communications Corporation
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 */
#ifndef _TAGSORT_H_
#define _TAGSORT_H_

#include <stdio.h>
#include "dbop.h"
#include "format.h"
#include "strbuf.h"
#include "varray.h"

typedef struct {
	/*
	 * Common area.
	 */
	int format;				/* format type */
	int unique;				/* 1: make the output unique */
	FILE *op;				/* output file */
	/*
	 * for FORMAT_PATH
	 */
	DBOP *dbop;
	/*
	 * for FORMAT_CTAGS/FORMAT_CTAGS_X
	 */
	STRBUF *sb;
	VARRAY *vb;
	char prev[IDENTLEN];
} TAGSORT;

TAGSORT *tagsort_open(FILE *, int, int);
void tagsort_put(TAGSORT *, char *);
void tagsort_close(TAGSORT *);

#endif /* ! _TAGSORT_H_ */
