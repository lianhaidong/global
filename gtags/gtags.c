/*
 * Copyright (c) 1996, 1997, 1998, 1999
 *             Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999, 2000, 2001
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
#include <utime.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "getopt.h"

#include "global.h"

const char *progname  = "gtags";		/* command name */

static void	usage(void);
void	signal_setup(void);
void	onintr(int);
int	main(int, char **);
int	incremental(char *, char *);
void	updatetags(char *, char *, char *, int);
void	createtags(char *, char *, int);
int	printconf(char *);
char	*now(void);

int	cflag;					/* compact format */
int	iflag;					/* incremental update */
int	Iflag;					/* make  idutils index */
int	oflag;					/* suppress making GSYMS */
int	wflag;					/* warning message */
int	vflag;					/* verbose mode */
int     show_version;
int     show_help;
int	show_config;
int	do_find;
int	do_expand;
int	do_date;
int	do_pwd;
int	gtagsconf;
int	debug;
char	*extra_options;

int	extractmethod;
int	total;
const char *usage_const = "Usage: gtags [-c][-i][-I][-o][-v][-w][dbpath]\n";
const char *help_const = "\
Options:\n\
     -c, --compact\n\
             make tag files with compact format.\n\
     -i, --incremental\n\
             update tag files incrementally.\n\
     --gtagsconf file\n\
             load configuration file.\n\
     -I, --idutils\n\
             make index files for idutils(1).\n\
     -o, --omit-gsyms\n\
             suppress making GSYMS file.\n\
     -v, --verbose\n\
             verbose mode.\n\
     -w, --warning\n\
             print warning messages.\n\
";

static void
usage()
{
	fputs(usage_const, stderr);
	exit(2);
}
static void
help()
{
	fputs(usage_const, stdout);
	fputs(help_const, stdout);
	exit(2);
}

static struct option const long_options[] = {
	{"compact", no_argument, NULL, 'c'},
	{"incremental", no_argument, NULL, 'i'},
	{"omit-gsyms", no_argument, NULL, 'o'},
	{"verbose", no_argument, NULL, 'v'},
	{"warning", no_argument, NULL, 'w'},

	/* long name only */
	{"config", no_argument, &show_config, 1},
	{"date", no_argument, &do_date, 1},
	{"debug", no_argument, &debug, 1},
	{"expand", required_argument, &do_expand, 1},
	{"find", no_argument, &do_find, 1},
	{"gtagsconf", required_argument, &gtagsconf, 1},
	{"idutils", optional_argument, &Iflag, 1},
	{"pwd", no_argument, &do_pwd, 1},
	{"version", no_argument, &show_version, 1},
	{"help", no_argument, &show_help, 1},
	{ 0 }
};

/*
 * Gtags catch signal even if the parent ignore it.
 */
int	exitflag = 0;

