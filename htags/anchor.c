/*
 * Copyright (c) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004
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
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "global.h"
#include "anchor.h"
#include "htags.h"

static struct anchor *table;
static VARRAY *vb;

static struct anchor *start;
static struct anchor *curp;
static struct anchor *end;
static struct anchor *CURRENT;

/* compare routine for qsort(3) */
static int
cmp(s1, s2)
	const void *s1, *s2;
{
	return ((struct anchor *)s1)->lineno - ((struct anchor *)s2)->lineno;
}
/*
 * Pointers (as lineno).
 */
static int FIRST;
static int LAST;
static struct anchor *CURRENTDEF;

/*
 * anchor_load: load anchor table
 *
 *	i)	file	file name
 */
void
anchor_load(file)
	char *file;
{
	char command[MAXFILLEN];
	char *options[] = {NULL, "", "r", "s"};
	STRBUF *sb = strbuf_open(0);
	FILE *ip;
	int i, db;

	FIRST = LAST = 0;
	end = CURRENT = NULL;

	if (vb == NULL)
		vb = varray_open(sizeof(struct anchor), 1000);
	else
		varray_reset(vb);

	for (db = GTAGS; db < GTAGLIM; db++) {
		char *_;

		if (!symbol && db == GSYMS)
			continue;
		/*
		 * Setup input stream.
		 */
		snprintf(command, sizeof(command), "global -fn%s \"%s\"", options[db], file);
		ip = popen(command, "r");
		if (ip == NULL)
			die("cannot execute command '%s'.", command);
		while ((_ = strbuf_fgets(sb, ip, STRBUF_NOCRLF)) != NULL) {
			SPLIT ptable;
			struct anchor *a;
			int type;

			if (split(_, 4, &ptable) < 4) {
				recover(&ptable);
				die("too small number of parts in anchor_load().\n'%s'", _);
			}
			if (db == GTAGS) {
				char *p;

				for (p = ptable.part[PART_LINE].start; *p && isspace((unsigned char)*p); p++)
					;
				if (!*p) {
					recover(&ptable);
					die("The output of global is illegal.\n%s", p);
				}
				type = 'D';
				if (*p == '#') {
					for (p++; *p && isspace((unsigned char)*p); p++)
						;
					if (*p) {
						if (!strncmp(p, "define", sizeof("define") - 1))
							type = 'M';
						else if (!strncmp(p, "undef", sizeof("undef") - 1))
							type = 'M';
					}
				} else if (!strncmp(p, "typedef", sizeof("typedef") - 1))
					type = 'T';
			}  else if (db == GRTAGS)
				type = 'R';
			else
				type = 'Y';
			/* allocate an entry */
			a = varray_append(vb);
			a->lineno = atoi(ptable.part[PART_LNO].start);
			a->type = type;
			a->done = 0;
			settag(a, ptable.part[PART_TAG].start);
			recover(&ptable);
		}
		if (pclose(ip) != 0)
			die("command '%s' failed.", command);
	}
	strbuf_close(sb);
	if (vb->length == 0) {
		table = NULL;
	} else {
		int used = vb->length;
		/*
		 * Sort by lineno.
		 */
		table = varray_assign(vb, 0, 0);
		qsort(table, used, sizeof(struct anchor), cmp); 
		/*
		 * Setup some lineno.
		 */
		for (i = 0; i < used; i++)
			if (table[i].type == 'D')
				break;
		if (i < used)
			FIRST = table[i].lineno;
		for (i = used - 1; i >= 0; i--)
			if (table[i].type == 'D')
				break;
		if (i >= 0)
			LAST = table[i].lineno;
	}
	/*
	 * Setup loop range.
	 */
	start = table;
	curp = NULL;
	end = &table[vb->length];
	/* anchor_dump(stderr, 0);*/
}
/*
 * anchor_unload: unload anchor table
 */
void
anchor_unload()
{
	struct anchor *a;

	for (a = start; a && a < end; a++) {
		if (a->reserve) {
			free(a->reserve);
			a->reserve = NULL;
		}
	}
	/* We don't free varray */
	/* varray_close(vb); */
	FIRST = LAST = 0;
	start = curp = end = NULL;
}
/*
 * anchor_first: return the first anchor
 */
