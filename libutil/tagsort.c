/*
 * Copyright (c) 2005 Tama Communications Corporation
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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>

#include "die.h"
#include "gparam.h"
#include "dbop.h"
#include "split.h"
#include "strbuf.h"
#include "strlimcpy.h"
#include "tagsort.h"
#include "varray.h"

/*
 * A special version of sort command.
 *
 * 1. ctags [-x] format
 *
 * As long as the input meets the undermentioned requirement,
 * you can use this special sort command as a sort filter for
 * global(1) instead of external sort command.
 * 'gtags --sort [--unique]' is equivalent with
 * 'sort -k 1,1 -k 3,3 -k 2,2n [-u]',
 * and 'gtags --ctags --sort [--unique]' is equivalent with
 * 'sort -k 1,1 -k 2,2 -k 3,3n [-u]',
 * but does not need temporary files.
 *
 * - Requirement -
 * 1. input must be ctags [-x] format.
 * 2. input must be sorted in alphabetical order by tag name.
 *
 * usage: read from stdin, sort and write it to stdout.
 *
 * tagsort(unique, TAGSORT_CTAGS, stdin, stdout);
 *
 * 2. path name only format
 *
 * 'gtags --sort --pathname' is equivalent with
 * 'sort -u'.
 * The --pathname option always includes --unique option.
 */
/*
 * This entry corresponds to one record of ctags [-x] format.
 */
struct dup_entry {
        int offset;
        SPLIT ptable;
        const char *path;
        int lineno;
};

static int compare_dup_entry(const void *, const void *);
static void put_lines(int, int, char *, struct dup_entry *, int, FILE *);

/*
 * compare_dup_entry: compare function for sorting dup_entries.
 */
static int
compare_dup_entry(const void *v1, const void *v2)
{
	const struct dup_entry *e1 = v1, *e2 = v2;
	int ret;

	if ((ret = strcmp(e1->path, e2->path)) != 0)
		return ret;
	return e1->lineno - e2->lineno;
}
/*
 * put_lines: sort and print duplicate lines
 *
 *	i)	unique	unique or not
 *			0: sort, 1: sort -u
 *	i)	format	tag format
 *			TAGSORT_CTAGS_X: ctags -x format
 *			TAGSORT_CTAGS: ctags format
 *	i)	lines	ctags stream
 *	i)	entries	sort target
 *	i)	entry_count number of entry of the entries
 *	i)	op	output file
 */
static void
put_lines(int unique, int format, char *lines, struct dup_entry *entries, int entry_count, FILE *op)
{
	int i;
	char last_path[MAXPATHLEN+1];
	int last_lineno;
	int splits, part_lno, part_path;
	/*
	 * ctags format.
	 *
	 * PART_TAG     PART_PATH     PART_LNO
	 * +----------------------------------------------
	 * |main        src/main      227
	 *
	 * ctags -x format.
	 *
	 * PART_TAG     PART_LNO PART_PATH      PART_LINE
	 * +----------------------------------------------
	 * |main             227 src/main       main()
	 *
	 */
	switch (format) {
	case TAGSORT_CTAGS:
		splits = 3;
		part_lno = PART_CTAGS_LNO;
		part_path = PART_CTAGS_PATH;
		break;
	case TAGSORT_CTAGS_X:
		splits = 4;
		part_lno = PART_LNO;
		part_path = PART_PATH;
		break;
	default:
		die("internal error in put_lines.");
	}
	/*
	 * Parse and sort ctags [-x] format records.
	 */
	for (i = 0; i < entry_count; i++) {
		char *ctags_x = lines + entries[i].offset;
		SPLIT *ptable = &entries[i].ptable;

		if (split(ctags_x, splits, ptable) < splits) {
			recover(ptable);
			die("too small number of parts.\n'%s'", ctags_x);
		}
		entries[i].lineno = atoi(ptable->part[part_lno].start);
		entries[i].path = ptable->part[part_path].start;
	}
	qsort(entries, entry_count, sizeof(struct dup_entry), compare_dup_entry);
	/*
	 * The variables last_xxx has always the value of previous record.
	 * As for the initial value, it must be a value which does not
	 * appear in actual records.
	 */
	last_path[0] = '\0';
	last_lineno = 0;
	/*
	 * write sorted records.
	 */
	for (i = 0; i < entry_count; i++) {
		struct dup_entry *e = &entries[i];
		int skip = 0;

		if (unique) {
			if (!strcmp(e->path, last_path)) {
				if (e->lineno == last_lineno)
					skip = 1;
				else
					last_lineno = e->lineno;
			} else {
				last_lineno = e->lineno;
				strlimcpy(last_path, e->path, sizeof(last_path));
			}
		}
		recover(&e->ptable);
		if (!skip) {
			fputs(lines + e->offset, op);
			fputc('\n', op);
		}
	}
}

