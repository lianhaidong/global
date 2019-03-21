/*
 * Copyright (c) 2005, 2006, 2010 Tama Communications Corporation
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
#ifndef _CONVERT_H_
#define _CONVERT_H_

#include <stdio.h>
#include "gparam.h"
#include "strbuf.h"

#define CONVERT_COLOR		1
#define CONVERT_GREP		2
#define CONVERT_BASIC		4
#define CONVERT_ICASE		8
#define CONVERT_IDUTILS		16
#define CONVERT_PATH		32

typedef struct {
	FILE *op;
	int type;		/**< PATH_ABSOLUTE, PATH_RELATIVE */
	int format;		/**< defined in "format.h" */
	STRBUF *abspath;
	char basedir[MAXPATHLEN];
	int start_point;
	int db;			/**< for gtags-cscope */
	char *tag_for_display;
} CONVERT;

void set_convert_flags(int);
void set_print0(void);
CONVERT *convert_open(int, int, const char *, const char *, const char *, FILE *, int);
void convert_put(CONVERT *, const char *);
void convert_put_path(CONVERT *, const char *, const char *);
void convert_put_using(CONVERT *, const char *, const char *, int, const char *, const char *);
void convert_close(CONVERT *cv);

#endif /* ! _CONVERT_H_ */
