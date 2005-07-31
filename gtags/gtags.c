/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005
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
#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
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

static void usage(void);
static void help(void);
void signal_setup(void);
void onintr(int);
int match(const char *, const char *);
int main(int, char **);
int incremental(const char *, const char *);
void updatetags(const char *, const char *, IDSET *, STRBUF *, int, int);
void createtags(const char *, const char *, int);
int printconf(const char *);
void set_base_directory(const char *, const char *);
void put_converting(const char *, int, int);

int cflag;					/* compact format */
int iflag;					/* incremental update */
int Iflag;					/* make  id-utils index */
int oflag;					/* suppress making GSYMS */
int qflag;					/* quiet mode */
int wflag;					/* warning message */
int vflag;					/* verbose mode */
int max_args;
int show_version;
int show_help;
int show_config;
int do_convert;
int do_find;
int do_sort;
int do_relative;
int do_absolute;
int cxref;
int do_expand;
int gtagsconf;
int gtagslabel;
int other_files;
int debug;
int secure_mode;
const char *extra_options;
const char *info_string;

int extractmethod;
int total;

static void
usage(void)
{
	if (!qflag)
		fputs(usage_const, stderr);
	exit(2);
}
static void
help(void)
{
	fputs(usage_const, stdout);
	fputs(help_const, stdout);
	exit(0);
}

static struct option const long_options[] = {
	{"absolute", no_argument, &do_absolute, 1},
	{"compact", no_argument, NULL, 'c'},
	{"cxref", no_argument, &cxref, 1},
	{"idutils", no_argument, NULL, 'I'},
	{"incremental", no_argument, NULL, 'i'},
	{"max-args", required_argument, NULL, 'n'},
	{"omit-gsyms", no_argument, NULL, 'o'},
	{"quiet", no_argument, NULL, 'q'},
	{"verbose", no_argument, NULL, 'v'},
	{"warning", no_argument, NULL, 'w'},

	/* long name only */
	{"config", optional_argument, &show_config, 1},
	{"convert", no_argument, &do_convert, 1},
	{"debug", no_argument, &debug, 1},
	{"expand", required_argument, &do_expand, 1},
	{"find", no_argument, &do_find, 1},
	{"gtagsconf", required_argument, &gtagsconf, 1},
	{"gtagslabel", required_argument, &gtagslabel, 1},
	{"other", no_argument, &other_files, 1},
	{"relative", no_argument, &do_relative, 1},
	{"secure", no_argument, &secure_mode, 1},
	{"sort", no_argument, &do_sort, 1},
	{"version", no_argument, &show_version, 1},
	{"help", no_argument, &show_help, 1},
	{ 0 }
};

/*
 * Gtags catch signal even if the parent ignore it.
 */
int exitflag = 0;

static const char *langmap = DEFAULTLANGMAP;

void
onintr(signo)
	int signo;
{
	signo = 0;      /* to satisfy compiler */
	exitflag = 1;
}

