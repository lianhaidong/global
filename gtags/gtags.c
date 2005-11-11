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

struct dup_entry {
	int offset;
	SPLIT ptable;
	int lineno;
};

static void usage(void);
static void help(void);
static int compare_dup_entry(const void *, const void *);
static void put_lines(char *, struct dup_entry *, int);
int main(int, char **);
int incremental(const char *, const char *);
void updatetags(const char *, const char *, IDSET *, STRBUF *, int);
void createtags(const char *, const char *, int);
int printconf(const char *);
void set_base_directory(const char *, const char *);
void put_converting(const char *, int, int, int);

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
int do_sort;
int do_relative;
int do_absolute;
int cxref;
int fileid;
int gtagsconf;
int gtagslabel;
int debug;
const char *extra_options;
const char *info_string;
const char *file_list;

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
	{"compact", no_argument, NULL, 'c'},
	{"file", required_argument, NULL, 'f'},
	{"idutils", no_argument, NULL, 'I'},
	{"incremental", no_argument, NULL, 'i'},
	{"max-args", required_argument, NULL, 'n'},
	{"omit-gsyms", no_argument, NULL, 'o'},
	{"quiet", no_argument, NULL, 'q'},
	{"verbose", no_argument, NULL, 'v'},
	{"warning", no_argument, NULL, 'w'},

	/* long name only */
	{"absolute", no_argument, &do_absolute, 1},
	{"config", optional_argument, &show_config, 1},
	{"cxref", no_argument, &cxref, 1},
	{"debug", no_argument, &debug, 1},
	{"fileid", no_argument, &fileid, 1},
	{"gtagsconf", required_argument, &gtagsconf, 1},
	{"gtagslabel", required_argument, &gtagslabel, 1},
	{"relative", no_argument, &do_relative, 1},
	{"sort", no_argument, &do_sort, 1},
	{"version", no_argument, &show_version, 1},
	{"help", no_argument, &show_help, 1},
	{ 0 }
};

static const char *langmap = DEFAULTLANGMAP;

/*
 * compare_dup_entry: compare function for sorting.
 */
static int
compare_dup_entry(const void *v1, const void *v2)
{
	const struct dup_entry *e1 = v1, *e2 = v2;
	int ret;

	if ((ret = strcmp(e1->ptable.part[PART_PATH].start,
			  e2->ptable.part[PART_PATH].start)) != 0)
		return ret;
	return e1->lineno - e2->lineno;
}
/*
 * put_lines: sort and print duplicate lines
 */
static void
put_lines(char *lines, struct dup_entry *entries, int entry_count)
{
	int i;
	char last_path[MAXPATHLEN+1];
	int last_lineno;

	for (i = 0; i < entry_count; i++) {
		char *ctags_x = lines + entries[i].offset;
		SPLIT *ptable = &entries[i].ptable;

		if (split(ctags_x, 4, ptable) < 4) {
			recover(ptable);
			die("too small number of parts.\n'%s'", ctags_x);
		}
		entries[i].lineno = atoi(ptable->part[PART_LNO].start);
	}
	qsort(entries, entry_count, sizeof(struct dup_entry), compare_dup_entry);
	/*
	 * The variables last_xxx has always the value of previous record.
	 * As for the initial value, it must be the value which does not
	 * appear in an actual record.
	 */
	last_path[0] = '\0';
	last_lineno = 0;
	for (i = 0; i < entry_count; i++) {
		struct dup_entry *e = &entries[i];
		int skip = 0;

		if (!strcmp(e->ptable.part[PART_PATH].start, last_path)) {
			if (e->lineno == last_lineno)
				skip = 1;
			else
				last_lineno = e->lineno;
		} else {
			last_lineno = e->lineno;
			strlimcpy(last_path, e->ptable.part[PART_PATH].start, sizeof(last_path));
		}
		recover(&e->ptable);
		if (!skip)
			puts(lines + e->offset);
	}
}

