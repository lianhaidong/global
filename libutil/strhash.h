/*
 * Copyright (c) 2005 Tama Communications Corporation
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
#ifndef _STRHASH_H
#define _STRHASH_H

#include "obstack.h"
#include "queue.h"

struct sh_entry {
	SLIST_ENTRY(sh_entry) ptr;
	char *name;			/* name:  hash key		*/
	void *value;			/* value: user structure	*/
};

SLIST_HEAD(sh_head, sh_entry);

typedef struct {
	int buckets;			/* number of buckets		*/
	struct sh_head *htab;		/* hash buckets			*/
	struct obstack pool;		/* memory pool for obstack	*/
	void (*freefunc)(void *);
	/*
	 * iterator
	 */
	struct sh_entry *cur_entry;
	int cur_bucket;
} STRHASH;

STRHASH *strhash_open(int, void (*)(void *));
struct sh_entry *strhash_assign(STRHASH *, const char *, int);
struct sh_entry *strhash_first(STRHASH *);
struct sh_entry *strhash_next(STRHASH *);
void strhash_reset(STRHASH *);
void strhash_close(STRHASH *);

#endif /* ! _STRHASH_H */