void
signal_setup(void)
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
	const char *curtag;
	const char *line;
{
	const char *p, *q = line;

	for (p = curtag; *p; p++)
		if (*p != *q++)
			return 0;
	if (!isspace((unsigned char)*q))
		return 0;
	return 1;
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char root[MAXPATHLEN+1];
	char dbpath[MAXPATHLEN+1];
	char cwd[MAXPATHLEN+1];
	STRBUF *sb = strbuf_open(0);
	const char *p;
	int db;
	int optchar;
	int option_index = 0;

	while ((optchar = getopt_long(argc, argv, "cGiIn:oPqvw", long_options, &option_index)) != EOF) {
		switch (optchar) {
		case 0:
			p = long_options[option_index].name;
			if (!strcmp(p, "expand")) {
				settabs(atoi(optarg + 1));
			} else if (!strcmp(p, "config")) {
				if (optarg)
					info_string = optarg;
			} else if (gtagsconf || gtagslabel) {
				char value[MAXPATHLEN+1];
				const char *name = (gtagsconf) ? "GTAGSCONF" : "GTAGSLABEL";

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
		case 'n':
			max_args = atoi(optarg);
			if (max_args <= 0)
				die("--max-args option requires number > 0.");
			break;
		case 'o':
			oflag++;
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
		const char *fid;
		char *p, *q;
		int c;

		/*
		 * [Job]
		 *
		 * Read line from stdin and replace " ./<file name> "
		 * with the file number like this.
		 *
		 * <a href='http://xxx/global/S/ ./main.c .html#110'>main</a>\n
		 *				|
		 *				v
		 * <a href='http://xxx/global/S/39.html#110'>main</a>\n
		 *
		 * If the file name is not found in GPATH, change into the path to CGI script.
		 * <a href='http://xxx/global/S/ ./README .html#9'>main</a>\n
		 *				|
		 *				v
		 * <a href='http://xxx/global/cgi-bin/global.cgi?pattern=README&amp;type=source#9'>main</a>\n
		 */
		if (gpath_open(".", 0) < 0)
			die("GPATH not found.");
		while (strbuf_fgets(ib, stdin, 0) != NULL) {
			p = strbuf_value(ib);
			if (strncmp("<a ", p, 3))
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
				p += 2;
				while ((c = (unsigned char)*p++) != '\0') {
					if (isalnum(c))
						putc(c, stdout);
					else
						printf("%%%02x", c);
				}
				fputs("&amp;type=source", stdout);
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
	} else if (do_expand) {
		/*
		 * The 'gtags --expand' is nearly equivalent with 'expand'.
		 * We made this command to decrease dependency to external
		 * command. But now, the --secure option use this command
		 * positively.
		 */
		FILE *ip;
		STRBUF *ib = strbuf_open(MAXBUFLEN);

		if (argc) {
			if (secure_mode) {
				char buf[MAXPATHLEN+1], *path;
				size_t rootlen;

				getdbpath(cwd, root, dbpath, 0);
				path = realpath(argv[0], buf);
				if (path == NULL)
					die("realpath(%s, buf) failed. (errno=%d).", argv[0], errno);
				if (!isabspath(path))
					die("realpath(3) is not compatible with BSD version.");
				rootlen = strlen(root);
				if (strncmp(path, root, rootlen) || path[rootlen] != '/')
					die("'%s' is out of source tree.", path);
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
	} else if (do_find) {
		/*
		 * This code is used by htags(1) to traverse file system.
		 *
		 * If the --other option is not specified, 'gtags --find'
		 * read GPATH instead of traversing file. But if the option
		 * is specified, it traverse file system every time.
		 * It is because gtags doesn't record the paths other than
		 * source file in GPATH.
		 * Since it is slow, gtags should record not only source
		 * files but also other files in GPATH in the future.
		 * But it needs adding a new format version.
		 */
		const char *path;
		const char *local = (argc) ? argv[0] : NULL;

		for (vfind_open(local, other_files); (path = vfind_read()) != NULL; ) {
			fputs(path, stdout);
			fputc('\n', stdout);
		}
		vfind_close();
		exit(0);
	} else if (do_sort) {
		/*
		 * This code and the makedupindex() in htags(1) compose
		 * a pipeline 'global -x ".*" | gtags --sort'.
		 * The 'gtags --sort' is equivalent with 'sort -k 1,1 -k 3,3 -k 2,2n -u'
		 * but the latter is ineffective and needs a lot of temporary
		 * files when applied to a huge file. (According to circumstances,
		 * hundreds of files are generated.)
		 *
		 * Utilizing the feature that the output of 'global -x ".*"'
		 * is already sorted in alphabetical order by tag name,
		 * we splited the output into relatively small unit and
		 * execute sort for each unit.
		 *
		 * It is not certain whether the present unit value is the best.
		 */
		int unit = 1500;
		STRBUF *ib = strbuf_open(MAXBUFLEN);
		const char *ctags_x = strbuf_fgets(ib, stdin, 0);

		while (ctags_x != NULL) {
			int count = 0;
			FILE *op = popen("gnusort -k 1,1 -k 3,3 -k 2,2n -u", "w");
			do {
				fputs(ctags_x, op);
			} while ((ctags_x = strbuf_fgets(ib, stdin, 0)) != NULL && ++count < unit);
			if (ctags_x) {
				/* curtag = current tag name */
				STRBUF *curtag = strbuf_open(0);
				const char *p = ctags_x;
				while (!isspace((unsigned char)*p))
					strbuf_putc(curtag, *p++);
				/* read until next tag name */
				do {
					fputs(ctags_x, op);
				} while ((ctags_x = strbuf_fgets(ib, stdin, 0)) != NULL
					&& match(strbuf_value(curtag), ctags_x));
			}
			if (pclose(op) != 0)
				die("terminated abnormally.");
		}
		exit(0);
	} else if (do_relative || do_absolute) {
		/*
		 * This is the main body of path filter.
		 * This code extract path name from tag line and
		 * replace it with the relative or the absolute path name.
		 *
		 * By default, if we are in src/ directory, the output
		 * should be converted like follws:
		 *
		 * main      10 ./src/main.c  main(argc, argv)\n
		 * main      22 ./libc/func.c   main(argc, argv)\n
		 *		v
		 * main      10 main.c  main(argc, argv)\n
		 * main      22 ../libc/func.c   main(argc, argv)\n
		 *
		 * Similarly, the --absolute option specified, then
		 *		v
		 * main      10 /prj/xxx/src/main.c  main(argc, argv)\n
		 * main      22 /prj/xxx/libc/func.c   main(argc, argv)\n
		 */
		STRBUF *ib = strbuf_open(MAXBUFLEN);
		const char *root = argv[0];
		const char *cwd = argv[1];
		const char *ctags_x;

		if (argc < 2)
			die("do_relative: 2 arguments needed.");
		set_base_directory(root, cwd);
		while ((ctags_x = strbuf_fgets(ib, stdin, 0)) != NULL)
			put_converting(ctags_x, do_absolute ? 1 : 0, cxref);
		strbuf_close(ib);
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
			warning("GTAGS or GPATH not found. -i option ignored.");
		iflag = 0;
	}
	if (!test("d", dbpath))
		die("directory '%s' not found.", dbpath);
	if (vflag)
		fprintf(stderr, "[%s] Gtags started.\n", now());
	/*
	 * load .globalrc or /etc/gtags.conf
	 */
	openconf();
	if (getconfb("extractmethod"))
		extractmethod = 1;
	strbuf_reset(sb);
	if (cflag == 0 && getconfs("format", sb) && !strcmp(strbuf_value(sb), "compact"))
		cflag++;
	/*
	 * Pass the following information to gtags-parser(1)
	 * using environment variable.
	 *
	 * o langmap
	 * o DBPATH
	 */
	strbuf_reset(sb);
	if (getconfs("langmap", sb)) {
		const char *p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		langmap = p;
	}
	set_env("GTAGSLANGMAP", langmap);
	set_env("GTAGSDBPATH", dbpath);

	if (wflag)
		set_env("GTAGSWARNING", "1");
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
		 * get parser for db. (gtags-parser by default)
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
	const char *dbpath;
	const char *root;
{
	struct stat statp;
	time_t gtags_mtime;
	STRBUF *addlist = strbuf_open(0);
	STRBUF *deletelist = strbuf_open(0);
	IDSET *deleteset;
	int updated = 0;
	int addtotal = 0;
	const char *path;

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
	deleteset = idset_open(gpath_nextkey());
	/*
	 * make add list and update list.
	 */
	for (find_open(NULL); (path = find_read()) != NULL; ) {
		const char *fid;

		/* a blank at the head of path means 'NOT SOURCE'. */
		if (*path == ' ')
			continue;
		if (stat(path, &statp) < 0)
			die("stat failed '%s'.", path);
		if ((fid = gpath_path2fid(path)) == NULL) {
			strbuf_puts0(addlist, path);
			addtotal++;
		} else if (gtags_mtime < statp.st_mtime) {
			strbuf_puts0(addlist, path);
			addtotal++;
			idset_add(deleteset, atoi(fid));
		}
	}
	find_close();
	/*
	 * make delete list.
	 */
	{
		char fid[32];
		int i, limit = gpath_nextkey();

		for (i = 1; i < limit; i++) {
			snprintf(fid, sizeof(fid), "%d", i);
			if ((path = gpath_fid2path(fid)) == NULL)
				continue;
			if (!test("f", path)) {
				strbuf_puts0(deletelist, path);
				idset_add(deleteset, i);
			}
		}
	}
	gpath_close();
	if (strbuf_getlen(addlist) + strbuf_getlen(deletelist))
		updated = 1;
	/*
	 * execute updating.
	 */
	signal_setup();
	if (updated) {
		int db;

		for (db = GTAGS; db < GTAGLIM; db++) {
			/*
			 * GTAGS needed at least.
			 */
			if ((db == GRTAGS || db == GSYMS)
			    && !test("f", makepath(dbpath, dbname(db), NULL)))
				continue;
			if (vflag)
				fprintf(stderr, "[%s] Updating '%s'.\n", now(), dbname(db));
			updatetags(dbpath, root, deleteset, addlist, addtotal, db);
			if (exitflag)
				exit(1);
		}
	}
	if (strbuf_getlen(deletelist) > 0) {
		const char *start = strbuf_value(deletelist);
		const char *end = start + strbuf_getlen(deletelist);
		const char *p;

		gpath_open(dbpath, 2);
		for (p = start; p < end; p += strlen(p) + 1) {
			if (exitflag)
				break;
			gpath_delete(p);
		}
		gpath_close();
	}
	if (exitflag)
		exit(1);
	if (updated) {
		int db;
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
	idset_close(deleteset);

	return updated;
}
/*
 * updatetags: update tag file.
 *
 *	i)	dbpath		directory in which tag file exist
 *	i)	root		root directory of source tree
 *	i)	deleteset	bit array of fid of deleted or modified files 
 *	i)	addlist		\0 separated list of added or modified files
 *	i)	addtotal	number of files in addlist
 *	i)	db		GTAGS, GRTAGS, GSYMS
 */
void
updatetags(dbpath, root, deleteset, addlist, addtotal, db)
	const char *dbpath;
	const char *root;
	IDSET *deleteset;
	STRBUF *addlist;
	int addtotal;
	int db;
{
	GTOP *gtop;
	STRBUF *comline = strbuf_open(0);
	int gflags;
	int path_list_max;
	int arg_count = 0;

	/*
	 * GTAGS needed to make GRTAGS.
	 */
	if (db == GRTAGS && !test("f", makepath(dbpath, dbname(GTAGS), NULL)))
		die("GTAGS needed to create GRTAGS.");

	/*
	 * get tag command.
	 */
	if (!getconfs(dbname(db), comline))
		die("cannot get tag command. (%s)", dbname(db));
	/*
	 * determine the maximum length of the list of paths.
	 */
	path_list_max = exec_line_limit();
	path_list_max -= strbuf_getlen(comline);
	path_list_max -= 40;
	if (path_list_max < 0)
		path_list_max = 0;

	gtop = gtags_open(dbpath, root, db, GTAGS_MODIFY, 0);
	if (vflag) {
		char fid[32];
		const char *path;
		int seqno = 1;
		int total = idset_count(deleteset);
		int i;

		for (i = 0; i < deleteset->max; i++) {
			if (idset_contains(deleteset, i)) {
				snprintf(fid, sizeof(fid), "%d", i);
				path = gpath_fid2path(fid);
				if (path == NULL)
					die("GPATH is corrupted.");
				fprintf(stderr, " [%d/%d] deleting tags of %s\n", seqno++, total, path + 2);
			}
		}
	}
	if (deleteset->max > 0)
		gtags_delete(gtop, deleteset);
	gflags = 0;
	if (extractmethod)
		gflags |= GTAGS_EXTRACTMETHOD;
	if (debug)
		gflags |= GTAGS_DEBUG;
	/*
	 * If the --max-args option is not specified, we pass the parser
	 * the source file as a lot as possible to decrease the invoking
	 * frequency of the parser.
	 */
	{
		STRBUF *path_list = strbuf_open(0);
		const char *path = strbuf_value(addlist);
		const char *end = path + strbuf_getlen(addlist);
		int seqno = 1;

		while (path < end) {
			int pathlen = strlen(path);

			if (vflag)
				fprintf(stderr, " [%d/%d] adding tags of %s\n", seqno++, addtotal, path + 2);
			/*
			 * Execute parser when path name collects enough.
			 * Though the path_list is \0 separated list of path,
			 * we can think its length equals to the length of
			 * argument string because each \0 can be replaced
			 * with a blank.
			 */
			if (strbuf_getlen(path_list)) {
				if (path_list_max == 0 ||
				    (max_args > 0 && arg_count >= max_args) ||
				    strbuf_getlen(path_list) + pathlen > path_list_max)
				{
					gtags_add(gtop, strbuf_value(comline), path_list, gflags);
					strbuf_reset(path_list);
					arg_count = 0;
				}
			}
			if (exitflag)
				break;
			/*
			 * Add a path to the path list.
			 */
			strbuf_puts0(path_list, path);
			path += pathlen + 1;
			arg_count++;
		}
		if (strbuf_getlen(path_list))
			gtags_add(gtop, strbuf_value(comline), path_list, gflags);
		strbuf_close(path_list);
	}

	gtags_close(gtop);
	strbuf_close(comline);
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
	const char *dbpath;
	const char *root;
	int db;
{
	const char *path;
	GTOP *gtop;
	int flags, gflags;
	STRBUF *comline = strbuf_open(0);
	int count = 0;
	int arg_count = 0;
	STRBUF *path_list = strbuf_open(MAXPATHLEN);
	int path_list_max;

	/*
	 * get tag command.
	 */
	if (!getconfs(dbname(db), comline))
		die("cannot get tag command. (%s)", dbname(db));
	/*
	 * GTAGS needed to make GRTAGS.
	 */
	if (db == GRTAGS && !test("f", makepath(dbpath, dbname(GTAGS), NULL)))
		die("GTAGS needed to create GRTAGS.");

	/*
	 * determine the maximum length of the list of paths.
	 */
	path_list_max = exec_line_limit();
	path_list_max -= strbuf_getlen(comline);
	path_list_max -= 40;
	if (path_list_max < 0)
		path_list_max = 0;;

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
	if (vflag > 1)
		fprintf(stderr, " using tag command '%s <path>'.\n", strbuf_value(comline));
	gtop = gtags_open(dbpath, root, db, GTAGS_CREATE, flags);
	gflags = 0;
	if (extractmethod)
		gflags |= GTAGS_EXTRACTMETHOD;
	if (debug)
		gflags |= GTAGS_DEBUG;
	/*
	 * If the --max-args option is not specified, we pass the parser
	 * the source file as a lot as possible to decrease the invoking
	 * frequency of the parser.
	 */
	for (find_open(NULL); (path = find_read()) != NULL; ) {
		int skip = 0;

		/* a blank at the head of path means 'NOT SOURCE'. */
		if (*path == ' ')
			continue;
		if (exitflag)
			break;
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
		/*
		 * Execute parser when path name collects enough.
		 * Though the path_list is \0 separated list of string,
		 * we can think its length equals to the length of
		 * argument string because each \0 can be replaced
		 * with a blank.
		 */
		if (strbuf_getlen(path_list)) {
			if (path_list_max == 0 ||
			    (max_args > 0 && arg_count >= max_args) ||
			    strbuf_getlen(path_list) + strlen(path) > path_list_max)
			{
				gtags_add(gtop, strbuf_value(comline), path_list, gflags);
				strbuf_reset(path_list);
				arg_count = 0;
			}
		}
		/*
		 * Add a path to path_list.
		 */
		strbuf_puts0(path_list, path);
		arg_count++;
	}
	if (strbuf_getlen(path_list))
		gtags_add(gtop, strbuf_value(comline), path_list, gflags);
	total = count;				/* save total count */
	find_close();
	gtags_close(gtop);
	strbuf_close(comline);
	strbuf_close(path_list);
}
/*
 * printconf: print configuration data.
 *
 *	i)	name	label of config data
 *	r)		exit code
 */
int
printconf(name)
	const char *name;
{
	int num;
	int exist = 1;

	if (getconfn(name, &num))
		fprintf(stdout, "%d\n", num);
	else if (getconfb(name))
		fprintf(stdout, "1\n");
	else {
		STRBUF *sb = strbuf_open(0);
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
	const char *root;
	const char *cwd;
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
	const char *line;
	int absolute;
	int cxref;
{
	char buf[MAXPATHLEN+1];
	const char *p = line;

	/*
	 * print until path name.
	 */
	if (cxref) {
		/* print tag name */
		for (; *p && !isspace((unsigned char)*p); p++)
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
	for (; *p && !isspace((unsigned char)*p); p++)
		strbuf_putc(abspath, *p);
	/*
	 * put path with converting.
	 */
	if (absolute) {
		(void)fputs(strbuf_value(abspath), stdout);
	} else {
		const char *a = strbuf_value(abspath);
		const char *b = basedir;
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
