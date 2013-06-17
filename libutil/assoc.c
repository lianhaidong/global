/*
 * Copyright (c) 2004, 2010, 2013 Tama Communications Corporation
 *
 * This file is part of GNU GLOBAL.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "checkalloc.h"
#include "die.h"
#include "assoc.h"

/**
 * assoc_open: open associate array.
 *
 *	@return		descriptor
 */
ASSOC *
assoc_open(void)
{
	ASSOC *assoc = (ASSOC *)check_malloc(sizeof(ASSOC));

	/*
	 * Use invisible temporary file.
	 */
	assoc->db = dbopen(NULL, O_RDWR|O_CREAT|O_TRUNC, 0600, DB_BTREE, NULL);
	if (assoc->db == NULL)
		die("cannot make associate array.");
	return assoc;
}
/**
 * assoc_close: close associate array.
 *
 *	@param[in]	assoc	descriptor
 */
void
assoc_close(ASSOC *assoc)
{
	if (assoc == NULL)
		return;
	if (assoc->db == NULL)
		return;
#ifdef USE_DB185_COMPAT
	(void)assoc->db->close(assoc->db);
#else
	/*
	 * If dbname = NULL, omit writing to the disk in __bt_close().
	 */
	(void)assoc->db->close(assoc->db, 1);
#endif
	free(assoc);
}
/**
 * assoc_put: put data into associate array.
 *
 *	@param[in]	assoc	descriptor
 *	@param[in]	name	name
 *	@param[in]	value	value
 */
void
assoc_put(ASSOC *assoc, const char *name, const char *value)
{
	DB *db = assoc->db;
	DBT key, dat;
	int status;

	if (db == NULL)
		die("associate array is not prepared.");
	if (strlen(name) == 0)
		die("primary key size == 0.");
	key.data = (char *)name;
	key.size = strlen(name)+1;
	dat.data = (char *)value;
	dat.size = strlen(value)+1;

	status = (*db->put)(db, &key, &dat, 0);
	switch (status) {
	case RET_SUCCESS:
		break;
	case RET_ERROR:
	case RET_SPECIAL:
		die("cannot write to the associate array. (assoc_put)");
	}
}
/**
 * assoc_put_withlen: put data into associate array.
 *
 *      @param[in]      assoc   descriptor
 *      @param[in]      name    name
 *      @param[in]      value   value
 *      @param[in]      length  length of value
 */
void
assoc_put_withlen(ASSOC *assoc, const char *name, const char *value, int length)
{
	DB *db = assoc->db;
	DBT key, dat;
	int status;

	if (db == NULL)
		die("associate array is not prepared.");
	if (strlen(name) == 0)
		die("primary key size == 0.");
	key.data = (char *)name;
	key.size = strlen(name)+1;
	dat.data = (char *)value;
	dat.size = length;

	status = (*db->put)(db, &key, &dat, 0);
	switch (status) {
	case RET_SUCCESS:
		break;
	case RET_ERROR:
	case RET_SPECIAL:
		die("cannot write to the associate array. (assoc_put)");
	}
}
/**
 * assoc_get: get data from associate array.
 *
 *	@param[in]	assoc	descriptor
 *	@param[in]	name	name
 *	@return		value
 */
const char *
assoc_get(ASSOC *assoc, const char *name)
{
	DB *db = assoc->db;
	DBT key, dat;
	int status;

	if (db == NULL)
		die("associate array is not prepared.");
	key.data = (char *)name;
	key.size = strlen(name)+1;

	status = (*db->get)(db, &key, &dat, 0);
	switch (status) {
	case RET_SUCCESS:
		break;
	case RET_ERROR:
		die("cannot read from the associate array. (assoc_get)");
	case RET_SPECIAL:
		return (NULL);
	}
	return (dat.data);
}
