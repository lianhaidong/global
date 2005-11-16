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
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>

#include "die.h"
#include "gparam.h"
#include "split.h"
#include "strbuf.h"
#include "strlimcpy.h"
#include "tagsort.h"
#include "varray.h"

/*
 * A special version of sort command.
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
 * 1. input must be ctags -x format.
 * 2. input must be sorted in alphabetical order by tag name.
 *
 * usage: read from stdin, sort and write it to stdout.
 *
 * tagsort(0, stdin, stdout);
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
 *	i)	ctags	format of the records
 *			0: ctags -x format, 1: ctags format
 *	i)	lines	ctags stream
 *	i)	entries	sort target
 *	i)	entry_count number of entry of the entries
 *	i)	op	output file
 */
static void
put_lines(int unique, int ctags, char *lines, struct dup_entry *entries, int entry_count, FILE *op)
{
	int i;
	char last_path[MAXPATHLEN+1];
	int last_lineno;
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
	int splits = ctags ? 3 : 4;
	int part_lno = ctags ? PART_CTAGS_LNO : PART_LNO;
	int part_path = ctags ? PART_CTAGS_PATH : PART_PATH;

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
 * main body of sorting.
 *
 *	i)	unique	0: sort, 1: sort -u
 *	i)	ctags	0: ctags -x format, 1: ctags format
 *	i)	ip	input
 *	i)	op	output
 */
void
tagsort(int unique, int ctags, FILE *ip, FILE *op)
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
						ctags,
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
				ctags,
				strbuf_value(sb),
				varray_assign(vb, 0, 0),
				vb->length,
				op);
	}
	strbuf_close(ib);
	strbuf_close(sb);
	varray_close(vb);
}
