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
#ifndef _GTOP5_H_
#define _GTOP5_H_

#include <stdio.h>
#include <ctype.h>

#include "gparam.h"
#include "dbop.h"
#include "gtagsop.h"
#include "idset.h"
#include "strbuf.h"
#include "strhash.h"

GTOP *gtags_open5(GTOP *, const char *, const char *);
void gtags_put5(GTOP *, const char *, const char *);
void gtags_delete5(GTOP *, IDSET *);
const char *gtags_first5(GTOP *, const char *, int);
const char *gtags_next5(GTOP *);
void gtags_close5(GTOP *);

#endif /* ! _GTOP5_H_ */