void
onintr(signo)
int     signo;
{
	signo = 0;      /* to satisfy compiler */
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
	char    root[MAXPATHLEN+1];
	char	dbpath[MAXPATHLEN+1];
	char	cwd[MAXPATHLEN+1];
	STRBUF	*sb = strbuf_open(0);
	const char *p;
	int	db;
	int	optchar;
	int	option_index = 0;

	while ((optchar = getopt_long(argc, argv, "cGiIovw", long_options, &option_index)) != EOF) {
		switch (optchar) {
		case 0:
			p = long_options[option_index].name;
			if (!strcmp(p, "expand"))
				settabs(atoi(optarg + 1));
			else if (!strcmp(p, "idutils"))
				extra_options = optarg;
			else if (gtagsconf) {
				gtagsconf = 0;
				if (!optarg)
					usage();
				else
#ifdef HAVE_PUTENV
				{
					char *env = (char *)malloc(strlen("GTAGSCONF=")+strlen(optarg)+1);

					if (!env)	
						die("short of memory.");
					strcpy(env, "GTAGSCONF=");
					strcat(env, optarg);
					putenv(env);
				}
#else
				setenv("GTAGSCONF", optarg, 1);
#endif /* HAVE_PUTENV */
			}
			break;
		case 'c':
			cflag++;
			break;
		case 'i':
			iflag++;
			break;
		case 'I':
			Iflag++;
			break;
		case 'o':
			oflag++;
			break;
		case 'w':
			wflag++;
			break;
		case 'v':
			vflag++;
			break;
		/* for compatibility */
		case 's':
		case 'e':
			break;
		default:
			usage();
			break;
		}
	}
	if (show_version)
		version(NULL, vflag);
	if (show_help)
		help();

	argc -= optind;
        argv += optind;

	if (do_expand) {
		FILE *ip;
		STRBUF *ib = strbuf_open(MAXBUFLEN);

		ip = (argc) ? fopen(argv[0], "r") : stdin;
		if (ip == NULL)
			exit(1);
		while (strbuf_fgets(ib, ip, STRBUF_NOCRLF) != NULL)
			detab(stdout, strbuf_value(ib));
		strbuf_close(ib);
		exit(0);
	} else if (do_date) {
		fprintf(stdout, "%s\n", now());
		exit(0);
	} else if (do_pwd) {
		if (!getcwd(cwd, MAXPATHLEN))
			die("cannot get current directory.");
		canonpath(cwd);
		fprintf(stdout, "%s\n", cwd);
		exit(0);
	} else if (show_config) {
		if (debug)
			fprintf(stdout, "getconfline: %s\n", getconfline());
		if (argc == 0)
			fprintf(stdout, "%s\n", getconfline());
		else if (!printconf(argv[0]))
				exit(1);
		exit(0);
	} else if (do_find) {
		char	*local;
		char	*p;

		local = (argc) ? argv[0] : "./";
		getdbpath(cwd, root, dbpath, 0);
		for (gfindopen(dbpath, local); (p = gfindread()) != NULL; )
			fprintf(stdout, "%s\n", p);
		gfindclose();
		exit(0);
	} else if (Iflag) {
		if (!usable("mkid"))
			die("mkid not found.");
	}

	if (!getcwd(cwd, MAXPATHLEN))
		die("cannot get current directory.");
	canonpath(cwd);
	/*
	 * Decide directory (dbpath) in which gtags make tag files.
	 *
	 * Gtags create tag files at current directory by default.
	 * If dbpath is specified as an argument then use it.
	 * If the -i option specified and both GTAGS and GRTAGS exists
	 * at one of the candedite directories then gtags use existing
	 * tag files.
	 */
	if (iflag) {
		if (argc > 0)
			realpath(*argv, dbpath);
		else if (!gtagsexist(cwd, dbpath, MAXPATHLEN, vflag))
			strcpy(dbpath, cwd);
	} else {
		if (argc > 0)
			realpath(*argv, dbpath);
		else
			strcpy(dbpath, cwd);
	}
	if (iflag && (!test("f", makepath(dbpath, dbname(GTAGS), NULL)) ||
		!test("f", makepath(dbpath, dbname(GPATH), NULL)))) {
		if (wflag)
			fprintf(stderr, "Warning: GTAGS or GPATH not found. -i option ignored.\n");
		iflag = 0;
	}
	if (!test("d", dbpath))
		die("directory '%s' not found.", dbpath);
	if (vflag)
		fprintf(stderr, "[%s] Gtags started.\n", now());
	/*
	 * load .gtagsrc or /etc/gtags.conf
	 */
	openconf();
	if (getconfb("extractmethod"))
		extractmethod = 1;
	strbuf_reset(sb);
	if (getconfs("format", sb) && !strcmp(strbuf_value(sb), "compact"))
		cflag++;
	/*
	 * teach gctags(1) where is dbpath by environment variable.
	 */
#ifdef HAVE_PUTENV
	{
		char *env = (char *)malloc(strlen("GTAGSDBPATH=")+strlen(dbpath)+1);

		if (!env)	
			die("short of memory.");
		strcpy(env, "GTAGSDBPATH=");
		strcat(env, dbpath);
		putenv(env);
	}
#else
	setenv("GTAGSDBPATH", dbpath, 1);
#endif /* HAVE_PUTENV */

	if (wflag) {
#ifdef HAVE_PUTENV
		putenv("GTAGSWARNING=1");
#else
		setenv("GTAGSWARNING", "1", 1);
#endif /* HAVE_PUTENV */
	}
	/*
	 * incremental update.
	 */
	if (iflag) {
		/*
		 * Version check. If existing tag files are old enough
		 * gtagsopen() abort with error message.
		 */
		GTOP *gtop = gtagsopen(dbpath, cwd, GTAGS, GTAGS_MODIFY, 0);
		gtagsclose(gtop);
		/*
		 * GPATH is needed for incremental updating.
		 * Gtags check whether or not GPATH exist, since it may be
		 * removed by mistake.
		 */
		if (!test("f", makepath(dbpath, dbname(GPATH), NULL)))
			die("Old version tag file found. Please remake it.");
		(void)incremental(dbpath, cwd);
		exit(0);
	}
	/*
 	 * create GTAGS, GRTAGS and GSYMS
	 */
	signal_setup();
	total = 0;					/* counting file */
	for (db = GTAGS; db < GTAGLIM; db++) {

		if (oflag && db == GSYMS)
			continue;
		strbuf_reset(sb);
		if (!getconfs(dbname(db), sb))
			continue;
		if (!usable(strmake(strbuf_value(sb), " \t")))
			die("Parser '%s' not found or not executable.", strmake(strbuf_value(sb), " \t"));
		if (vflag)
			fprintf(stderr, "[%s] Creating '%s'.\n", now(), dbname(db));
		createtags(dbpath, cwd, db);
		strbuf_reset(sb);
		if (db == GTAGS) {
			if (getconfs("GTAGS_extra", sb))
				if (system(strbuf_value(sb)))
					fprintf(stderr, "GTAGS_extra command failed: %s\n", strbuf_value(sb));
		} else if (db == GRTAGS) {
			if (getconfs("GRTAGS_extra", sb))
				if (system(strbuf_value(sb)))
					fprintf(stderr, "GRTAGS_extra command failed: %s\n", strbuf_value(sb));
		} else if (db == GSYMS) {
			if (getconfs("GSYMS_extra", sb))
				if (system(strbuf_value(sb)))
					fprintf(stderr, "GSYMS_extra command failed: %s\n", strbuf_value(sb));
		}
		if (exitflag)
			exit(1);
	}
	/*
	 * create idutils index.
	 */
	if (Iflag) {
		if (vflag)
			fprintf(stderr, "[%s] Creating indexes for idutils.\n", now());
		strbuf_reset(sb);
		strbuf_puts(sb, "mkid --file=");
		strbuf_puts(sb, makepath(dbpath, "ID", NULL));
		if (vflag)
			strbuf_puts(sb, " -v");
		if (extra_options) {
			strbuf_putc(sb, ' ');
			strbuf_puts(sb, extra_options);
		}
		strbuf_putc(sb, ' ');
		strbuf_puts(sb, "`gtags --find`");
		strbuf_putc(sb, ' ');
		if (vflag)
			strbuf_puts(sb, "1>&2");
		else
			strbuf_puts(sb, ">/dev/null");
		if (debug)
			fprintf(stderr, "executing mkid like: %s\n", strbuf_value(sb));
		if (system(strbuf_value(sb)))
			die("mkid failed: %s", strbuf_value(sb));
		strbuf_reset(sb);
		strbuf_puts(sb, "chmod 644 ");
		strbuf_puts(sb, makepath(dbpath, "ID", NULL));
		if (system(strbuf_value(sb)))
			die("chmod failed: %s", strbuf_value(sb));
	}
	if (vflag)
		fprintf(stderr, "[%s] Done.\n", now());
	closeconf();
	strbuf_close(sb);

	return 0;
}
/*
 * incremental: incremental update
 *
 *	i)	dbpath	dbpath directory
 *	i)	root	root directory of source tree
 *	r)		0: not updated, 1: updated
 */
