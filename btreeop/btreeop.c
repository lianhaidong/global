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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif


#include "gparam.h"
#include "die.h"
#include "dbop.h"
#include "strbuf.h"
#include "tab.h"
#include "version.h"

const char *dbdefault = "btree";   	/* default database name */
const char *progname  = "btreeop";		/* command name */
int statistics;

static void	usage(void);
void	signal_setup(void);
void	onintr(int);
int	main(int, char **);
void	dbwrite(DBOP *);
void	dbkey(DBOP *, char *, int);
void	dbscan(DBOP *, char *, int);
void	dbdel(DBOP *, char *, int);
void	dbbysecondkey(DBOP *, int, char *, int);

#define F_KEY	0
#define F_DEL	1

static void
usage()
{
	fprintf(stderr, "%s\n",
		"usage: btreeop [-A][-C][-D[n] key][-K[n] key][-L[2]][-k prefix][dbname]");
	exit(2);
}

/*
 * Btreeop catch signal even if the parent ignore it.
 */
int	exitflag = 0;

void
onintr(signo)
int	signo;
{
	signo = 0;	/* to satisfy compiler */
	exitflag = 1;
}

void
signal_setup()
{
	signal(SIGINT, onintr);
	signal(SIGTERM, onintr);
#ifdef SIGHUP
	signal(SIGHUP, onintr);
#endif
#ifdef SIGQUIT
	signal(SIGQUIT, onintr);
#endif
}

int
main(argc, argv)
int	argc;
char	*argv[];
{
	char	command = 'R';
	char	*key = NULL;
	int     mode = 0;
	const char *db_name;
	DBOP	*dbop;
	int	i, c;
	int	secondkey = 0;
	int	keylist = 0;
	char	*prefix = NULL;

	for (i = 1; i < argc && argv[i][0] == '-'; ++i) {
		if (!strcmp(argv[i], "--version"))
			version(NULL, 0);
#ifdef STATISTICS
		else if (!strcmp(argv[i], "--dump")) {
			command = 'P';
			continue;
		}
		else if (!strcmp(argv[i], "--stat")) {
			statistics = 1;
			continue;
		}
#endif
		switch (c = argv[i][1]) {
		case 'D':
		case 'K':
			if (argv[i][2] && isdigit(argv[i][2]))
				secondkey = atoi(&argv[i][2]);
			if (++i < argc)
				key = argv[i];
			else
				usage();
			/* FALLTHROUGH */
		case 'A':
		case 'C':
		case 'L':
			if (command != 'R')
				usage();
			command = c;
			if (command == 'L') {
				keylist = 1;
				if (argv[i][2] == '2')
					keylist = 2;
			}
			break;
		case 'k':
			if (++i < argc)
				prefix = argv[i];
			else
				usage();
			break;
		default:
			usage();
			break;
		}
	}
	db_name = (i < argc) ? argv[i] : dbdefault;
	switch (command) {
	case 'A':
	case 'D':
		mode = 2;
		break;
	case 'C':
		mode = 1;
		break;
	case 'K':
	case 'L':
	case 'P':
	case 'R':
		mode = 0;
		break;
	}
	dbop = dbop_open(db_name, mode, 0644, DBOP_DUP);
	if (dbop == NULL) {
		switch (mode) {
		case 0:
		case 2:
			die("cannot open '%s'.", db_name);
			break;
		case 1:
			die("cannot create '%s'.", db_name);
			break;
		}
	}
	switch (command) {
	case 'A':			/* Append records */
	case 'C':			/* Create database */
		dbwrite(dbop);
		break;
	case 'D':			/* Delete records */
		dbdel(dbop, key, secondkey);
		break;
	case 'K':			/* Keyed (indexed) read */
		dbkey(dbop, key, secondkey);
		break;
	case 'R':			/* sequencial Read */
	case 'L':			/* primary key List */
		dbscan(dbop, prefix, keylist);
		break;
#ifdef STATISTICS
	case 'P':
		dbop_dump(dbop);
		break;
#endif
	}
#ifdef STATISTICS
	if (statistics)
		dbop_stat(dbop);
#endif
	dbop_close(dbop);

	return exitflag;
}
/*
 * dbwrite: write to database
 *
 *	i)	dbop		database
 */