/*
 * ctags_sort: sort ctags [-x] format records
 *
 *	i)	unique	0: sort, 1: sort -u
 *	i)	format	tag format
 *			TAGSORT_CTAGS_X: ctags -x format
 *			TAGSORT_CTAGS: ctags format
 *	i)	ip	input
 *	i)	op	output
 */
static void
ctags_sort(int unique, int format, FILE *ip, FILE *op)
{
	STRBUF *ib = strbuf_open(MAXBUFLEN);
	STRBUF *sb = strbuf_open(MAXBUFLEN);
	VARRAY *vb = varray_open(sizeof(struct dup_entry), 100);
	char *ctags_x, prev[IDENTLEN];

	prev[0] = '\0';
	while ((ctags_x = strbuf_fgets(ib, ip, STRBUF_NOCRLF)) != NULL) {
		const char *tag;
		struct dup_entry *entry;
		SPLIT ptable;

		if (split(ctags_x, 2, &ptable) < 2) {
			recover(&ptable);
			die("too small number of parts.\n'%s'", ctags_x);
		}
		/*
		 * collect the records with the same tag, sort and write
		 * using put_lines().
		 */
		tag = ptable.part[PART_TAG].start;
		if (strcmp(prev, tag) != 0) {
			if (prev[0] != '\0') {
				if (vb->length == 1) {
					fputs(strbuf_value(sb), op);
					fputc('\n', op);
				} else
					put_lines(unique,
						format,
						strbuf_value(sb),
						varray_assign(vb, 0, 0),
						vb->length,
						op);
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
		if (vb->length == 1) {
			fputs(strbuf_value(sb), op);
			fputc('\n', op);
		} else
			put_lines(unique,
				format,
				strbuf_value(sb),
				varray_assign(vb, 0, 0),
				vb->length,
				op);
	}
	strbuf_close(ib);
	strbuf_close(sb);
	varray_close(vb);
}
/*
 * path_sort: sort path name only format records
 *
 *	i)	ip	input
 *	i)	op	output
 *
 * This function is applicable in the output of find(1) etc.
 */
static void
path_sort(FILE *ip, FILE *op)
{
	DBOP *dbop;
	STRBUF *ib = strbuf_open(MAXBUFLEN);
	const char *path;
	const char *null = "";

	dbop = dbop_open(NULL, 1, 0600, 0);
	if (!dbop)
		die("cannot make temporary file in path_sort().");
	while ((path = strbuf_fgets(ib, ip, 0)) != NULL) {
		dbop_put(dbop, path, null);
	}
	for (path = dbop_first(dbop, NULL, NULL, DBOP_KEY);
	     path != NULL;
	     path = dbop_next(dbop)) {
		fputs(path, op);
	}
	dbop_close(dbop);
	strbuf_close(ib);
}
/*
 * tagsort:
 *
 *	i)	unique	0: sort, 1: sort -u
 *			In TAGSORT_PATH format, it is always considered 1.
 *	i)	format	tag format
 *			TAGSORT_CTAGS_X: ctags -x format
 *			TAGSORT_CTAGS: ctags format
 *			TAGSORT_PATH: path name
 *	i)	ip	input
 *	i)	op	output
 */
void
tagsort(int unique, int format, FILE *ip, FILE *op)
{
	switch (format) {
	case TAGSORT_PATH:
		path_sort(ip, op);
		break;
	case TAGSORT_CTAGS_X:
	case TAGSORT_CTAGS:
		ctags_sort(unique, format, ip, op);	
		break;
	default:
		die("internal error in tagsort.");
	}
}
