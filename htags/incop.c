/*
 * Copyright (c) 2003, 2004 Tama Communications Corporation
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
#include "config.h"
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#include "queue.h"
#include "global.h"
#include "incop.h"
/*----------------------------------------------------------------------*/
/* Pool									*/
/*----------------------------------------------------------------------*/
SLIST_HEAD(pool, data);

/*
 * open_pool: open new string pool.
 */
static struct pool *
open_pool()
{
	struct pool *head = (struct pool *)malloc(sizeof(struct pool));

	if (!head)
		die("short of memory.");
	SLIST_INIT(head);

	return head;
}
/*
 * put_pool: put string into the pool.
 *
 *	i)	head	pool header
 *	i)	name	string name
 *	i)	contents string contents
 *	i)	id	string id
 */
static void
put_pool(head, name, contents, id)
	struct pool *head;
	char *name;
	char *contents;
	int id;
{
	struct data *data;

	if (strlen(name) > MAXPATHLEN)
		die("name is too long.");
	SLIST_FOREACH(data, head, next) {
		if (!strcmp(data->name, name))
			break;
	}
	if (!data) {
		data = (struct data *)malloc(sizeof(struct data));
		if (!data)
			die("short of memory.");
		strlimcpy(data->name, name, sizeof(data->name));
		data->id = id;
		data->contents = strbuf_open(0);
		data->count = 0;
		SLIST_INSERT_HEAD(head, data, next);
	}
	strbuf_puts0(data->contents, contents);
	data->count++;
}
/*
 * get_pool: get string pool.
 *
 *	i)	name	name of string pool
 *	r)		descriptor
 */
static struct data *
get_pool(head, name)
	struct pool *head;
	char *name;
{
	struct data *data;

	SLIST_FOREACH(data, head, next) {
		if (!strcmp(data->name, name))
			break;
	}
	return data;
}
/*
 * first_data: get the first data in the pool.
 *
 *	r)		descriptor
 */
static struct data *
first_data(head)
	struct pool *head;
{
	return SLIST_FIRST(head);
}
/*
 * next_data: get the next data in the pool.
 *
 *	r)		descriptor
 */
static struct data *
next_data(data)
	struct data *data;
{
	return SLIST_NEXT(data, next);
}
/*
 * Terminate function is not needed.
 */
/*----------------------------------------------------------------------*/
/* Include path list							*/
/*----------------------------------------------------------------------*/
static struct pool* head_inc;
static struct data *cur_inc;

static struct pool* head_included;
static struct data *cur_included;

/*
 * init_inc: initialize include file list.
 */
void
init_inc()
{
	head_inc = open_pool();
	head_included = open_pool();
}
/*
 * put_inc: put include file.
 *
 *	i)	file	file name (the last component of the path)
 *	i)	path	path name or command line.
 *	i)	id	path id
 */
void
put_inc(file, path, id)
	char *file;
	char *path;
	int id;
{
	put_pool(head_inc, file, path, id);
}
/*
 * get_inc: get include file.
 *
 *	i)	path	path name or command line.
 *	r)		descriptor
 */
struct data *
get_inc(name)
	char *name;
{
	return get_pool(head_inc, name);
}
/*
 * first_inc: get the first include file.
 *
 *	r)		descriptor
 */
struct data *
first_inc()
{
	return cur_inc = first_data(head_inc);
}
/*
 * next_inc: get the next include file.
 *
 *	r)		descriptor
 */
struct data *
next_inc()
{
	return cur_inc = next_data(cur_inc);
}


/*
 * put_included: put include file.
 *
 *	i)	file	file name (the last component of the path)
 *	i)	path	path name or command line.
 *	i)	id	path id
 */
void
put_included(file, path)
	char *file;
	char *path;
{
	put_pool(head_included, file, path, 0);
}
/*
 * get_included: get included file.
 *
 *	i)	path	path name or command line.
 *	r)		descriptor
 */
struct data *
get_included(name)
	char *name;
{
	return get_pool(head_included, name);
}
/*
 * first_included: get the first included file.
 *
 *	r)		descriptor
 */
struct data *
first_included()
{
	return cur_included = first_data(head_included);
}
/*
 * next_included: get the next included file.
 *
 *	r)		descriptor
 */
struct data *
next_included()
{
	return cur_included = next_data(cur_included);
}
/*
 * Terminate function is not needed.
 */
