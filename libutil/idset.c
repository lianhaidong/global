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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include "checkalloc.h"
#include "die.h"
#include "idset.h"

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

/*
Idset: usage and memory status

				idset->set
				[]

				 00000000  00111111  11112222
				 01234567  89012345  67890123
idset = idset_open(21)		[00000000][00000000][00000___](3bytes)
				  v
idset_add(idset, 1)		[01000000][00000000][00000___]
				   v
idset_add(idset, 2)		[01100000][00000000][00000___]
				                         v
idset_add(idset, 20)		[01100000][00000000][00001___]

idset_contains(idset, 2) == true
idset_contains(idset, 3) == false

idset_close(idset)		[]
 */
/*
 * Allocate memory for new idset.
 */
IDSET *
idset_open(unsigned int size)
{
	IDSET *idset = (IDSET *)check_calloc(sizeof(IDSET), 1);

	idset->set = (unsigned char *)check_calloc((size + CHAR_BIT - 1) / CHAR_BIT, 1);
	idset->max = -1;
	idset->size = size;
	return idset;
}
/*
 * Add id to the idset.
 *
 *	i)	idset	idset structure
 *	i)	id	id number
 */
void
idset_add(IDSET *idset, unsigned int id)
{
	if (id >= idset->size)
		die("idset_add: id is out of range.");
	idset->set[id / CHAR_BIT] |= 1 << (id % CHAR_BIT);
	if (idset->max == -1 || id > idset->max)
		idset->max = id;
}
/*
 * Whether or not idset includes specified id.
 *
 *	i)	idset	idset structure
 *	i)	id	id number
 *	r)		true: contains, false: doesn't contain
 */
int
idset_contains(IDSET *idset, unsigned int id)
{
	return (idset->max == -1 || id > idset->max) ? 0 :
			(idset->set[id / CHAR_BIT] & (1 << (id % CHAR_BIT)));
}
/*
 * Get first id.
 *
 *      i)      idset   idset structure
 *      r)              id (-1: end of id)
 *
 */
int
idset_first(IDSET *idset)
{
	int i, limit = idset->max / CHAR_BIT + 1;
	int index0 = 0;

	if (idset->max == -1)
		return -1;
	for (i = index0; i < limit && idset->set[i] == 0; i++)
		;
	if (i >= limit)
		return -1;
	index0 = i;
	for (i = 0; i < CHAR_BIT; i++)
		if ((1 << i) & idset->set[index0])
			return idset->lastid = index0 * CHAR_BIT + i;
	die("idset_first: internal error.");
}
/*
 * Get next id.
 *
 *      i)      idset   idset structure
 *      r)              id (-1: end of id)
 *
 */
int
idset_next(IDSET *idset)
{
	int i, limit = idset->max / CHAR_BIT + 1;
	int index0, index1;

	if (idset->max == -1)
		return -1;
	if (idset->lastid >= idset->max)
		return -1;
	index0 = idset->lastid / CHAR_BIT;
	index1 = idset->lastid % CHAR_BIT;
	for (i = ++index1; i < CHAR_BIT; i++)
		if ((1 << i) & idset->set[index0])
			return idset->lastid = index0 * CHAR_BIT + i;
	index0++;
	for (i = index0; i < limit && idset->set[i] == 0; i++)
		;
	if (i >= limit)
		return -1;
	index0 = i;
	for (i = 0; i < CHAR_BIT; i++)
		if ((1 << i) & idset->set[index0])
			return idset->lastid = index0 * CHAR_BIT + i;
	die("idset_next: internal error.");
}
/*
 * Return the number of bits.
 *
 *	i)	idset	idset structure
 *	r)		number of bits
 */
int
idset_count(IDSET *idset)
{
	int id, count = 0;

	for (id = idset_first(idset); id != -1; id = idset_next(idset))
		count++;
	return count;
}
/*
 * Free memory for the idset.
 */
void
idset_close(IDSET *idset)
{
	free(idset->set);
	free(idset);
}