void
dbwrite(dbop)
DBOP	*dbop;
{
	char	*p, *c;
	char	keybuf[MAXKEYLEN+1];
	STRBUF	*ib = strbuf_open(MAXBUFLEN);

	signal_setup();
	/*
	 * Input file format:
	 * +--------------------------------------------------
	 * |Primary-key	secondary-key-1 secondary-key-2 Data\n
	 * |Primary-key	secondary-key-1 secondary-key-2 Data\n
	 * 	.
	 * 	.
	 * - Keys and Data are separated by blank('\t' or ' '). 
	 * - Keys cannot include blank.
	 * - Data can include blank.
	 * - Null record not allowed.
	 * - Secondary-key is assumed as a part of data by db(3).
	 *
	 * META record:
	 * You can write meta record by making key start with a ' '.
	 * You can read this record only by indexed read ('-K' option).
	 * +------------------
	 * | __.VERSION 2
	 */
	while ((p = strbuf_fgets(ib, stdin, STRBUF_NOCRLF)) != NULL) {
		if (exitflag)
			break;
		c = p;
		if (*c == ' ') {			/* META record */
			if (*++c == ' ')
				die("key cannot include blanks.");
		}
		for (; *c && !isspace(*c); c++)		/* skip key part */
			;
		if (*c == 0)
			die("data part not found.");
		if (c - p > MAXKEYLEN)
			die("primary key too long.");
		strncpy(keybuf, p, c - p);		/* make key string */
		keybuf[c - p] = 0;
		for (; *c && isspace(*c); c++)		/* skip blanks */
			;
		if (*c == 0)
			die("data part is null.");
		dbop_put(dbop, keybuf, p);
	}
	strbuf_close(ib);
}

/*
 * dbkey: Keyed search
 *
 *	i)	dbop		database
 *	i)	skey		key for search
 *	i)	secondkey	0: primary key, >0: secondary key
 */
void
dbkey(dbop, skey, secondkey)
DBOP	*dbop;
char	*skey;
int	secondkey;
{
	char	*p;

	if (!secondkey) {
		for (p = dbop_first(dbop, skey, NULL, 0); p; p = dbop_next(dbop))
			fprintf(stdout, "%s\n", p);
		return;
	}
	dbbysecondkey(dbop, F_KEY, skey, secondkey);
}

/*
 * dbscan: Scan records
 *
 *	i)	dbop		database
 *	i)	prefix		prefix of primary key
 *	i)	keylist		0: data, 1: key, 2: key and data
 */
void
dbscan(dbop, prefix, keylist)
DBOP	*dbop;
char	*prefix;
int	keylist;
{
	char	*p;
	int	flags = 0;

	if (prefix)	
		flags |= DBOP_PREFIX;
	if (keylist)
		flags |= DBOP_KEY;

	for (p = dbop_first(dbop, prefix, NULL, flags); p; p = dbop_next(dbop)) {
		if (keylist == 2)
			fprintf(stdout, "%s %s\n", p, dbop_lastdat(dbop));
		else
			detab(stdout, p);
	}
}

/*
 * dbdel: Delete records
 *
 *	i)	dbop		database
 *	i)	skey		key for search
 *	i)	secondkey	0: primary key, >0: secondary key
 */
void
dbdel(dbop, skey, secondkey)
DBOP	*dbop;
char	*skey;
int	secondkey;
{
	signal_setup();
	if (!secondkey) {
		dbop_del(dbop, skey);
		return;
	}
	dbbysecondkey(dbop, F_DEL, skey, secondkey);
}
/*
 * dbbysecondkey: proc by second key
 *
 *	i)	dbop	database
 *	i)	func	F_KEY, F_DEL
 *	i)	skey
 *	i)	secondkey
 */
void
dbbysecondkey(dbop, func, skey, secondkey)
DBOP	*dbop;
int	func;
char	*skey;
int	secondkey;
{
	char	*c, *p;
	int	i;

	/* trim skey */
	for (c = skey; *c && isspace(*c); c++)
		;
	skey = c;
	for (c = skey+strlen(skey)-1; *c && isspace(*c); c--)
		*c = 0;

	for (p = dbop_first(dbop, NULL, NULL, 0); p; p = dbop_next(dbop)) {
		if (exitflag)
			break;
		c = p;
		/* reach to specified key */
		for (i = secondkey; i; i--) {
			for (; *c && !isspace(*c); c++)
				;
			if (*c == 0)
				die("specified key not found.");
			for (; *c && isspace(*c); c++)
				;
			if (*c == 0)
				die("specified key not found.");
		}
		i = strlen(skey);
		if (!strncmp(c, skey, i) && (*(c+i) == 0 || isspace(*(c+i)))) {
			switch (func) {
			case F_KEY:
				detab(stdout, p);
				break;
			case F_DEL:
				dbop_del(dbop, NULL);
				break;
			}
		}
		if (exitflag)
			break;
	}
}
