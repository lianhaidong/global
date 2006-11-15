/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2006
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
int main(int, char **);
int incremental(const char *, const char *);
void updatetags(const char *, const char *, IDSET *, STRBUF *, int);
void createtags(const char *, const char *, int);
int printconf(const char *);
void set_base_directory(const char *, const char *);

int iflag;					/* incremental update */
int Iflag;					/* make  idutils index */
int qflag;					/* quiet mode */
int wflag;					/* warning message */
int vflag;					/* verbose mode */
int max_args;
int show_version;
int show_help;
int show_config;
char *gtagsconf;
char *gtagslabel;
int debug;
const char *config_name;
const char *file_list;

/*
 * Path filter
 */
int do_path;
int convert_type = PATH_RELATIVE;
int format = FORMAT_PATH;
/*
 * Sort filter
 */
int do_sort;
int unique;

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
	/*
	 * These options have long name and short name.
	 * We throw them to the processing of short options.
	 *
	 * Though the -o(--omit-gsyms) was removed, this code
	 * is left for compatibility.
	 */
	{"file", required_argument, NULL, 'f'},
	{"idutils", no_argument, NULL, 'I'},
	{"incremental", no_argument, NULL, 'i'},
	{"max-args", required_argument, NULL, 'n'},
	{"omit-gsyms", no_argument, NULL, 'o'},		/* removed */
	{"quiet", no_argument, NULL, 'q'},
	{"verbose", no_argument, NULL, 'v'},
	{"warning", no_argument, NULL, 'w'},

	/*
	 * The following are long name only.
	 */
	/* flag value */
	{"debug", no_argument, &debug, 1},
	{"sort", no_argument, &do_sort, 1},
	{"unique", no_argument, &unique, 1},
	{"version", no_argument, &show_version, 1},
	{"help", no_argument, &show_help, 1},

	/* accept value */
#define OPT_CONFIG		128
#define OPT_FORMAT		129
#define OPT_GTAGSCONF		130
#define OPT_GTAGSLABEL		131
#define OPT_PATH		132
	{"config", optional_argument, NULL, OPT_CONFIG},
	{"format", required_argument, NULL, OPT_FORMAT},
	{"gtagsconf", required_argument, NULL, OPT_GTAGSCONF},
	{"gtagslabel", required_argument, NULL, OPT_GTAGSLABEL},
	{"path", required_argument, NULL, OPT_PATH},
	{ 0 }
};

static const char *langmap = DEFAULTLANGMAP;

void
output(const char *s)
{
	fputs(s, stdout);
	fputc('\n', stdout);
}

