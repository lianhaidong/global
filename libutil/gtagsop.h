/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2005, 2006
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

#ifndef _GTOP_H_
#define _GTOP_H_
#include <stdio.h>
#include <ctype.h>

#include "gparam.h"
#include "dbop.h"
#include "idset.h"
#include "strbuf.h"
#include "strhash.h"

#define COMPACTKEY	" __.COMPACT"
#define COMPRESSKEY	" __.COMPRESS"

#define GPATH		0
#define GTAGS		1
#define GRTAGS		2
#define GSYMS		3
#define GTAGLIM		4

#define	GTAGS_READ	0
#define GTAGS_CREATE	1
#define GTAGS_MODIFY	2

/* gtags_open() */
#define GTAGS_COMPACT		1	/* compact option */
#define GTAGS_COMPRESS		2	/* compression option */
/* gtags_add() */
#define GTAGS_UNIQUE		1	/* compress duplicate lines */
#define GTAGS_EXTRACTMETHOD	2	/* extract method from class definition */
#define GTAGS_DEBUG		65536	/* print information for debug */
/* gtags_first() */
#define GTOP_KEY		1	/* read key part */
#define GTOP_PREFIX		2	/* prefixed read */
#define GTOP_NOSOURCE		4	/* don't read source file */
#define GTOP_NOREGEX		8	/* don't use regular expression */
#define GTOP_IGNORECASE		16	/* ignore case distinction */
#define GTOP_BASICREGEX		32	/* use basic regular expression */
#define GTOP_PATH		64	/* read path part */

typedef struct {
	DBOP *dbop;			/* descripter of DBOP */
	int format_version;		/* format version */
	int format;			/* GTAGS_COMPACT, GTAGS_COMPRESS */
	int mode;			/* mode */
	int db;				/* 0:GTAGS, 1:GRTAGS, 2:GSYMS */
	int openflags;			/* flags value of gtags_open() */
	int flags;			/* flags */
	char root[MAXPATHLEN+1];	/* root directory of source tree */
	/*
	 * Path name only.
	 */
	int path_count;
	int path_index;
	char **path_array;
	/*
	 * Stuff for compact format
	 */
	int opened;			/* whether or not file opened */
	char *line;			/* current record */
	char tag[IDENTLEN+1];		/* current tag */
	char path[MAXPATHLEN+1];	/* current path */
	char prev_path[MAXPATHLEN+1];	/* previous path */
	STRBUF *sb;			/* string buffer */
	STRBUF *ib;			/* input buffer */
	FILE *fp;			/* descriptor of 'path' */
	const char *lnop;		/* current line number */
	int lno;			/* integer value of 'lnop' */
	/* used for compact format and path name only read */
	STRHASH *pool;
} GTOP;

const char *dbname(int);
GTOP *gtags_open(const char *, const char *, int, int);
void gtags_put(GTOP *, const char *, const char *);
void gtags_delete(GTOP *, IDSET *);
const char *gtags_first(GTOP *, const char *, int);
const char *gtags_next(GTOP *);
void gtags_close(GTOP *);

#endif /* ! _GTOP_H_ */
