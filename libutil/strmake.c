/*
 * Copyright (c) 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000 Tama Communications Corporation
 *
 * This file is part of GNU GLOBAL.
 *
 * GNU GLOBAL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GNU GLOBAL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#include "strbuf.h"
#include "strmake.h"

static STRBUF *sb;

char *
strmake(p, lim)
	const char *p;
	const char *lim;
{
	const char *c;

	if (sb == NULL)
		sb = strbuf_open(0);
	strbuf_reset(sb);
	for (; *p; p++) {
		for (c = lim; *c; c++)
			if (*p == *c)
				goto end;
		strbuf_putc(sb,*p);
	}
end:
	return strbuf_value(sb);
}