int
main(int argc, char **argv)
{
	char dbpath[MAXPATHLEN+1];
	char cwd[MAXPATHLEN+1];
	STRBUF *sb = strbuf_open(0);
	const char *p;
	int db;
	int optchar;
	int option_index = 0;

	while ((optchar = getopt_long(argc, argv, "cf:iIn:oqvwse", long_options, &option_index)) != EOF) {
		switch (optchar) {
		case 0:
			p = long_options[option_index].name;
			if (!strcmp(p, "config")) {
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
		 */
		STRBUF *ib = strbuf_open(MAXBUFLEN);
		STRBUF *sb = strbuf_open(MAXBUFLEN);
		VARRAY *vb = varray_open(sizeof(struct dup_entry), 100);
		char *ctags_x, prev[IDENTLEN];

		prev[0] = '\0';
		while ((ctags_x = strbuf_fgets(ib, stdin, STRBUF_NOCRLF)) != NULL) {
			const char *tag;
			struct dup_entry *entry;
			SPLIT ptable;

			if (split(ctags_x, 2, &ptable) < 2) {
				recover(&ptable);
				die("too small number of parts.\n'%s'", ctags_x);
			}
			tag = ptable.part[PART_TAG].start;
			if (prev[0] == '\0' || strcmp(prev, tag) != 0) {
				if (prev[0] != '\0') {
					if (vb->length == 1)
						puts(strbuf_value(sb));
					else
						put_lines(strbuf_value(sb),
							varray_assign(vb, 0, 0),
							vb->length);
				}
				strlimcpy(prev, tag, sizeof(prev));
				strbuf_reset(sb);
				varray_reset(vb);
			}
			entry = varray_append(vb);
			entry->offset = strbuf_getlen(sb);
			recover(&ptable);
			strbuf_puts0(sb, ctags_x);
		}
		if (prev[0] != '\0') {
			if (vb->length == 1)
				puts(strbuf_value(sb));
			else
				put_lines(strbuf_value(sb),
					varray_assign(vb, 0, 0), vb->length);
		}
		strbuf_close(ib);
		strbuf_close(sb);
		varray_close(vb);
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
		const char *a_root = argv[0];
		const char *a_cwd = argv[1];
		const char *a_dbpath = argv[2];
		const char *ctags_x;

		if (argc < 2)
			die("do_relative: 2 arguments needed.");
		set_base_directory(a_root, a_cwd);
                if (gpath_open(a_dbpath, 0) < 0)
                        die("GPATH not found.");
		while ((ctags_x = strbuf_fgets(ib, stdin, 0)) != NULL)
			put_converting(ctags_x, do_absolute ? 1 : 0, cxref, fileid);
		gpath_close();
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
	 * Use C locale in order to avoid the degradation of performance
	 * by internationalized sort command.
	 */
	set_env("LC_ALL", "C");
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
		int other;

		snprintf(fid, sizeof(fid), "%d", i);
		/*
		 * This is a hole of GPATH. The hole increases if the deletion
		 * and the addition are repeated.
		 */
		if ((path = gpath_fid2path(fid, &other)) == NULL)
			continue;
		/*
		 * The file which does not exist in the findset is treated
		 * assuming that it does not exist in the file system.
		 */
		if (other) {
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
	gtop = gtags_open(dbpath, root, db, GTAGS_MODIFY, 0);
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
	 * consecutive.
	 *
	 * We assume that the output of gtags-parser is consecutive for each
	 * file. About the other parsers, it is not guaranteed, so we sort it
	 * using external sort command (gnusort).
	 */
	if (gtop->format & GTAGS_COMPACT) {
		if (locatestring(strbuf_value(comline), "gtags-parser", MATCH_FIRST) == NULL) {
			if (locatestring(strbuf_value(comline), "%s", MATCH_FIRST) == NULL)
				strbuf_puts(comline, " %s");
			strbuf_sprintf(comline, "| %s -k 3,3", POSIX_SORT);
		}
	}
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
	int flags;
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
	/*
	 * Compact format:
	 *
	 * non: STANDARD format
	 * -c:  COMPACT format + PATHINDEX option
	 * -cc: STANDARD format + PATHINDEX option
	 * Ths -cc is undocumented.
	 * In the future, it may become the standard format of GLOBAL.
	 */
	flags = 0;
	if (cflag) {
		flags |= GTAGS_PATHINDEX;
		if (cflag == 1)
			flags |= GTAGS_COMPACT;
	}
	if (vflag > 1)
		fprintf(stderr, " using tag command '%s <path>'.\n", strbuf_value(comline));
	gtop = gtags_open(dbpath, root, db, GTAGS_CREATE, flags);
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
	 * consecutive.
	 *
	 * We assume that the output of gtags-parser is consecutive for each
	 * file. About the other parsers, it is not guaranteed, so we sort it
	 * using external sort command (gnusort).
	 */
	if (gtop->format & GTAGS_COMPACT) {
		if (locatestring(strbuf_value(comline), "gtags-parser", MATCH_FIRST) == NULL) {
			if (locatestring(strbuf_value(comline), "%s", MATCH_FIRST) == NULL)
				strbuf_puts(comline, " %s");
			strbuf_sprintf(comline, "| %s -k 3,3", POSIX_SORT);
		}
	}
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
/*
 * put_converting: convert path into relative or absolute and print.
 *
 *	i)	raw output from global(1)
 *	i)	absolute 1: absolute, 0: relative
 *	i)	cxref 1: -x format, 0: file name only
 *	i)	fileid 1: add fid to the head of line, 0: do nothing
 */
static STRBUF *abspath;
static char basedir[MAXPATHLEN+1];
static int start_point;
void
set_base_directory(const char *root, const char *cwd)
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
put_converting(const char *ctags, int absolute, int cxref, int fileid)
{
	char buf[MAXPATHLEN+1];
	char *path, *p;

	/*
	 * print until path name.
	 */
	if (cxref) {
		int savec = 0;
		/*
		 * Move to the head of path name.
		 *
		 * Don't use split() function, since we must support
		 * following both format:
		 * ctags:	main    ./main.c     20
		 * ctags -x:	main      20 ./main.c main()
		 */
		/* skip tag name */
		for (p = (char *)ctags; *p && !isspace((unsigned char)*p); p++)
			;
		/* skip blanks (and line number) */
		for (; *p && *p != '.'; p++)
			;
		path = p;
		/*
		 * The --fileid option specified, put file id to the head
		 * of the output like follows.
		 *
		 * 120@main        32 ./main.c main(int argc, char **argv)
		 * ~~~                ~~~~~~~~~
		 *  ^                     |
		 *  +---------------------+
		 *
		 * Global.cgi script generated by htags(1) refers it to
		 * generate anchor.
		 */
		if (fileid) {
			for (; *p && !isspace((unsigned char)*p); p++)
				;
			savec = *p;
			*p = '\0';
			fputs(gpath_path2fid(path, NULL), stdout);
			*p = savec;
			putc('@', stdout);
		}
		for (p = (char *)ctags; *p && p < path; p++)
			(void)putc(*p, stdout);
	} else {
		path = (char *)ctags;
	}
	if (*path++ == '\0')
		return;
	/*
	 * make absolute path.
	 */
	strbuf_setlen(abspath, start_point);
	for (p = path; *p && !isspace((unsigned char)*p); p++)
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
