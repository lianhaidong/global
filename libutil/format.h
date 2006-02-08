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
#ifndef _FORMAT_H_
#define _FORMAT_H_

/*
 * (1) ctags -x format (FORMAT_CTAGS_X)
 *
 * PART_TAG     PART_LNO PART_PATH      PART_LINE
 * +----------------------------------------------
 * |main             227 ./src/main     main()
 *
 * (2) ctags -x + file id format (FORMAT_CTAGS_XID)
 *
 * 0    PART_TAG+1 PART_LNO+1 PART_PATH+1   PART_LINE+1
 * +----------------------------------------------
 * |110	main             227 ./src/main     main()
 *
 * (3) ctags format (FORMAT_CTAGS)
 *
 * PART_TAG     PART_CTAGS_PATH    PART_CTAGS_LNO
 * +----------------------------------------------
 * |main        ./src/main         227
 *
 * (4) path name format (FORMAT_PATH)
 *
 * +----------------------------------------------
 * |./src/main
 *
 */
#define FORMAT_CTAGS		1
#define FORMAT_CTAGS_X		2
#define FORMAT_CTAGS_XID	3
#define FORMAT_PATH		4

/*
 * FORMAT_CTAGS_X
 */
#define PART_TAG  0
#define PART_LNO  1
#define PART_PATH 2
#define PART_LINE 3
/*
 * FORMAT_CTAGS
 */
#define PART_CTAGS_PATH 1
#define PART_CTAGS_LNO  2
/*
 * Compact format
 * PART_TAG  PART_PATH_COMP  PART_LNO_COMP
 * +----------------------------------------------------
 * |main     ./src/main      227
 *
 * Compact format with pathindex option
 * PART_TAG  PART_PATH_COMP  PART_LNO_COMP
 * +----------------------------------------------------
 * |main     100             227
 */
#define PART_PATH_COMP  1
#define PART_LNO_COMP   2

/*
 * Path name type
 */
#define PATH_RELATIVE	1
#define PATH_ABSOLUTE	2

#endif /* ! _FORMAT_H_ */
