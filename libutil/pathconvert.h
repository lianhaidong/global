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
#ifndef _PATHCONVERT_H_
#define _PATHCONVERT_H_

typedef struct {
	FILE *op;
	int type;		/* PATH_ABSOLUTE, PATH_RELATIVE */
	int format;		/* defined in format.h */
	int fileid;		/* 1: fileid */
	STRBUF *abspath;
	char basedir[MAXPATHLEN+1];
	int start_point;

} CONVERT;

CONVERT *convert_open(int, int, int, const char *, const char *, const char *, FILE *);
void convert_put(CONVERT *cv, const char *line);
void convert_close(CONVERT *cv);

#endif /* ! _PATHCONVERT_H_ */
