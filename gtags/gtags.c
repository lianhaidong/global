/*
 * Copyright (c) 1997, 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000, 2001, 2002, 2003 Tama Communications Corporation
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
#include <errno.h>

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
#include "const.h"

static void	usage(void);
static void	help(void);
void	signal_setup(void);
void	onintr(int);
int	match(char *, char *);
int	main(int, char **);
int	incremental(char *, char *);
void	updatetags(char *, char *, char *, int);
void	createtags(char *, char *, int);
char	*now(void);
int	printconf(char *);
void	set_base_directory(char *, char *);
void	put_converting(char *, int, int);

int	cflag;					/* compact format */
int	iflag;					/* incremental update */
int	Iflag;					/* make  id-utils index */
int	oflag;					/* suppress making GSYMS */
int	Pflag;					/* use postgres */
int	qflag;					/* quiet mode */
int	wflag;					/* warning message */
int	vflag;					/* verbose mode */
int     show_version;
int     show_help;
int	show_config;
int	do_convert;
int	do_createdb;
int	do_readdb;
int	do_scandb;
int	do_find;
int	do_sort;
int	do_write;
int	do_relative;
int	do_absolute;
int	cxref;
int	do_expand;
int	do_date;
int	do_pwd;
int	gtagsconf;
int	gtagslabel;
int	other_files;
int	info;
int	debug;
int	secure_mode;
char	*extra_options;
char	*info_string;
char	*btree_dbname;

int	extractmethod;
int	total;

static void
usage()
{
	if (!qflag)
		fputs(usage_const, stderr);
	exit(2);
}
static void
help()
{
	fputs(usage_const, stdout);
	fputs(help_const, stdout);
	exit(0);
}

static struct option const long_options[] = {
	{"absolute", no_argument, &do_absolute, 1},
	{"compact", no_argument, NULL, 'c'},
	{"cxref", no_argument, &cxref, 1},
	{"incremental", no_argument, NULL, 'i'},
	{"omit-gsyms", no_argument, NULL, 'o'},
	{"quiet", no_argument, NULL, 'q'},
	{"verbose", no_argument, NULL, 'v'},
	{"warning", no_argument, NULL, 'w'},

