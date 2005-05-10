/*
 * Copyright (c) 2004 Tama Communications Corporation
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
#ifndef _VARRAY_H
#define _VARRAY_H

typedef struct _varray {
        char *vbuf;
	int size;
	int length;
	int alloced;
	int expand;
} VARRAY;

VARRAY *varray_open(int, int);
void *varray_assign(VARRAY *, int, int);
void *varray_append(VARRAY *);
void varray_reset(VARRAY *);
void varray_close(VARRAY *);

#endif /* ! _VARRAY_H */
