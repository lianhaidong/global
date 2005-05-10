/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2002
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

#include "gparam.h"
#include "die.h"
#include "getdbpath.h"
#include "gtagsop.h"
#include "makepath.h"
#include "path.h"
#include "strlimcpy.h"
#include "test.h"

static const char *makeobjdirprefix;	/* obj partition		*/
static const char *makeobjdir;		/* obj directory		*/

/*
 * setupvariables: load variables regard to BSD OBJ directory.
 */
static void
setupvariables(verbose)
	int verbose;
{
	const char *p;

	if ((p = getenv("MAKEOBJDIRPREFIX")) != NULL) {
		makeobjdirprefix = p;
		if (verbose)
			fprintf(stderr, "MAKEOBJDIRPREFIX is set to '%s'.\n", p);
	} else {
		makeobjdirprefix = "/usr/obj";
	}
	if ((p = getenv("MAKEOBJDIR")) != NULL) {
		makeobjdir = p;
		if (verbose)
			fprintf(stderr, "MAKEOBJDIR is set to '%s'.\n", p);
	} else {
		makeobjdir = "obj";
	}
}
/*
 * gtagsexist: test whether GTAGS's existence.
 *
 *	i)	candidate candidate root directory
 *	o)	dbpath	directory which GTAGS exist
 *	i)	size	size of dbpath buffer
 *	i)	verbose	verbose mode 1: on, 0: off
 *	r)		0: not found, 1: found
 *
 * Gtagsexist locate GTAGS file in "$candidate/", "$candidate/obj/" and
 * "/usr/obj/$candidate/" in this order by default.
 * This behavior is same with BSD make(1)'s one.
 */
int
gtagsexist(candidate, dbpath, size, verbose)
	const char *candidate;
	char *dbpath;
	int size;
	int verbose;
{
	char path[MAXPATHLEN+1];

	/*
	 * setup makeobjdir and makeobjdirprefix (only first time).
	 */
	if (makeobjdir == NULL)
		setupvariables(verbose);

	snprintf(path, sizeof(path), "%s/%s", candidate, dbname(GTAGS));
	if (verbose)
		fprintf(stderr, "checking %s\n", path);
	if (test("fr", path)) {
		if (verbose)
			fprintf(stderr, "GTAGS found at '%s'.\n", path);
		snprintf(dbpath, size, "%s", candidate);
		return 1;
	}
	snprintf(path, sizeof(path),
		"%s/%s/%s", candidate, makeobjdir, dbname(GTAGS));
	if (verbose)
		fprintf(stderr, "checking %s\n", path);
	if (test("fr", path)) {
		if (verbose)
			fprintf(stderr, "GTAGS found at '%s'.\n", path);
		snprintf(dbpath, size, "%s/%s", candidate, makeobjdir);
		return 1;
	}
#if !defined(_WIN32) && !defined(__DJGPP__)
	snprintf(path, sizeof(path),
		"%s%s/%s", makeobjdirprefix, candidate, dbname(GTAGS));
	if (verbose)
		fprintf(stderr, "checking %s\n", path);
	if (test("fr", path)) {
		if (verbose)
			fprintf(stderr, "GTAGS found at '%s'.\n", path);
		snprintf(dbpath, size, "%s%s", makeobjdirprefix, candidate);
		return 1;
	}
#endif
	return 0;
}
/*
 * getdbpath: get dbpath directory
 *
 *	o)	cwd	current directory
 *	o)	root	root of source tree
 *	o)	dbpath	directory which GTAGS exist
 *	i)	verbose	verbose mode 1: on, 0: off
 *
 * root and dbpath assumed as
 *	char	cwd[MAXPATHLEN+1];
 *	char	root[MAXPATHLEN+1];
 *	char	dbpath[MAXPATHLEN+1];
 *
 * At first, getdbpath locate GTAGS file in the current directory.
 * If not found, it move up to the parent directory and locate GTAGS again.
 * It repeat above behavior until GTAGS file is found or reach the system's
 * root directory '/'. If reached to '/' then getdbpath print "GTAGS not found."
 * and exit.
 */
void
getdbpath(cwd, root, dbpath, verbose)
	char *cwd;
	char *root;
	char *dbpath;
	int verbose;
{
	struct stat sb;
	char *p;

	if (!getcwd(cwd, MAXPATHLEN))
		die("cannot get current directory.");
	canonpath(cwd);

	if ((p = getenv("GTAGSROOT")) != NULL) {
		if (verbose)
			fprintf(stderr, "GTAGSROOT is set to '%s'.\n", p);
		if (!isabspath(p))
			die("GTAGSROOT must be an absolute path.");
		if (stat(p, &sb) || !S_ISDIR(sb.st_mode))
			die("directory '%s' not found.", p);
		if (realpath(p, root) == NULL)
			die("cannot get real path of '%s'.", p);
		/*
		 * GTAGSDBPATH is meaningful only when GTAGSROOT exist.
		 */
		if ((p = getenv("GTAGSDBPATH")) != NULL) {
			if (verbose)
				fprintf(stderr, "GTAGSDBPATH is set to '%s'.\n", p);
			if (!isabspath(p))
				die("GTAGSDBPATH must be an absolute path.");
			if (stat(p, &sb) || !S_ISDIR(sb.st_mode))
				die("directory '%s' not found.", p);
			strlimcpy(dbpath, getenv("GTAGSDBPATH"), MAXPATHLEN);
		} else {
			if (!gtagsexist(root, dbpath, MAXPATHLEN, verbose))
				die_with_code(3, "GTAGS not found.");
		}
	} else {
		if (verbose && getenv("GTAGSDBPATH"))
			fprintf(stderr, "warning: GTAGSDBPATH is ignored becase GTAGSROOT is not set.\n");
		/*
		 * start from current directory to '/' directory.
		 */
		strlimcpy(root, cwd, MAXPATHLEN);
		p = root + strlen(root);
		while (!gtagsexist(root, dbpath, MAXPATHLEN, verbose)) {
			while (*--p != '/' && p > root)
				;
			*p = 0;
			if (root == p)	/* reached root directory */
				break;
		}
		if (*root == 0)
			die_with_code(3, "GTAGS not found.");
		/*
		 * If file 'GTAGSROOT' found without environment variable
		 * GTAGSDBPATH, use the value of it as GTAGSROOT.
		 */
		do {
			FILE *fp;
			STRBUF *sb;
			const char *s, *path;

			path = makepath(root, "GTAGSROOT", NULL);
			if (!test("fr", path)) {
				break;
			}
			fp = fopen(path, "r");
			if (fp == NULL) {
				if (verbose)
					fprintf(stderr, "'%s' ignored because it cannot be opened.\n", path);
				break;
			}
			sb = strbuf_open(0);
			s = strbuf_fgets(sb, fp, STRBUF_NOCRLF);
			if (!test("d", s)) {
				if (verbose)
					fprintf(stderr, "'%s' ignored because it doesn't include existent directory name.\n", path);
			} else {
				if (verbose)
					fprintf(stderr, "GTAGSROOT found at '%s'.\n", path);
				if (*s != '/') {
					char buf[MAXPATHLEN+1];
					s = realpath(makepath(root, s, NULL), buf);
				}
				strlimcpy(root, s, MAXPATHLEN);
			}
			fclose(fp);
			strbuf_close(sb);
			break;
		} while (0);
	}
}