int
incremental(dbpath, root)
char	*dbpath;
char	*root;
{
	struct stat statp;
	time_t	gtags_mtime;
	STRBUF	*addlist = strbuf_open(0);
	STRBUF	*updatelist = strbuf_open(0);
	STRBUF	*deletelist = strbuf_open(0);
	int	updated = 0;
	char	*path;

	if (vflag) {
		fprintf(stderr, " Tag found in '%s'.\n", dbpath);
		fprintf(stderr, " Incremental update.\n");
	}
	/*
	 * get modified time of GTAGS.
	 */
	path = makepath(dbpath, dbname(GTAGS), NULL);
	if (stat(path, &statp) < 0)
		die("stat failed '%s'.", path);
	gtags_mtime = statp.st_mtime;

	if (gpath_open(dbpath, 0) < 0)
		die("GPATH not found.");
	/*
	 * make add list and update list.
	 */
	for (ffindopen(); (path = ffindread(NULL)) != NULL; ) {
		if (locatestring(path, " ", MATCH_FIRST)) {
			if (wflag)
				fprintf(stderr, "Warning: '%s' ignored, because it includes blank in the path.\n", path);
			continue;
		}
		if (stat(path, &statp) < 0)
			die("stat failed '%s'.", path);
		if (!gpath_path2id(path))
			strbuf_puts0(addlist, path);
		else if (gtags_mtime < statp.st_mtime)
			strbuf_puts0(updatelist, path);
	}
	ffindclose();
	/*
	 * make delete list.
	 */
	{
		int i, limit = gpath_nextkey();

		for (i = 0; i < limit; i++) {
			if ((path = gpath_id2path(i)) == NULL)
				continue;
			if (!test("f", path))
				strbuf_puts0(deletelist, path);
		}
	}
	gpath_close();
	if (strbuf_getlen(addlist) + strbuf_getlen(deletelist) + strbuf_getlen(updatelist))
		updated = 1;
	/*
	 * execute updating.
	 */
	signal_setup();
	if (strbuf_getlen(updatelist) > 0) {
		char	*start = strbuf_value(updatelist);
		char	*end = start + strbuf_getlen(updatelist);
		char	*p;

		for (p = start; p < end; p += strlen(p) + 1) {
			updatetags(dbpath, root, p, 0);
			if (exitflag)
				exit(1);
		}
		updated = 1;
	}
	if (strbuf_getlen(addlist) > 0) {
		char	*start = strbuf_value(addlist);
		char	*end = start + strbuf_getlen(addlist);
		char	*p;

		for (p = start; p < end; p += strlen(p) + 1) {
			updatetags(dbpath, root, p, 1);
			if (exitflag)
				exit(1);
		}
		updated = 1;
	}
	if (strbuf_getlen(deletelist) > 0) {
		char	*start = strbuf_value(deletelist);
		char	*end = start + strbuf_getlen(deletelist);
		char	*p;

		for (p = start; p < end; p += strlen(p) + 1) {
			updatetags(dbpath, root, p, 2);
			if (exitflag)
				exit(1);
		}

		gpath_open(dbpath, 2);
		for (p = start; p < end; p += strlen(p) + 1) {
			if (exitflag)
				break;
			gpath_del(p);
		}
		gpath_close();
		updated = 1;
	}
	if (exitflag)
		exit(1);
	if (updated) {
		int	db;
		/*
		 * Update modification time of tag files
		 * because they may have no definitions.
		 */
		for (db = GTAGS; db < GTAGLIM; db++)
#ifdef HAVE_UTIMES
			utimes(makepath(dbpath, dbname(db), NULL), NULL);
#else
			utime(makepath(dbpath, dbname(db), NULL), NULL);
#endif /* HAVE_UTIMES */
	}
	if (vflag) {
		if (updated)
			fprintf(stderr, " Global databases have been modified.\n");
		else
			fprintf(stderr, " Global databases are up to date.\n");
		fprintf(stderr, "[%s] Done.\n", now());
	}
	strbuf_close(addlist);
	strbuf_close(deletelist);
	strbuf_close(updatelist);
	return updated;
}
/*
 * updatetags: update tag file.
 *
 *	i)	dbpath	directory in which tag file exist
 *	i)	root	root directory of source tree
 *	i)	path	path which should be updated
 *	i)	type	0:update, 1:add, 2:delete
 */
