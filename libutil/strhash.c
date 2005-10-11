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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "die.h"
#include "strhash.h"
#include "obstack.h"

/*

String Hash (associative array): usage and memory status

hash = strhash_open(10, free);			// allocate hash buckets.

entry = strhash_assign(hash, "name1", 0);	// get entry for the name.

	entry == NULL				// entry not found.

entry = strhash_assign(hash, "name1", 1);	// allocate entry for the name.
						entry
						+-------------+
						|name: "name1"|
						|value:    *---->(void *)NULL
						+-------------+
// strhash_xxx() doesn't affect entry->value. So, you can use it freely.

entry->value = strdup("NAME1");
						entry
						+-------------+
						|name: "name1"|
						|value:    *---->"NAME1"
						+-------------+

entry = strhash_assign(hash, "name1", 0);	// get entry of the name.

						entry
						+-------------+
						|name: "name1"|
						|value:    *---->"NAME1"
						+-------------+
char *s = (char *)entry->value;

	s == "NAME1"

strhash_close(hash);				// free resources.
						entry

*/

/*
 * check_malloc: memory allocator
 */
static void *check_malloc(int size)
{
        void *p = (void *)malloc(size);
        if (p == NULL)
		die("short of memory.");
        return p;
}
#define obstack_chunk_alloc check_malloc
#define obstack_chunk_free free

/*
 * strhash_open: open string hash table.
 *
 *	i)	buckets	 size of bucket table
 *	i)	freefunc free function for sh->value
 *	r)	sh	STRHASH structure
 */
STRHASH *
strhash_open(int buckets, void (*freefunc)(void *))
{
	STRHASH *sh = (STRHASH *)check_malloc(sizeof(STRHASH));
	int i;

	sh->htab = (struct sh_head *)check_malloc(sizeof(struct sh_head) * buckets);
	for (i = 0; i < buckets; i++)
		SLIST_INIT(&sh->htab[i]);
	sh->freefunc = freefunc;
	sh->buckets = buckets;
	obstack_init(&sh->pool);
	return sh;
}
/*
 * strhash_assign: assign hash entry.
 *
 *	i)	sh	STRHASH structure
 *	i)	name	name
 *	i)	force	if entry not found, create it.
 *	r)		pointer of the entry
 *
 * If specified entry is found then it is returned, else if the force == 1
 * then new allocated entry is returned.
 * This procedure doesn't operate the contents of entry->value.
 */
struct sh_entry *
strhash_assign(STRHASH *sh, const char *name, int force)
{
	struct sh_head *head = &sh->htab[__hash_string(name) % sh->buckets];
	struct sh_entry *entry;

	/*
	 * Lookup the name's entry.
	 */
	SLIST_FOREACH(entry, head, ptr)
		if (strcmp(entry->name, name) == 0)
			break;
	/*
	 * If not found, allocate an entry.
	 */
	if (entry == NULL && force) {
		entry = obstack_alloc(&sh->pool, sizeof(struct sh_entry));
		entry->name = obstack_copy0(&sh->pool, name, strlen(name));
		entry->value = NULL;
		SLIST_INSERT_HEAD(head, entry, ptr);
	}
	return entry;
}
/*
 * strhash_first: get first entry
 *
 *	i)	sh	STRHASH structure
 */
struct sh_entry *
strhash_first(STRHASH *sh)
{
	sh->cur_bucket = -1;		/* to start from index 0. */
	sh->cur_entry = NULL;
	return strhash_next(sh);
}
/*
 * strhash_next: get next entry
 *
 *	i)	sh	STRHASH structure
 */
struct sh_entry *
strhash_next(STRHASH *sh)
{
	struct sh_entry *entry = NULL;

	if (sh->buckets > 0 && sh->cur_bucket < sh->buckets) {
		entry = sh->cur_entry;
		if (entry == NULL) {
			while (++sh->cur_bucket < sh->buckets) {
				entry = SLIST_FIRST(&sh->htab[sh->cur_bucket]);
				if (entry)
					break;
			}
		}
		sh->cur_entry = (entry) ? SLIST_NEXT(entry, ptr) : NULL;
	}
	return entry;
}
/*
 * strhash_reset: reset string hash.
 *
 *	i)	sh	STRHASH structure
 */
void
strhash_reset(STRHASH *sh)
{
	struct sh_entry *entry;
	int i;

	/*
	 * Free user memory if freefunc is specified.
	 */
	if (sh->freefunc)
		for (entry = strhash_first(sh); entry; entry = strhash_next(sh))
			if (entry->value)
				sh->freefunc(entry->value);
	/*
	 * Free and reinitialize entries for each bucket.
	 */
	for (i = 0; i < sh->buckets; i++) {
		SLIST_INIT(&sh->htab[i]);
	}
	obstack_free(&sh->pool, NULL);
	obstack_init(&sh->pool);
}
/*
 * strhash_close: close hash array.
 *
 *	i)	sh	STRHASH structure
 */
void
strhash_close(STRHASH *sh)
{
	strhash_reset(sh);
	free(sh->htab);
	free(sh);
}
