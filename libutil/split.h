/*
 * Copyright (c) 2002, 2005 Tama Communications Corporation
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

#ifndef _SPLIT_H_
#define _SPLIT_H_

#define NPART 10

/*
 * Element id for ctags -x format.
 *
 * PART_TAG     PART_LNO PART_PATH      PART_LINE
 * +----------------------------------------------
 * |main             227 src/main       main()
 */
#define PART_TAG  0
#define PART_LNO  1
#define PART_PATH 2
#define PART_LINE 3
#define PART_PATH_COMP	1
#define PART_LNO_COMP	2
/*
 * Element id for ctags format.
 *
 * PART_TAG     PART_PATH     PART_LNO
 * +----------------------------------------------
 * |main        src/main      227
 */
#define PART_CTAGS_PATH 1
#define PART_CTAGS_LNO  2

typedef struct {
        int npart;
	struct part {
		char *start;
		char *end;
		int savec;
	} part[NPART];
} SPLIT;

int split(char *, int, SPLIT *);
void recover(SPLIT *);
void split_dump(SPLIT *);

#endif /* ! _SPLIT_H_ */