void
updatetags(dbpath, root, path, type)
char	*dbpath;
char	*root;
char	*path;
int	type;
{
	GTOP	*gtop;
	STRBUF	*sb = strbuf_open(0);
	int	db;
	const char *msg = NULL;

	switch (type) {
	case 0:	msg = "Updating"; break;
	case 1: msg = "Adding"; break;
	case 2:	msg = "Deleting"; break;
	}
	if (vflag)
		fprintf(stderr, " %s tags of '%s' ...", msg, path + 2);
	for (db = GTAGS; db < GTAGLIM; db++) {
		int	gflags = 0;

		if (exitflag)
			break;
		/*
		 * GTAGS needed at least.
		 */
		if ((db == GRTAGS || db == GSYMS) && !test("f", makepath(dbpath, dbname(db), NULL)))
			continue;
		/*
		 * GTAGS needed to make GRTAGS.
		 */
		if (db == GRTAGS && !test("f", makepath(dbpath, dbname(GTAGS), NULL)))
			die("GTAGS needed to create GRTAGS.");
		if (vflag)
			fprintf(stderr, "%s", dbname(db));
		/*
		 * get tag command.
		 */
		strbuf_reset(sb);
		if (!getconfs(dbname(db), sb))
			die("cannot get tag command. (%s)", dbname(db));
		gtop = gtagsopen(dbpath, root, db, GTAGS_MODIFY, 0);
		if (type != 1)
			gtagsdelete(gtop, path);
		if (vflag)
			fprintf(stderr, "..");
		if (type != 2) {
			if (db == GSYMS)
				gflags |= GTAGS_UNIQUE;
			if (extractmethod)
				gflags |= GTAGS_EXTRACTMETHOD;
			if (debug)
				gflags |= GTAGS_DEBUG;
			gtagsadd(gtop, strbuf_value(sb), path, gflags);
		}
		gtagsclose(gtop);
	}
	strbuf_close(sb);
	if (exitflag)
		return;
	if (vflag)
		fprintf(stderr, " Done.\n");
}
/*
 * createtags: create tags file
 *
 *	i)	dbpath	dbpath directory
 *	i)	root	root directory of source tree
 *	i)	db	GTAGS, GRTAGS, GSYMS
 */
