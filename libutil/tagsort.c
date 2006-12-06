/*
 * Copyright (c) 2005, 2006 Tama Communications Corporation
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
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#include <sys/param.h>

#include "checkalloc.h"
#include "die.h"
#include "gparam.h"
#include "dbop.h"
#include "split.h"
#include "strbuf.h"
#include "strlimcpy.h"
#include "tagsort.h"
#include "varray.h"

/*
 * Internal sort filter.
 *
 * This internal filter is equivalent with 'sort -k 1,1 -k 3,3 -k 2,2n [-u]'.
 *
 * - Requirement -
 * (1) input must be ctags-x format.
 * (2) input must be sorted in alphabetical order by tag name.
 *
 * Usage:
 *
 * TAGSORT *ts = tagsort_open(stdout, format, unique, passthru);
 * while (<getting new line>)
 * 	tagsort_put(ts, <line>);
 * tagsort_close(ts);
 *
 */
/*
 * This entry corresponds to one record of ctags [-x] format.
 */
struct dup_entry {
	int offset;
	int lineno;
	char *path;
	int pathlen;
	int savec;
};

static int compare_dup_entry(const void *, const void *);
static void put_lines(int, char *, struct dup_entry *, int, void (*output)(const char *));

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
 *	i)	lines	ctags stream
 *	i)	entries	sort target
 *	i)	entry_count number of entry of the entries
 *	i)	output	output function
 */
static void
put_lines(int unique, char *lines, struct dup_entry *entries, int entry_count, void (*output)(const char *))
{
	int i;
	char last_path[MAXPATHLEN+1];
	int last_lineno;
	/*
	 * Parse and sort ctags -x format records.
	 *
	 * PART_TAG     PART_LNO PART_PATH      PART_LINE
	 * +----------------------------------------------
	 * |main             227 src/main       main()
	 */
	for (i = 0; i < entry_count; i++) {
		struct dup_entry *e = &entries[i];
		char *ctags_x = lines + e->offset;
		SPLIT ptable;

		if (split(ctags_x, 4, &ptable) < 4) {
			recover(&ptable);
			die("too small number of parts.\n'%s'", ctags_x);
		}
		e->lineno = atoi(ptable.part[PART_LNO].start);
		e->path = ptable.part[PART_PATH].start;
		e->savec = ptable.part[PART_PATH].savec;
		e->pathlen = ptable.part[PART_PATH].end - ptable.part[PART_PATH].start;
		recover(&ptable);
		if (e->savec)
			e->path[e->pathlen] = '\0';
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
		if (e->savec)
			e->path[e->pathlen] = e->savec;
		if (!skip) {
			char *p = lines + e->offset;
			output(p);
		}
	}
}
/*
 * tagsort_open: open sort filter
 *
 *	i)	output	output function
 *	i)	format	tag format
 *	i)	unique	1: make the output unique.
 *	i)	passthru 1: pass through
 *	r)		tagsort structure
 */
TAGSORT *
tagsort_open(void (*output)(const char *), int format, int unique, int passthru)
{
	TAGSORT *ts = (TAGSORT *)check_calloc(sizeof(TAGSORT), 1);

	if (!passthru) {
		ts->sb = strbuf_open(MAXBUFLEN);
		ts->vb = varray_open(sizeof(struct dup_entry), 200);
		ts->prev[0] = '\0';
	}
	ts->output = output;
	ts->format = format;
	ts->unique = unique;
	ts->passthru = passthru;

	return ts;
}
/*
 * tagsort_put: sort the output
 *
 *	i)	ts	tagsort structure
 *	i)	line	record
 */
void
tagsort_put(TAGSORT *ts, const char *line)	/* virtually const */
{
	const char *tag;
	struct dup_entry *entry;
	SPLIT ptable;

	/*
	 * pass through.
	 */
	if (ts->passthru) {
		ts->output(line);
		return;
	}
	/*
	 * extract the tag name.
	 */
	if (split((char *)line, 2, &ptable) < 2) {
		recover(&ptable);
		die("too small number of parts.\n'%s'", line);
	}
	/*
	 * collect the records with the same tag, sort and write
	 * using put_lines().
	 */
	tag = ptable.part[PART_TAG].start;
	if (strcmp(ts->prev, tag) != 0) {
		if (ts->prev[0] != '\0') {
			if (ts->vb->length == 1) {
				ts->output(strbuf_value(ts->sb));
			} else
				put_lines(ts->unique,
					strbuf_value(ts->sb),
					varray_assign(ts->vb, 0, 0),
					ts->vb->length,
					ts->output);
		}
		strlimcpy(ts->prev, tag, sizeof(ts->prev));
		strbuf_reset(ts->sb);
		varray_reset(ts->vb);
	}
	entry = varray_append(ts->vb);
	entry->offset = strbuf_getlen(ts->sb);
	recover(&ptable);
	strbuf_puts0(ts->sb, line);
}
/*
 * tagsort_close: close sort filter
 *
 *	i)	ts	tagsort structure
 */
void
tagsort_close(TAGSORT *ts)
{
	if (!ts->passthru) {
		if (ts->prev[0] != '\0') {
			if (ts->vb->length == 1) {
				ts->output(strbuf_value(ts->sb));
			} else
				put_lines(ts->unique,
					strbuf_value(ts->sb),
					varray_assign(ts->vb, 0, 0),
					ts->vb->length,
					ts->output);
		}
		strbuf_close(ts->sb);
		varray_close(ts->vb);
	}
	(void)free(ts);
}
