/*
 * Copyright (c) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "cache.h"
#include "common.h"
#include "queue.h"
#include "global.h"
#include "htags.h"

/*
 * Data for each tag file.
 *
 *				      GTAGS         GRTAGS       GSYMS
 */
static const char *dirs[]    = {NULL, DEFS,         REFS,        SYMS};
static const char *kinds[]   = {NULL, "definition", "reference", "symbol"};
static const char *options[] = {NULL, "",           "r",         "s"};

static char command[MAXFILLEN];

/*
 * Open duplicate object index file.
 *
 *	i)	db	tag type(GTAGS, GRTAGS, GSYMS)
 *	r)	op	file pointer
 */
static FILE *
open_dup_file(db, count)
	int db;
	int count;
{
	char path[MAXPATHLEN];
	FILE *op;

	snprintf(path, sizeof(path), "%s/%s/%d.%s", distpath, dirs[db], count, HTML);
	if (cflag) {
		snprintf(command, sizeof(command), "gzip -c >%s", path);
		op = popen(command, "w");
		if (op == NULL)
			die("cannot execute command '%s'.", command);
	} else {
		op = fopen(path, "w");
		if (op == NULL)
			die("cannot create file '%s'.", path);
	}
	return op;
}
/*
 * Close duplicate object index file.
 *
 *	i)	op	file pointer
 */
static void
close_dup_file(op)
	FILE *op;
{
	if (cflag) {
		if (pclose(op) != 0)
			die("'%s' failed.", command);
	} else {
		fclose(op);
	}
}
/*
 * Make duplicate object index.
 *
 * If referred tag is only one, direct link which points the tag is generated.
 * Else if two or more tag exists, indirect link which points the tag list
 * is generated.
 */
int
makedupindex(void)
{
	STRBUF *sb = strbuf_open(0);
	int definition_count = 0;
	char srcdir[MAXPATHLEN];
	int db;
	char buf[1024];
	FILE *op = NULL;
	FILE *ip = NULL;

	snprintf(srcdir, sizeof(srcdir), "../%s", SRCS);
	for (db = GTAGS; db < GTAGLIM; db++) {
		const char *kind = kinds[db];
		const char *option = options[db];
		int writing = 0;
		int count = 0;
		int entry_count = 0;
		char *ctags_x, tag[IDENTLEN], prev[IDENTLEN], first_line[MAXBUFLEN], command[MAXFILLEN];

		if (!symbol && db == GSYMS)
			continue;
		prev[0] = 0;
		first_line[0] = 0;
		snprintf(command, sizeof(command), "global -xn%s%s \".*\" | gtags --sort", dynamic ? "n" : "", option);
		if ((ip = popen(command, "r")) == NULL)
			die("cannot execute command '%s'.", command);
		while ((ctags_x = strbuf_fgets(sb, ip, STRBUF_NOCRLF)) != NULL) {
			SPLIT ptable;

			if (split(ctags_x, 2, &ptable) < 2) {
				recover(&ptable);
				die("too small number of parts.(1)\n'%s'", ctags_x);
			}
			strlimcpy(tag, ptable.part[PART_TAG].start, sizeof(tag));
			recover(&ptable);

			if (strcmp(prev, tag)) {
				count++;
				if (vflag)
					fprintf(stderr, " [%d] adding %s %s\n", count, kind, tag);
				if (writing) {
					if (!dynamic) {
						fputs_nl(gen_list_end(), op);
						fputs_nl(body_end, op);
						fputs_nl(gen_page_end(), op);
						close_dup_file(op);
						file_count++;
					}
					writing = 0;
					/*
					 * cache record: " <file id> <entry number>"
					 */
					snprintf(buf, sizeof(buf), " %d %d", count - 1, entry_count);
					cache_put(db, prev, buf);
				}				
				/* single entry */
				if (first_line[0]) {
					if (split(first_line, 3, &ptable) < 3) {
						recover(&ptable);
						die("too small number of parts.(2)\n'%s'", ctags_x);
					}
					snprintf(buf, sizeof(buf), "%s %s", ptable.part[PART_LNO].start, ptable.part[PART_PATH].start);
					cache_put(db, prev, buf);
					recover(&ptable);
				}
				/*
				 * Chop the tail of the line. It is not important.
				 * strlimcpy(first_line, ctags_x, sizeof(first_line));
				 */
				strncpy(first_line, ctags_x, sizeof(first_line));
				first_line[sizeof(first_line) - 1] = '\0';
				strlimcpy(prev, tag, sizeof(prev));
				entry_count = 0;
			} else {
				/* duplicate entry */
				if (first_line[0]) {
					if (!dynamic) {
						op = open_dup_file(db, count);
						fputs_nl(gen_page_begin(tag, 1), op);
						fputs_nl(body_begin, op);
						fputs_nl(gen_list_begin(), op);
						fputs_nl(gen_list_body(srcdir, first_line), op);
					}
					writing = 1;
					entry_count++;
					first_line[0] = 0;
				}
				if (!dynamic) {
					fputs_nl(gen_list_body(srcdir, ctags_x), op);
				}
				entry_count++;
			}
		}
		if (db == GTAGS)
			definition_count = count;
		if (pclose(ip) != 0)
			die("'%s' failed.", command);
		if (writing) {
			if (!dynamic) {
				fputs_nl(gen_list_end(), op);
				fputs_nl(body_end, op);
				fputs_nl(gen_page_end(), op);
				close_dup_file(op);
				file_count++;
			}
			/*
			 * cache record: " <file id> <entry number>"
			 */
			snprintf(buf, sizeof(buf), " %d %d", count, entry_count);
			cache_put(db, prev, buf);
		}
		if (first_line[0]) {
			SPLIT ptable;

			if (split(first_line, 3, &ptable) < 3) {
				recover(&ptable);
				die("too small number of parts.(3)\n'%s'", ctags_x);
			}
			snprintf(buf, sizeof(buf), "%s %s", ptable.part[PART_LNO].start, ptable.part[PART_PATH].start);
			cache_put(db, prev, buf);
			recover(&ptable);
		}
	}
	strbuf_close(sb);
	return definition_count;
}
