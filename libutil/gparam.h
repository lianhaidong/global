/*
 * Copyright (c) 1996, 1997, 1998, 1999
 *             Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999, 2000
 *             Tama Communications Corporation. All rights reserved.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _GPARAM_H_
#define _GPARAM_H_
#ifndef __BORLANDC__
#include <sys/param.h>
#endif

#define MAXFILLEN	1024		/* max length of filter		*/
#define IDENTLEN	512		/* max length of ident		*/
#define MAXBUFLEN	1024		/* max length of buffer		*/
#define MAXPROPLEN	1024		/* max length of property	*/
#define MAXARGLEN	512		/* max length of argment	*/
#define MAXKEYLEN	300		/* max length of record key	*/
#define MAXTOKEN	512		/* max length of token		*/
#ifndef MAXPATHLEN
#define MAXPATHLEN	1024		/* max length of path		*/
#endif

#endif /* ! _GPARAM_H_ */