	/* long name only */
	{"config", optional_argument, &show_config, 1},
	{"convert", no_argument, &do_convert, 1},
	{"createdb", required_argument, &do_createdb, 1},
	{"date", no_argument, &do_date, 1},
	{"debug", no_argument, &debug, 1},
	{"expand", required_argument, &do_expand, 1},
	{"find", no_argument, &do_find, 1},
	{"gtagsconf", required_argument, &gtagsconf, 1},
	{"gtagslabel", required_argument, &gtagslabel, 1},
	{"idutils", no_argument, NULL, 'I'},
	{"info", required_argument, &info, 1},
	{"other", no_argument, &other_files, 1},
	{"postgres", optional_argument, NULL, 'P'},
	{"pwd", no_argument, &do_pwd, 1},
	{"readdb", required_argument, &do_readdb, 1},
	{"relative", no_argument, &do_relative, 1},
	{"scandb", required_argument, &do_scandb, 1},
	{"secure", no_argument, &secure_mode, 1},
	{"sort", no_argument, &do_sort, 1},
	{"write", no_argument, &do_write, 1},
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
match(curtag, line)
char *curtag;
char *line;
{
	char *p, *q = line;

	for (p = curtag; *p; p++)
		if (*p != *q++)
			return 0;
	if (!isspace(*q))
		return 0;
	return 1;
}

int
main(argc, argv)
int	argc;
char	*argv[];
{
	char	root[MAXPATHLEN+1];
	char	dbpath[MAXPATHLEN+1];
	char	cwd[MAXPATHLEN+1];
	STRBUF	*sb = strbuf_open(0);
	const char *p;
	int	db;
	int	optchar;
	int	option_index = 0;

	while ((optchar = getopt_long(argc, argv, "cGiIoPqvw", long_options, &option_index)) != EOF) {
		switch (optchar) {
		case 0:
			p = long_options[option_index].name;
			if (!strcmp(p, "expand")) {
				settabs(atoi(optarg + 1));
			} else if (!strcmp(p, "info")) {
				info_string = optarg;
			} else if (!strcmp(p, "config")) {
				if (optarg)
					info_string = optarg;
			} else if (!strcmp(p, "createdb") || !strcmp(p, "readdb") || !strcmp(p, "scandb")) {
				if (optarg)
					btree_dbname = optarg;
			} else if (gtagsconf || gtagslabel) {
				char    value[MAXPATHLEN+1];
				char	*name = (gtagsconf) ? "GTAGSCONF" : "GTAGSLABEL";

				if (gtagsconf) {
					if (realpath(optarg, value) == NULL)
						die("%s not found.", optarg);
				} else {
					strlimcpy(value, optarg, sizeof(value));
				}
				set_env(name, value);
				gtagsconf = gtagslabel = 0;
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
		case 'P':
			Pflag++;
			/* pass info string to PQconnectdb(3) */
			if (optarg)
				gtags_setinfo(optarg);
			break;
		case 'q':
			qflag++;
			setquiet();
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
	if (qflag)
		vflag = 0;
#ifndef USE_POSTGRES
	if (Pflag)
		die_with_code(2, "The -P option not available. Please configure GLOBAL with --with-postgres and rebuild it.");
#endif
	if (Pflag) {
		/* for backward compatibility */
		if (info_string)
			gtags_setinfo(info_string);
	}
	if (show_version)
		version(NULL, vflag);
	if (show_help)
		help();

	argc -= optind;
        argv += optind;

	if (show_config) {
		if (!info_string && argc)
			info_string = argv[0];
		if (info_string) {
			printconf(info_string);
		} else {
			fprintf(stdout, "%s\n", getconfline());
		}
		exit(0);
	} else if (do_convert) {
		STRBUF *ib = strbuf_open(MAXBUFLEN);
		char *p, *q, *fid;

		/*
		 * [Job]
		 *
		 * Read line from stdin and replace " ./<file name> "
		 * with the file number like this.
		 *
		 * <A HREF="http://xxx/global/S/ ./main.c .html#110">main</A>\n
		 *				|
		 *				v
		 * <A HREF="http://xxx/global/S/39.html#110">main</A>\n
		 *
		 * If the file name is not found in GPATH, change into the path to CGI script.
		 * <A HREF="http://xxx/global/S/ ./README .html#9">main</A>\n
		 *				|
		 *				v
		 * <A HREF="http://xxx/global/cgi-bin/global.cgi?pattern=README&type=source#9">main</A>\n
		 */
		if (gpath_open(".", 0, 0) < 0)
			die("GPATH not found.");
		while (strbuf_fgets(ib, stdin, 0) != NULL) {
			p = strbuf_value(ib);
			if (strncmp("<A ", p, 3))
				continue;
			q = locatestring(p, "/S/ ", MATCH_FIRST);
			if (q == NULL) {
				printf("%s: ERROR(1): %s", progname, strbuf_value(ib));
				continue;
			}
			/* Print just before "/S/ " and skip "/S/ ". */
			for (; p < q; p++)
				putc(*p, stdout);
			for (; *p && *p != ' '; p++)
				;
			/* Extract path name. */
			for (q = ++p; *q && *q != ' '; q++)
				;
			if (*q == '\0') {
				printf("%s: ERROR(2): %s", progname, strbuf_value(ib));
				continue;
			}
			*q++ = '\0';
			/*
			 * Convert path name into URL.
			 * The output of 'global -xgo' may include lines about
			 * files other than source code. In this case, file id
			 * doesn't exist in GPATH.
			 */
			fid = gpath_path2fid(p);
			if (fid) {
				fputs("/S/", stdout);
				fputs(fid, stdout);
				fputs(q, stdout);
			} else {
				fputs("/cgi-bin/global.cgi?pattern=", stdout);
				fputs(p + 2, stdout);
				fputs("&type=source", stdout);
				for (; *q && *q != '#'; q++)
					;
				if (*q == '\0') {
					printf("%s: ERROR(2): %s", progname, strbuf_value(ib));
					continue;
				}
				fputs(q, stdout);
			}
		}
		gpath_close();
		strbuf_close(ib);
		exit(0);
	} else if (do_createdb) {
		DBOP *dbop;
		STRBUF *ib = strbuf_open(MAXBUFLEN);
		char *p, *q;

		/*
		 * [Job]
		 *
		 * Read line from stdin and write to btree database.
		 * Format: [0-9]+ [0-9]+[MDRY][a-zA-Z_0-9]+
		 */
		dbop = dbop_open(btree_dbname, 1, 0644, DBOP_DUP);
		if (dbop == NULL)
			die("cannot create '%s'.", btree_dbname);
		while (strbuf_fgets(ib, stdin, STRBUF_NOCRLF) != NULL) {
			if (exitflag)
				break;
			p = q = strbuf_value(ib);
			for (; *q && isdigit(*q); q++)	/* key part */
				;
			if (*q == 0)
				die("data part not found.");
			*q++ = '\0';
			if (*q == 0)
				die("data part is null.");
			dbop_put(dbop, p, q, 0);
		}
		dbop_close(dbop);
		strbuf_close(ib);
		exit(0);
	} else if (do_readdb) {
		DBOP *dbop;
		char *p;

		/*
		 * [Job]
		 *
		 * Read specified key records from btree database and write to stdout.
		 */
		if (argc == 0)
			die("usage: %s --readdb=dbname key", progname);
		dbop = dbop_open(btree_dbname, 0, 0, 0);
		if (dbop == NULL)
			die("cannot open '%s'.", btree_dbname);
		for (p = dbop_first(dbop, argv[0], NULL, 0); p; p = dbop_next(dbop)) {
			fputs(p, stdout);
			fputc('\n', stdout);
		}
		dbop_close(dbop);
		exit(0);
	} else if (do_scandb) {
		DBOP *dbop;
		char *p;

		/*
		 * [Job]
		 *
		 * Load GPATH record.
		 */
		if (argc == 0)
			die("usage: %s --scandb=dbname prefix", progname);
		dbop = dbop_open(btree_dbname, 0, 0, 0);
		if (dbop == NULL)
			die("cannot open '%s'.", btree_dbname);
		for (p = dbop_first(dbop, "./", NULL, DBOP_PREFIX | DBOP_KEY); p; p = dbop_next(dbop))
			fprintf(stdout, "%s %s\n", p, dbop_lastdat(dbop));
		dbop_close(dbop);
		exit(0);
	} else if (do_expand) {
		FILE *ip;
		STRBUF *ib = strbuf_open(MAXBUFLEN);

		if (argc) {
			if (secure_mode) {
				char	buf[MAXPATHLEN+1], *path;
				getdbpath(cwd, root, dbpath, 0);
				path = realpath(argv[0], buf);
				if (path == NULL)
					die("realpath(%s, buf) failed. (errno=%d).", argv[0], errno);
				if (!isabspath(path))
					die("realpath(3) is not compatible with BSD version.");
				if (strncmp(path, root, strlen(root)))
					die("'%s' is out of source tree.\n", path);
			}
			ip = fopen(argv[0], "r");
			if (ip == NULL)
				exit(1);
		} else
			ip = stdin;
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
	} else if (do_find) {
		char	*path;
		char *local = (argc) ? argv[0] : NULL;

		for (vfind_open(local, other_files); (path = vfind_read()) != NULL; ) {
			fputs(path, stdout);
			fputc('\n', stdout);
		}
		vfind_close();
		exit(0);
	} else if (do_sort) {
		/*
		 * This is GLOBAL version of sort.
		 * We replace sort with this procedure for performance.
		 */
		int unit = 1500;
		STRBUF *ib = strbuf_open(MAXBUFLEN);
		char *line = strbuf_fgets(ib, stdin, 0);

		while (line != NULL) {
			int count = 0;
			FILE *op = popen("sort +0 -1 +2 -3 +1n -2 -u", "w");
			do {
				fputs(line, op);
			} while ((line = strbuf_fgets(ib, stdin, 0)) != NULL && ++count < unit);
			if (line) {
				/* curtag = current tag name */
				STRBUF *curtag = strbuf_open(0);
				char *p = line;
				while (!isspace(*p))
					strbuf_putc(curtag, *p++);
				/* read until next tag name */
				do {
					fputs(line, op);
				} while ((line = strbuf_fgets(ib, stdin, 0)) != NULL
					&& match(strbuf_value(curtag), line));
			}
			pclose(op);
		}
		exit(0);
	} else if (do_write) {
		FILE *op = NULL;
		STRBUF *ib = strbuf_open(MAXBUFLEN);
		int writing = 0;
		int pipe = 0;

		while (strbuf_fgets(ib, stdin, STRBUF_NOCRLF) != NULL) {
			char *p = strbuf_value(ib);
			if (*p == '<' || *p == '\t') {
				strbuf_putc(ib, '\n');
				fputs(p, op);
			} else {
				if (writing) {
					if (pipe)
						(void)pclose(op);
					else
						(void)fclose(op);
				}
				pipe = 0;
				if (*p == 'N') {
					p++;
					if ((op = fopen(p, "w")) == NULL)
						die("cannot create file '%s'.", p);
				} else if (*p == 'C') {
					char command[MAXPATHLEN];
					p++;
					snprintf(command, sizeof(command), "gzip -c > %s", p);
					if ((op = popen(command, "w")) == NULL)
						die("cannot create file '%s'.", p);
					pipe = 1;
				} else {
					die("illegal internal file format (--write).");
				}
				writing = 1;
			}
		}
		if (writing) {
			if (pipe)
				(void)pclose(op);
			else
				(void)fclose(op);
		}
		strbuf_close(ib);
		exit(0);
	} else if (do_relative || do_absolute) {
		STRBUF *ib = strbuf_open(MAXBUFLEN);
		char	*root = argv[0];
		char	*cwd = argv[1];

		if (argc < 2)
			die("do_relative: 2 arguments needed.");
		set_base_directory(root, cwd);
		while (strbuf_fgets(ib, stdin, 0) != NULL)
			put_converting(strbuf_value(ib), do_absolute ? 1 : 0, cxref);
		strbuf_close(ib);
		exit(0);
	} else if (Iflag) {
		if (!usable("mkid"))
			die("mkid not found.");
	}

	/*
	 * Check whether or not your system has GLOBAL's gctags.
	 * Some GNU/Linux distributions rename emacs's ctags to gctags!
	 */
	{
		FILE *ip = popen("gctags --check", "r");
		STRBUF *ib = strbuf_open(MAXBUFLEN);
		if (strbuf_fgets(ib, ip, STRBUF_NOCRLF) == NULL || strcmp(strbuf_value(ib), "Part of GLOBAL")) {
			if (!qflag) {
				fprintf(stderr, "Warning: gctags in your system is not GLOBAL's one.\n");
				fprintf(stderr, "Please type 'gctags --version'\n");
			}
		}
		strbuf_close(ib);
		pclose(ip);
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
			strlimcpy(dbpath, cwd, sizeof(dbpath));
	} else {
		if (argc > 0)
			realpath(*argv, dbpath);
		else
			strlimcpy(dbpath, cwd, sizeof(dbpath));
	}
	if (iflag && (!test("f", makepath(dbpath, dbname(GTAGS), NULL)) ||
		!test("f", makepath(dbpath, dbname(GPATH), NULL)))) {
		if (wflag)
			fprintf(stderr, "Warning: GTAGS or GPATH not found. -i option ignored.\n");
		iflag = 0;
	}
	if (Pflag && iflag) {
		if (wflag)
			fprintf(stderr, "Warning: existing tag files are used. -P option ignored.\n");
		Pflag = 0;
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
	if (cflag == 0 && getconfs("format", sb) && !strcmp(strbuf_value(sb), "compact"))
		cflag++;
	/*
	 * teach gctags(1) where is dbpath by environment variable.
	 */
	set_env("GTAGSDBPATH", dbpath);

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
		GTOP *gtop = gtags_open(dbpath, cwd, GTAGS, GTAGS_MODIFY, 0);
		gtags_close(gtop);
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
		/*
		 * get parser for db. (gctags by default)
		 */
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
	 * create id-utils index.
	 */
	if (Iflag) {
		if (vflag)
			fprintf(stderr, "[%s] Creating indexes for id-utils.\n", now());
		strbuf_reset(sb);
		strbuf_puts(sb, "mkid");
		if (vflag)
			strbuf_puts(sb, " -v");
		if (vflag) {
#ifdef __DJGPP__
			if (is_unixy())	/* test for 4DOS as well? */
#endif
			strbuf_puts(sb, " 1>&2");
		} else {
			strbuf_puts(sb, " >/dev/null");
		}
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

	if (gpath_open(dbpath, 0, 0) < 0)
		die("GPATH not found.");
	/*
	 * make add list and update list.
	 */
	for (find_open(NULL); (path = find_read()) != NULL; ) {
		/* a blank at the head of path means 'NOT SOURCE'. */
		if (*path == ' ')
			continue;
		if (locatestring(path, " ", MATCH_FIRST)) {
			if (!qflag)
				fprintf(stderr, "Warning: '%s' ignored, because it includes blank in the path.\n", path);
			continue;
		}
		if (stat(path, &statp) < 0)
			die("stat failed '%s'.", path);
		if (!gpath_path2fid(path))
			strbuf_puts0(addlist, path);
		else if (gtags_mtime < statp.st_mtime)
			strbuf_puts0(updatelist, path);
	}
	find_close();
	/*
	 * make delete list.
	 */
	{
		char    fid[32];
		int i, limit = gpath_nextkey();

		for (i = 1; i < limit; i++) {
			snprintf(fid, sizeof(fid), "%d", i);
			if ((path = gpath_fid2path(fid)) == NULL)
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

		gpath_open(dbpath, 2, 0);
		for (p = start; p < end; p += strlen(p) + 1) {
			if (exitflag)
				break;
			gpath_delete(p);
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
		gtop = gtags_open(dbpath, root, db, GTAGS_MODIFY, 0);
		if (type != 1)
			gtags_delete(gtop, path);
		if (vflag)
			fprintf(stderr, "..");
		if (type != 2) {
			if (extractmethod)
				gflags |= GTAGS_EXTRACTMETHOD;
			if (debug)
				gflags |= GTAGS_DEBUG;
			if (Pflag)
				gflags |= GTAGS_POSTGRES;
			gtags_add(gtop, strbuf_value(sb), path, gflags);
		}
		gtags_close(gtop);
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
	/*
	 * Compact format:
	 *
	 * -c: COMPACT format.
	 * -cc: PATHINDEX format.
	 * Ths -cc is undocumented.
	 * In the future, it may become the standard format of GLOBAL.
	 */
	if (cflag) {
		flags |= GTAGS_PATHINDEX;
		if (cflag == 1)
			flags |= GTAGS_COMPACT;
	}
	if (Pflag) {
		flags |= GTAGS_POSTGRES;
	}
	strbuf_reset(sb);
	if (vflag > 1 && getconfs(dbname(db), sb))
		fprintf(stderr, " using tag command '%s <path>'.\n", strbuf_value(sb));
	gtop = gtags_open(dbpath, root, db, GTAGS_CREATE, flags);
	for (find_open(NULL); (path = find_read()) != NULL; ) {
		int	gflags = 0;
		int	skip = 0;

		/* a blank at the head of path means 'NOT SOURCE'. */
		if (*path == ' ')
			continue;
		if (exitflag)
			break;
		if (locatestring(path, " ", MATCH_FIRST)) {
			if (!qflag)
				fprintf(stderr, "Warning: '%s' ignored, because it includes blank in the path.\n", path + 2);
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
			fprintf(stderr, " extracting tags of %s", path + 2);
			if (skip)
				fprintf(stderr, " (skipped)");
			fputc('\n', stderr);
		}
		if (skip)
			continue;
		if (extractmethod)
			gflags |= GTAGS_EXTRACTMETHOD;
		if (debug)
			gflags |= GTAGS_DEBUG;
		if (Pflag)
			gflags |= GTAGS_POSTGRES;
		gtags_add(gtop, comline, path, gflags);
	}
	total = count;				/* save total count */
	find_close();
	gtags_close(gtop);
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

	strlimcpy(buf, "unkown time", sizeof(buf));
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
/*
 * put_converting: convert path into relative or absolute and print.
 *
 *	i)	line	raw output from global(1)
 *	i)	absolute 1: absolute, 0: relative
 *	i)	cxref 1: -x format, 0: file name only
 */
static STRBUF *abspath;
static char basedir[MAXPATHLEN+1];
static int start_point;
void
set_base_directory(root, cwd)
char	*root;
char	*cwd;
{
	abspath = strbuf_open(MAXPATHLEN);
	strbuf_puts(abspath, root);
	strbuf_unputc(abspath, '/');
	start_point = strbuf_getlen(abspath);

	if (strlen(cwd) > MAXPATHLEN)
		die("current directory name too long.");
	strlimcpy(basedir, cwd, sizeof(basedir));
	/* leave abspath unclosed. */
}
void
put_converting(line, absolute, cxref)
char	*line;
int	absolute;
int	cxref;
{
	char buf[MAXPATHLEN+1];
	char *p = line;

	/*
	 * print until path name.
	 */
	if (cxref) {
		/* print tag name */
		for (; *p && !isspace(*p); p++)
			(void)putc(*p, stdout);
		/* print blanks and line number */
		for (; *p && *p != '.'; p++)
			(void)putc(*p, stdout);
	}
	if (*p++ == '\0')
		return;
	/*
	 * make absolute path.
	 */
	strbuf_setlen(abspath, start_point);
	for (; *p && !isspace(*p); p++)
		strbuf_putc(abspath, *p);
	/*
	 * put path with converting.
	 */
	if (absolute) {
		(void)fputs(strbuf_value(abspath), stdout);
	} else {
		char *a = strbuf_value(abspath);
		char *b = basedir;
#if defined(_WIN32) || defined(__DJGPP__)
		/* skip drive char in 'c:/usr/bin' */
		while (*a != '/')
			a++;
		while (*b != '/')
			b++;
#endif
		if (!abs2rel(a, b, buf, sizeof(buf)))
			die("abs2rel failed. (path=%s, base=%s).", a, b);
		(void)fputs(buf, stdout);
	}
	/*
	 * print the rest of the record.
	 */
	(void)fputs(p, stdout);
}