void
createtags(dbpath, root, db)
char	*dbpath;
char	*root;
int	db;
{
	char	*path;
	GTOP	*gtop;
	int	flags;
	char	*comline;
	STRBUF	*sb = strbuf_open(0);
	int	count = 0;

	/*
	 * get tag command.
	 */
	if (!getconfs(dbname(db), sb))
		die("cannot get tag command. (%s)", dbname(db));
	comline = strdup(strbuf_value(sb));
	if (!comline)
		die("short of memory.");
	/*
	 * GTAGS needed to make GRTAGS.
	 */
	if (db == GRTAGS && !test("f", makepath(dbpath, dbname(GTAGS), NULL)))
		die("GTAGS needed to create GRTAGS.");
	flags = 0;
	if (cflag) {
		flags |= GTAGS_COMPACT;
		flags |= GTAGS_PATHINDEX;
	}
	strbuf_reset(sb);
	if (vflag > 1 && getconfs(dbname(db), sb))
		fprintf(stderr, " using tag command '%s <path>'.\n", strbuf_value(sb));
	gtop = gtagsopen(dbpath, root, db, GTAGS_CREATE, flags);
	for (ffindopen(); (path = ffindread(NULL)) != NULL; ) {
		int	gflags = 0;
		int	skip = 0;

		if (exitflag)
			break;
		if (locatestring(path, " ", MATCH_FIRST)) {
			if (wflag)
				fprintf(stderr, "Warning: '%s' ignored, because it includes blank in the path.\n", path);
			continue;
		}
		count++;
		/*
		 * GSYMS doesn't treat asembler.
		 */
		if (db == GSYMS) {
			if (locatestring(path, ".s", MATCH_AT_LAST) != NULL ||
			    locatestring(path, ".S", MATCH_AT_LAST) != NULL)
				skip = 1;
		}
		if (vflag) {
			if (total)
				fprintf(stderr, " [%d/%d]", count, total);
			else
				fprintf(stderr, " [%d]", count);
			fprintf(stderr, " extracting tags of %s", path);
			if (skip)
				fprintf(stderr, " (skipped)");
			fputc('\n', stderr);
		}
		if (skip)
			continue;
		if (db == GSYMS)
			gflags |= GTAGS_UNIQUE;
		if (extractmethod)
			gflags |= GTAGS_EXTRACTMETHOD;
		if (debug)
			gflags |= GTAGS_DEBUG;
		gtagsadd(gtop, comline, path, gflags);
	}
	total = count;				/* save total count */
	ffindclose();
	gtagsclose(gtop);
	free(comline);
	strbuf_close(sb);
}
/*
 * now: current date and time
 *
 *	r)		date and time
 */
char *
now(void)
{
	static	char	buf[128];

#ifdef HAVE_STRFTIME
	time_t	tval;

	if (time(&tval) == -1)
		die("cannot get current time.");
	(void)strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Z %Y", localtime(&tval));
#else
	FILE	*ip;

	strcpy(buf, "unkown time");
	if (ip = popen("date", "r")) {
		if (fgets(buf, sizeof(buf), ip))
			buf[strlen(buf) - 1] = 0;
		fclose(ip);
	}
#endif
	return buf;
}
/*
 * printconf: print configuration data.
 *
 *	i)	name	label of config data
 *	r)		exit code
 */
int
printconf(name)
char	*name;
{
	int	num;
	int	exist = 1;

	if (getconfn(name, &num))
		fprintf(stdout, "%d\n", num);
	else if (getconfb(name))
		fprintf(stdout, "1\n");
	else {
		STRBUF  *sb = strbuf_open(0);
		if (getconfs(name, sb))
			fprintf(stdout, "%s\n", strbuf_value(sb));
		else
			exist = 0;
		strbuf_close(sb);
	}
	return exist;
}