int
main(int argc, char **argv)
{
	char dbpath[MAXPATHLEN+1];
	char cwd[MAXPATHLEN+1];
	STRBUF *sb = strbuf_open(0);
	int db;
	int optchar;
	int option_index = 0;

	while ((optchar = getopt_long(argc, argv, "f:iIn:oqvwse", long_options, &option_index)) != EOF) {
		switch (optchar) {
		case 0:
			/* already flags set */
			break;
		case OPT_CONFIG:
			show_config = 1;
			if (optarg)
				config_name = optarg;
			break;
		case OPT_FORMAT:
			if (!strcmp("ctags", optarg))
				format = FORMAT_CTAGS;
			else if (!strcmp("path", optarg))
				format = FORMAT_PATH;
			else if (!strcmp("grep", optarg))
				format = FORMAT_GREP;
			else if (!strcmp("cscope", optarg))
				format = FORMAT_CSCOPE;
			else if (!strcmp("ctags-x", optarg))
				format = FORMAT_CTAGS_X;
			else
				die("Unknown format type.");
			break;
		case OPT_GTAGSCONF:
			gtagsconf = optarg;
			break;
		case OPT_GTAGSLABEL:
			gtagslabel = optarg;
			break;
		case OPT_PATH:
			do_path = 1;
			if (!strcmp("absolute", optarg))
				convert_type = PATH_ABSOLUTE;
			else if (!strcmp("relative", optarg))
				convert_type = PATH_RELATIVE;
			else if (!strcmp("through", optarg))
				convert_type = PATH_THROUGH;
			else
				die("Unknown path type.");
			break;
		case 'f':
			file_list = optarg;
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
			/*
			 * Though the -o(--omit-gsyms) was removed, this code
			 * is left for compatibility.
			 */
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
		default:
			usage();
			break;
		}
	}
	if (gtagsconf) {
		char path[MAXPATHLEN+1];

		if (realpath(gtagsconf, path) == NULL)
			die("%s not found.", optarg);
		set_env("GTAGSCONF", path);
		if (gtagslabel)
			set_env("GTAGSLABEL", gtagslabel);
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
		if (config_name)
			printconf(config_name);
		else
			fprintf(stdout, "%s\n", getconfline());
		exit(0);
	} else if (do_sort) {
		/*
		 * This is the main body of sort filter.
		 *
		 * - Requirement -
		 * 1. input must be one of these format:
		 *    0: ctags -x format
		 *    1: ctags format
		 *    2: path name
		 * 2. input must be sorted in alphabetical order by tag name
		 *    if it is ctags [-x] format.
		 */
		STRBUF *ib = strbuf_open(MAXBUFLEN);
		TAGSORT *ts = tagsort_open(output, format, unique, 0);

		while (strbuf_fgets(ib, stdin, STRBUF_NOCRLF) != NULL)
			tagsort_put(ts, strbuf_value(ib));
		tagsort_close(ts);
		strbuf_close(ib);
		exit(0);
	} else if (do_path) {
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
		 * Similarly, the --path=absolute option specified, then
		 *		v
		 * main      10 /prj/xxx/src/main.c  main(argc, argv)\n
		 * main      22 /prj/xxx/libc/func.c   main(argc, argv)\n
		 */
		STRBUF *ib = strbuf_open(MAXBUFLEN);
		CONVERT *cv = convert_open(convert_type, format, argv[0], argv[1], argv[2], stdout);
		char *ctags_x;

		if (argc < 3)
			die("gtags --path: 3 arguments needed.");
		while ((ctags_x = strbuf_fgets(ib, stdin, STRBUF_NOCRLF)) != NULL)
			convert_put(cv, ctags_x);
		convert_close(cv);
		strbuf_close(ib);
		exit(0);
	} else if (Iflag) {
		if (!usable("mkid"))
			die("mkid not found.");
	}

	/*
	 * If the file_list other than "-" is given, it must be readable file.
	 */
	if (file_list && strcmp(file_list, "-")) {
		if (test("d", file_list))
			die("'%s' is a directory.", file_list);
		else if (!test("f", file_list))
			die("'%s' not found.", file_list);
		else if (!test("r", file_list))
			die("'%s' is not readable.", file_list);
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
	/*
	 * Pass the following information to gtags-parser(1)
	 * using environment variable.
	 *
	 * o langmap
	 * o DBPATH
	 */
	strbuf_reset(sb);
	if (getconfs("langmap", sb))
		langmap = check_strdup(strbuf_value(sb));
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
		GTOP *gtop = gtags_open(dbpath, cwd, GTAGS, GTAGS_MODIFY);
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
	for (db = GTAGS; db < GTAGLIM; db++) {

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
	}
	/*
	 * create idutils index.
	 */
	if (Iflag) {
		if (vflag)
			fprintf(stderr, "[%s] Creating indexes for idutils.\n", now());
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
incremental(const char *dbpath, const char *root)
{
	struct stat statp;
	time_t gtags_mtime;
	STRBUF *addlist = strbuf_open(0);
	STRBUF *deletelist = strbuf_open(0);
	STRBUF *addlist_other = strbuf_open(0);
	IDSET *deleteset, *findset;
	int updated = 0;
	const char *path;
	int i, limit;

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
	 * deleteset:
	 *	The list of the path name which should be deleted from GPATH.
	 * findset:
	 *	The list of the path name which exists in the current project.
	 *	A project is limited by the --file option.
	 */
	deleteset = idset_open(gpath_nextkey());
	findset = idset_open(gpath_nextkey());
	/*
	 * make add list and delete list for update.
	 */
	if (file_list)
		find_open_filelist(file_list, root);
	else
		find_open(NULL);
	total = 0;
	while ((path = find_read()) != NULL) {
		const char *fid;
		int n_fid = 0;
		int other = 0;

		/* a blank at the head of path means 'NOT SOURCE'. */
		if (*path == ' ') {
			if (test("b", ++path))
				continue;
			other = 1;
		}
		if (stat(path, &statp) < 0)
			die("stat failed '%s'.", path);
		fid = gpath_path2fid(path, NULL);
		if (fid) { 
			n_fid = atoi(fid);
			idset_add(findset, n_fid);
		}
		if (other) {
			if (fid == NULL)
				strbuf_puts0(addlist_other, path);
		} else {
			if (fid == NULL) {
				strbuf_puts0(addlist, path);
				total++;
			} else if (gtags_mtime < statp.st_mtime) {
				strbuf_puts0(addlist, path);
				total++;
				idset_add(deleteset, n_fid);
			}
		}
	}
	find_close();
	/*
	 * make delete list.
	 */
	limit = gpath_nextkey();
	for (i = 1; i < limit; i++) {
		char fid[32];
		int type;

		snprintf(fid, sizeof(fid), "%d", i);
		/*
		 * This is a hole of GPATH. The hole increases if the deletion
		 * and the addition are repeated.
		 */
		if ((path = gpath_fid2path(fid, &type)) == NULL)
			continue;
		/*
		 * The file which does not exist in the findset is treated
		 * assuming that it does not exist in the file system.
		 */
		if (type == GPATH_OTHER) {
			if (!idset_contains(findset, i) || !test("f", path) || test("b", path))
				strbuf_puts0(deletelist, path);
		} else {
			if (!idset_contains(findset, i) || !test("f", path)) {
				strbuf_puts0(deletelist, path);
				idset_add(deleteset, i);
			}
		}
	}
	gpath_close();
	/*
	 * execute updating.
	 */
	if (deleteset->max + strbuf_getlen(addlist)) {
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
			updatetags(dbpath, root, deleteset, addlist, db);
		}
		updated = 1;
	}
	if (strbuf_getlen(deletelist) + strbuf_getlen(addlist_other) > 0) {
		const char *start, *end, *p;

		if (vflag)
			fprintf(stderr, "[%s] Updating '%s'.\n", now(), dbname(0));
		gpath_open(dbpath, 2);
		if (strbuf_getlen(deletelist) > 0) {
			start = strbuf_value(deletelist);
			end = start + strbuf_getlen(deletelist);

			for (p = start; p < end; p += strlen(p) + 1)
				gpath_delete(p);
		}
		if (strbuf_getlen(addlist_other) > 0) {
			start = strbuf_value(addlist_other);
			end = start + strbuf_getlen(addlist_other);

			for (p = start; p < end; p += strlen(p) + 1)
				gpath_put(p, GPATH_OTHER);
		}
		gpath_close();
		updated = 1;
	}
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
	strbuf_close(addlist_other);
	idset_close(deleteset);
	idset_close(findset);

	return updated;
}
/*
 * updatetags: update tag file.
 *
 *	i)	dbpath		directory in which tag file exist
 *	i)	root		root directory of source tree
 *	i)	deleteset	bit array of fid of deleted or modified files 
 *	i)	addlist		\0 separated list of added or modified files
 *	i)	db		GTAGS, GRTAGS, GSYMS
 */
static void
verbose_updatetags(char *path, int seqno, int skip)
{
	if (total)
		fprintf(stderr, " [%d/%d]", seqno, total);
	else
		fprintf(stderr, " [%d]", seqno);
	fprintf(stderr, " adding tags of %s", path);
	if (skip)
		fprintf(stderr, " (skipped)");
	fputc('\n', stderr);
}
void
updatetags(const char *dbpath, const char *root, IDSET *deleteset, STRBUF *addlist, int db)
{
	GTOP *gtop;
	STRBUF *comline = strbuf_open(0);
	int seqno;

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
	gtop = gtags_open(dbpath, root, db, GTAGS_MODIFY);
	if (vflag) {
		char fid[32];
		const char *path;
		int total = idset_count(deleteset);
		int i;

		seqno = 1;
		for (i = 0; i < deleteset->max; i++) {
			if (idset_contains(deleteset, i)) {
				snprintf(fid, sizeof(fid), "%d", i);
				path = gpath_fid2path(fid, NULL);
				if (path == NULL)
					die("GPATH is corrupted.");
				fprintf(stderr, " [%d/%d] deleting tags of %s\n", seqno++, total, path + 2);
			}
		}
	}
	if (deleteset->max > 0)
		gtags_delete(gtop, deleteset);
	gtop->flags = 0;
	if (extractmethod)
		gtop->flags |= GTAGS_EXTRACTMETHOD;
	if (debug)
		gtop->flags |= GTAGS_DEBUG;
	/*
	 * Compact format requires the tag records of the same file are
	 * consecutive. We assume that the output of gtags-parser and
	 * any plug-in parsers are consecutive for each file.
	 * if (gtop->format & GTAGS_COMPACT) {
	 *	nothing to do
	 * }
	 */
	/*
	 * If the --max-args option is not specified, we pass the parser
	 * the source file as a lot as possible to decrease the invoking
	 * frequency of the parser.
	 */
	{
		XARGS *xp;
		char *ctags_x;
		char tag[MAXTOKEN], *p;

		xp = xargs_open_with_strbuf(strbuf_value(comline), max_args, addlist);
		xp->put_gpath = 1;
		if (vflag)
			xp->verbose = verbose_updatetags;
		if (db == GSYMS)
			xp->skip_assembly = 1;
		while ((ctags_x = xargs_read(xp)) != NULL) {
			strlimcpy(tag, strmake(ctags_x, " \t"), sizeof(tag));
			/*
			 * extract method when class method definition.
			 *
			 * Ex: Class::method(...)
			 *
			 * key	= 'method'
			 * data = 'Class::method  103 ./class.cpp ...'
			 */
			p = tag;
			if (gtop->flags & GTAGS_EXTRACTMETHOD) {
				if ((p = locatestring(tag, ".", MATCH_LAST)) != NULL)
					p++;
				else if ((p = locatestring(tag, "::", MATCH_LAST)) != NULL)
					p += 2;
				else
					p = tag;
			}
			gtags_put(gtop, p, ctags_x);
		}
		total = xargs_close(xp);
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
static void
verbose_createtags(char *path, int seqno, int skip)
{
	if (total)
		fprintf(stderr, " [%d/%d]", seqno, total);
	else
		fprintf(stderr, " [%d]", seqno);
	fprintf(stderr, " extracting tags of %s", path);
	if (skip)
		fprintf(stderr, " (skipped)");
	fputc('\n', stderr);
}
void
createtags(const char *dbpath, const char *root, int db)
{
	GTOP *gtop;
	XARGS *xp;
	char *ctags_x;
	STRBUF *comline = strbuf_open(0);
	STRBUF *path_list = strbuf_open(MAXPATHLEN);

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
	gtop = gtags_open(dbpath, root, db, GTAGS_CREATE);
	/*
	 * Set flags.
	 */
	gtop->flags = 0;
	if (extractmethod)
		gtop->flags |= GTAGS_EXTRACTMETHOD;
	if (debug)
		gtop->flags |= GTAGS_DEBUG;
	/*
	 * Compact format requires the tag records of the same file are
	 * consecutive. We assume that the output of gtags-parser and
	 * any plug-in parsers are consecutive for each file.
	 * if (gtop->format & GTAGS_COMPACT) {
	 *	nothing to do
	 * }
	 */
	/*
	 * If the --max-args option is not specified, we pass the parser
	 * the source file as a lot as possible to decrease the invoking
	 * frequency of the parser.
	 */
	if (file_list)
		find_open_filelist(file_list, root);
	else
		find_open(NULL);
	/*
	 * Add tags.
	 */
	xp = xargs_open_with_find(strbuf_value(comline), max_args);
	xp->put_gpath = 1;
	if (vflag)
		xp->verbose = verbose_createtags;
	if (db == GSYMS)
		xp->skip_assembly = 1;
	while ((ctags_x = xargs_read(xp)) != NULL) {
		char tag[MAXTOKEN], *p;

		strlimcpy(tag, strmake(ctags_x, " \t"), sizeof(tag));
		/*
		 * extract method when class method definition.
		 *
		 * Ex: Class::method(...)
		 *
		 * key	= 'method'
		 * data = 'Class::method  103 ./class.cpp ...'
		 */
		p = tag;
		if (gtop->flags & GTAGS_EXTRACTMETHOD) {
			if ((p = locatestring(tag, ".", MATCH_LAST)) != NULL)
				p++;
			else if ((p = locatestring(tag, "::", MATCH_LAST)) != NULL)
				p += 2;
			else
				p = tag;
		}
		gtags_put(gtop, p, ctags_x);
	}
	total = xargs_close(xp);
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
printconf(const char *name)
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
