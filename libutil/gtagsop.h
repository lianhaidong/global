/*
 * Copyright (c) 1997, 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000, 2001 Tama Communications Corporation
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

#ifndef _GTOP_H_
#define _GTOP_H_
#include <stdio.h>
#include <ctype.h>

#include "gparam.h"
#include "dbop.h"
#include "strbuf.h"

#define VERSIONKEY	" __.VERSION"
#define COMPACTKEY	" __.COMPACT"
#define PATHINDEXKEY	" __.PATHINDEX"

#define GPATH		0
#define GTAGS		1
#define GRTAGS		2
#define GSYMS		3
#define GTAGLIM		4

#define	GTAGS_READ	0
#define GTAGS_CREATE	1
#define GTAGS_MODIFY	2

/* gtagsopen() */
#define GTAGS_STANDARD		0	/* standard format */
#define GTAGS_COMPACT		1	/* compact format */
#define GTAGS_PATHINDEX		2	/* use path index */
#define GTAGS_POSTGRES		4	/* use postgres database */
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

#define isregexchar(c)	(regexchar[c])

typedef struct {
	DBOP	*dbop;			/* descripter of DBOP */
	int	format_version;		/* format version */
	int	format;			/* GTAGS_STANDARD, GTAGS_COMPACT */
	int	mode;			/* mode */
	int	db;			/* 0:GTAGS, 1:GRTAGS, 2:GSYMS */
	int	openflags;		/* flags value of gtags_open() */
	int	flags;			/* flags */
	char	root[MAXPATHLEN+1];	/* root directory of source tree */
	/*
	 * Stuff for compact format
	 */
	int	opened;			/* whether or not file opened */
	char	*line;			/* current record */
	char	tag[IDENTLEN+1];	/* current tag */
	char	prev_tag[IDENTLEN+1];	/* previous tag */
	char	path[MAXPATHLEN+1];	/* current path */
	char	prev_path[MAXPATHLEN+1];/* previous path */
	char	prev_fid[32];		/* previous fid (postgres) */
	STRBUF	*sb;			/* string buffer */
	STRBUF	*ib;			/* input buffer */
	FILE	*fp;			/* descriptor of 'path' */
	char	*lnop;			/* current line number */
	int	lno;			/* integer value of 'lnop' */
} GTOP;

const char *dbname(int);
void	makecommand(char *, char *, STRBUF *);
int	formatcheck(char *, int);
int	notnamechar(char *);
int	isregex(char *);
void	gtags_setinfo(char *);
GTOP	*gtags_open(char *, char *, int, int, int);
void	gtags_put(GTOP *, char *, char *, char *);
char	*gtags_get(GTOP *, char *);
void    gtags_add(GTOP *, char *, char *, int);
void	gtags_delete(GTOP *, char *);
char	*gtags_first(GTOP *, char *, int);
char	*gtags_next(GTOP *);
void	gtags_close(GTOP *);

#endif /* ! _GTOP_H_ */
