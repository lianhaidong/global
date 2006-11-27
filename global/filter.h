/*
 * Copyright (c) 2006
 *	Tama Communications Corporation
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
#ifndef _FILTER_H
#define _FILTER_H

#include "strbuf.h"

#define SORT_FILTER     1
#define PATH_FILTER     2
#define BOTH_FILTER     (SORT_FILTER|PATH_FILTER)

const char *getsortfilter(void);
const char *getpathfilter(void);
void setup_sortfilter(int, int, int);
void setup_pathfilter(int, int, const char *, const char *, const char *);
void makesortfilter(int, int, int);
void makepathfilter(int, int, const char *, const char *, const char *);
void makefilter(STRBUF *);
void filter_open(void);
void filter_put(const char *);
void filter_close(void);

#endif /* ! _FILTER_H */