struct anchor *
anchor_first()
{
	if (!start || start == end)
		return NULL;
	CURRENT = start;
	if (CURRENT->type == 'D')
		CURRENTDEF = CURRENT;
	return CURRENT;
}
/*
 * anchor_next: return the next anchor
 */
struct anchor *
anchor_next()
{
	if (!start)
		return NULL;
	if (++CURRENT >= end)
		return NULL;
	if (CURRENT->type == 'D')
		CURRENTDEF = CURRENT;
	return CURRENT;
}
/*
 * anchor_get: return the specified anchor
 *
 *	i)	name	name of anchor
 *	i)	length	lenght of the name
 *	i)	type	==0: not specified
 *			!=0: D, M, T, R, Y
 */
struct anchor *
anchor_get(name, length, type, lineno)
	char *name;
	int length;
	int type;
	int lineno;
{
	struct anchor *p = curp ? curp : start;

	if (table == NULL)
		return NULL;
	if (p->lineno > lineno)
		return NULL;
	/*
	 * set pointer to the top of the cluster.
	 */
	for (; p < end && p->lineno < lineno; p++)
		;
	if (p >= end || p->lineno != lineno)
		return NULL;
	curp = p;
	for (; p < end && p->lineno == lineno; p++)
		if (!p->done && p->length == length && !strcmp(gettag(p), name))
			if (!type || p->type == type)
				return p;
	return NULL;
}
/*
 * define_line: check whether or not this is a define line.
 *
 *	i)	lineno	line number
 *	go)	curp	pointer to the current cluster
 *	r)		1: definition, 0: not definition
 */
int
define_line(lineno)
	int lineno;
{
	struct anchor *p = curp ? curp : start;

	if (table == NULL)
		return 0;
	if (p->lineno > lineno)
		return 0;
	/*
	 * set pointer to the top of the cluster.
	 */
	for (; p < end && p->lineno < lineno; p++)
		;
	if (p >= end || p->lineno != lineno)
		return 0;
	curp = p;
	for (; p < end && p->lineno == lineno; p++)
		if (p->type == 'D')
			return 1;
	return 0;
}
/*
 * anchor_getlinks: return anchor link array
 *		(previous, next, first, last, top, bottom)
 */
int *
anchor_getlinks(lineno)
	int lineno;
{
	static int ref[A_SIZE];
	int i;

	for (i = 0; i < A_SIZE; i++)
		ref[i] = 0;
	if (lineno >= 1 && start) {
		struct anchor *c, *p;

		if (CURRENTDEF == NULL) {
			for (c = start; c < end; c++)
				if (c->lineno == lineno && c->type == 'D')
					break;
			CURRENTDEF = c;
		} else {
			for (c = CURRENTDEF; c >= start; c--)
				if (c->lineno == lineno && c->type == 'D')
					break;
		}
		for (p = c - 1; p >= start; p--)
			if (p->type == 'D') {
				ref[A_PREV] = p->lineno;
				break;
			}
		for (p = c + 1; p < end; p++)
			if (p->type == 'D') {
				ref[A_NEXT] = p->lineno;
				break;
			}
	}
	if (FIRST > 0 && lineno != FIRST)
		ref[A_FIRST] = FIRST;
	if (LAST > 0 && lineno != LAST)
		ref[A_LAST] = LAST;
	if (lineno != 0)
		ref[A_TOP] = -1;
	if (lineno != -1)
		ref[A_BOTTOM] = -2;
	if (FIRST > 0 && FIRST == LAST) {
		if (lineno == 0)
			ref[A_LAST] = 0;
		if (lineno == -1)
			ref[A_FIRST] = 0;
	}
	return ref;
}
void
anchor_dump(op, lineno)
	FILE *op;
	int lineno;
{
	struct anchor *a;

	if (op == NULL)
		op = stderr;
	for (a = start; a < end ; a++)
		if (lineno == 0 || a->lineno == lineno)
			fprintf(op, "%d %s(%c)\n",
				a->lineno, gettag(a), a->type);
}
