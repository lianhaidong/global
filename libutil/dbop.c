/*
 * Copyright (c) 1997, 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000, 2001, 2002, 2003 Tama Communications Corporation
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
#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>
#include <fcntl.h>
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

#include "dbop.h"
#include "die.h"
#include "locatestring.h"
#include "strbuf.h"
#include "strlimcpy.h"
#include "strmake.h"
#include "test.h"

#ifdef USE_POSTGRES

PGconn *conn = NULL;

static int pgop_load_tagfile(const char *, STRBUF *);
static int pgop_extract_from_tagfile(const char *, const char *, char *, int);
DBOP *pgop_open(const char *, int, int, int);
char *pgop_get(DBOP *, const char *);
void pgop_put(DBOP *, const char *, const char *, const char *);
void pgop_delete(DBOP *, const char *);
char *pgop_getkey_by_fid(DBOP *, const char *);
void pgop_delete_by_fid(DBOP *, const char *);
void pgop_update(DBOP *, const char *, const char *, const char *);
char *pgop_first(DBOP *, const char *, regex_t *, int);
char *pgop_next(DBOP *);
char *pgop_lastdat(DBOP *);
void pgop_close(DBOP *);
#endif /* USE_POSTGRES */

int print_statistics = 0;
char *postgres_info;

/*
 * dbop_setinfo: set info string.
 *
 *	i)	info	info string
 *
 * Currently this method is used only for postgres.
 */
void
dbop_setinfo(info)
	char *info;
{
	postgres_info = info;
}
/*
 * dbop_open: open db database.
 *
 *	i)	path	database name
 *	i)	mode	0: read only, 1: create, 2: modify
 *	i)	perm	file permission
 *	i)	flags
 *			DBOP_DUP: allow duplicate records.
 *			DBOP_REMOVE: remove on closed.
 *			DBOP_POSTGRES: use postgres database
 *	r)		descripter for dbop_xxx()
 */
DBOP *
dbop_open(path, mode, perm, flags)
	const char *path;
	int mode;
	int perm;
	int flags;
{
	DB *db;
	int rw = 0;
	DBOP *dbop;
	BTREEINFO info;

#ifdef USE_POSTGRES
	if (mode != 1 && pgop_load_tagfile(path, NULL))
		flags |= DBOP_POSTGRES;
	if (flags & DBOP_POSTGRES)
		return pgop_open(path, mode, perm, flags);
#endif /* USE_POSTGRES */
	/*
	 * setup arguments.
	 */
	switch (mode) {
	case 0:
		rw = O_RDONLY;
		break;
	case 1:
		rw = O_RDWR|O_CREAT|O_TRUNC;
		break;
	case 2:
		rw = O_RDWR;
		break;
	default:
		assert(0);
	}
	memset(&info, 0, sizeof(info));
	if (flags & DBOP_DUP)
		info.flags |= R_DUP;
	info.psize = DBOP_PAGESIZE;
	/*
	 * accept user's request but needs 0.5MB at least.
	 */
	if (getenv("GTAGSCACHE") != NULL)
		info.cachesize = atoi(getenv("GTAGSCACHE"));
	if (info.cachesize < 500000)
		info.cachesize = 500000;

	/*
	 * if unlink do job normally, those who already open tag file can use
	 * it until closing.
	 */
	if (mode == 1 && test("f", path))
		(void)unlink(path);
	db = dbopen(path, rw, 0600, DB_BTREE, &info);
	if (!db)
		return NULL;
	if (!(dbop = (DBOP *)malloc(sizeof(DBOP))))
		die("short of memory.");
	strlimcpy(dbop->dbname, path, sizeof(dbop->dbname));
	dbop->db	= db;
	dbop->openflags	= flags;
	dbop->perm	= (mode == 1) ? perm : 0;
	dbop->lastdat	= NULL;

	return dbop;
}
/*
 * dbop_get: get data by a key.
 *
 *	i)	dbop	descripter
 *	i)	name	name
 *	r)		pointer to data
 */
char *
dbop_get(dbop, name)
	DBOP *dbop;
	const char *name;
{
	DB *db = dbop->db;
	DBT key, dat;
	int status;

#ifdef USE_POSTGRES
	if (dbop->openflags & DBOP_POSTGRES)
		return pgop_get(dbop, name);
#endif
	key.data = (char *)name;
	key.size = strlen(name)+1;

	status = (*db->get)(db, &key, &dat, 0);
	dbop->lastdat	= (char *)dat.data;
	switch (status) {
	case RET_SUCCESS:
		break;
	case RET_ERROR:
		die("cannot read from database.");
	case RET_SPECIAL:
		return (NULL);
	}
	return((char *)dat.data);
}
/*
 * dbop_put: put data by a key.
 *
 *	i)	dbop	descripter
 *	i)	name	key
 *	i)	data	data
 *	i)	fid	file id (only for postgres)
 */
void
dbop_put(dbop, name, data, fid)
	DBOP *dbop;
	const char *name;
	const char *data;
	const char *fid;
{
	DB *db = dbop->db;
	DBT key, dat;
	int status;
	int len;

	if (!(len = strlen(name)))
		die("primary key size == 0.");
	if (len > MAXKEYLEN)
		die("primary key too long.");
#ifdef USE_POSTGRES
	if (dbop->openflags & DBOP_POSTGRES)
		return pgop_put(dbop, name, data, fid);
#endif
	key.data = (char *)name;
	key.size = strlen(name)+1;
	dat.data = (char *)data;
	dat.size = strlen(data)+1;

	status = (*db->put)(db, &key, &dat, 0);
	switch (status) {
	case RET_SUCCESS:
		break;
	case RET_ERROR:
	case RET_SPECIAL:
		die("cannot write to database.");
	}
}
/*
 * dbop_delete: delete record by path name.
 *
 *	i)	dbop	descripter
 *	i)	path	path name
 */
void
dbop_delete(dbop, path)
	DBOP *dbop;
	const char *path;
{
	DB *db = dbop->db;
	DBT key;
	int status;

#ifdef USE_POSTGRES
	if (dbop->openflags & DBOP_POSTGRES)
		return pgop_delete(dbop, path);
#endif
	if (path) {
		key.data = (char *)path;
		key.size = strlen(path)+1;
		status = (*db->del)(db, &key, 0);
	} else
		status = (*db->del)(db, &key, R_CURSOR);
	if (status == RET_ERROR)
		die("cannot delete record.");
}
/*
 * dbop_update: update record.
 *
 *	i)	dbop	descripter
 *	i)	key	key
 *	i)	dat	data
 *	i)	fid	file id
 */
void
dbop_update(dbop, key, dat, fid)
	DBOP *dbop;
	const char *key;
	const char *dat;
	const char *fid;
{
#ifdef USE_POSTGRES
	if (dbop->openflags & DBOP_POSTGRES)
		pgop_update(dbop, key, dat, fid);
	else
#endif
		dbop_put(dbop, key, dat, fid);
}
#ifdef USE_POSTGRES
/*
 * dbop_getkey_by_fid: get key by fid.
 *
 *	i)	dbop	descripter
 *	i)	fid	file id
 *	r)		pointer to data
 */
char	*
dbop_getkey_by_fid(dbop, fid)
	DBOP *dbop;
	const char *fid;
{
	assert(dbop->openflags & DBOP_POSTGRES);
	return pgop_getkey_by_fid(dbop, fid);
}
/*
 * dbop_delete_by_fid: delete record by fid.
 *
 *	i)	dbop	descripter
 *	i)	fid	file id
 */
void
dbop_delete_by_fid(dbop, fid)
	DBOP *dbop;
	const char *fid;
{
	assert(dbop->openflags & DBOP_POSTGRES);
	return pgop_delete_by_fid(dbop, fid);
}
#endif
/*
 * dbop_first: get first record. 
 * 
 *	i)	dbop	dbop descripter
 *	i)	name	key value or prefix
 *			!=NULL: indexed read by key
 *			==NULL: sequential read
 *	i)	preg	compiled regular expression if any.
 *	i)	flags	following dbop_next call take over this.
 *			DBOP_KEY	read key part
 *			DBOP_PREFIX	prefix read
 *					only valied when sequential read
 *	r)		data
 */
char	*
dbop_first(dbop, name, preg, flags)
DBOP *dbop;
const char *name;
regex_t *preg;
int	flags;
{
	DB *db = dbop->db;
	DBT key, dat;
	int status;

#ifdef USE_POSTGRES
	if (dbop->openflags & DBOP_POSTGRES)
		return pgop_first(dbop, name, preg, flags);
#endif

	dbop->preg = preg;
	if (flags & DBOP_PREFIX && !name)
		flags &= ~DBOP_PREFIX;
	if (name) {
		if (strlen(name) > MAXKEYLEN)
			die("primary key too long.");
		strlimcpy(dbop->key, name, sizeof(dbop->key));
		key.data = (char *)name;
		key.size = strlen(name);
		/*
		 * includes NULL character unless prefix read.
		 */
		if (!(flags & DBOP_PREFIX))
			key.size++;
		dbop->keylen = key.size;
		for (status = (*db->seq)(db, &key, &dat, R_CURSOR);
			status == RET_SUCCESS;
			status = (*db->seq)(db, &key, &dat, R_NEXT)) {
			if (flags & DBOP_PREFIX) {
				if (strncmp((char *)key.data, dbop->key, dbop->keylen))
					return NULL;
			} else {
				if (strcmp((char *)key.data, dbop->key))
					return NULL;
			}
			if (preg && regexec(preg, (char *)key.data, 0, 0, 0) != 0)
				continue;
			break;
		}
	} else {
		dbop->keylen = dbop->key[0] = 0;
		for (status = (*db->seq)(db, &key, &dat, R_FIRST);
			status == RET_SUCCESS;
			status = (*db->seq)(db, &key, &dat, R_NEXT)) {
			if (*((char *)key.data) == ' ')	/* meta record */
				continue;
			if (preg && regexec(preg, (char *)key.data, 0, 0, 0) != 0)
				continue;
			break;
		}
	}
	dbop->lastdat	= (char *)dat.data;
	switch (status) {
	case RET_SUCCESS:
		break;
	case RET_ERROR:
		die("dbop_first failed.");
	case RET_SPECIAL:
		return (NULL);
	}
	dbop->ioflags = flags;
	if (flags & DBOP_KEY) {
		strlimcpy(dbop->prev, (char *)key.data, sizeof(dbop->prev));
		return (char *)key.data;
	}
	return ((char *)dat.data);
}
/*
 * dbop_next: get next record. 
 * 
 *	i)	dbop	dbop descripter
 *	r)		data
 *
 * Db_next always skip meta records.
 */
char *
dbop_next(dbop)
	DBOP *dbop;
{
	DB *db = dbop->db;
	int flags = dbop->ioflags;
	DBT key, dat;
	int status;

#ifdef USE_POSTGRES
	if (dbop->openflags & DBOP_POSTGRES)
		return pgop_next(dbop);
#endif

	while ((status = (*db->seq)(db, &key, &dat, R_NEXT)) == RET_SUCCESS) {
		assert(dat.data != NULL);
		if (flags & DBOP_KEY && *((char *)key.data) == ' ')
			continue;
		else if (*((char *)dat.data) == ' ')
			continue;
		if (flags & DBOP_KEY) {
			if (!strcmp(dbop->prev, (char *)key.data))
				continue;
			if (strlen((char *)key.data) > MAXKEYLEN)
				die("primary key too long.");
			strlimcpy(dbop->prev, (char *)key.data, sizeof(dbop->prev));
		}
		dbop->lastdat	= (char *)dat.data;
		if (flags & DBOP_PREFIX) {
			if (strncmp((char *)key.data, dbop->key, dbop->keylen))
				return NULL;
		} else if (dbop->keylen) {
			if (strcmp((char *)key.data, dbop->key))
				return NULL;
		}
		if (dbop->preg && regexec(dbop->preg, (char *)key.data, 0, 0, 0) != 0)
			continue;
		return (flags & DBOP_KEY) ? (char *)key.data : (char *)dat.data;
	}
	if (status == RET_ERROR)
		die("dbop_next failed.");
	return NULL;
}
/*
 * dbop_lastdat: get last data
 * 
 *	i)	dbop	dbop descripter
 *	r)		last data
 */
char *
dbop_lastdat(dbop)
	DBOP *dbop;
{
	return dbop->lastdat;
}
/*
 * dbop_close: close db
 * 
 *	i)	dbop	dbop descripter
 */
void
dbop_close(dbop)
	DBOP *dbop;
{
	DB *db = dbop->db;

#ifdef USE_POSTGRES
	if (dbop->openflags & DBOP_POSTGRES)
		return pgop_close(dbop);
#endif

	(void)db->close(db);
	if (dbop->openflags & DBOP_REMOVE)
		(void)unlink(dbop->dbname);
	else if (dbop->perm && chmod(dbop->dbname, dbop->perm) < 0)
		die("cannot change file mode.");
	(void)free(dbop);
}
#ifdef USE_POSTGRES
/*
 * pgop_execute: execute sql statement.
 *
 *	i)	dbop	dbop descripter
 *	i)	sql	sql statement
 *	i)	line	line number
 */
static void
pgop_execute(dbop, sql, line)
	DBOP *dbop;
	char *sql;
	int line;
{
	int ignore = 0;
	extern int debug;

	if (debug)
		fprintf(stderr, "sql: '%s' at %d\n", sql, line);
	if (dbop->res)
		PQclear(dbop->res);
	if (*sql == '@') {
		dbop->res = PQexec(conn, sql + 1);
	} else {
		dbop->res = PQexec(conn, sql);
		if (!dbop->res ||
		PQresultStatus(dbop->res) != PGRES_COMMAND_OK &&
		PQresultStatus(dbop->res) != PGRES_TUPLES_OK) {
			fprintf(stderr, "%s: SQL statement failed at %d.\n", progname, line);
			fprintf(stderr, "sql: %s\n", sql);
			fprintf(stderr, "postgres: %s", PQerrorMessage(conn));
			PQfinish(conn);
			abort();
			exit(1);
		}
	}
}
/*
 * pgop_connect: connect to databse server.
 *
 *	i)	info	connect string
 */
static void
pgop_connect(info)
	const char *info;
{
	if (conn)
		return;
	conn = PQconnectdb(info);
	if (PQstatus(conn) == CONNECTION_BAD) {
		fprintf(stderr, "%s: Connection to database failed.\n", progname);
		fprintf(stderr, "PQconnectdb('%s')", info);
		fprintf(stderr, "postgres: %s", PQerrorMessage(conn));
		PQfinish(conn);
		exit(1);
	}
}
/*
 * pgop_load_tagfile: load tag file.
 *
 *	i)	path	path of tag file
 *	o)	sb	contents of tag file
 *	r)		1: postgres tag file loaded
 *			0: file not exist or is not postgres tag file
 */
static int
pgop_load_tagfile(path, sb)
	const char *path;
	STRBUF *sb;
{
	char *p;
	int alloc = 0;
	int ispostgres = 0;
	FILE *fp = fopen(path, "r");

	if (fp == NULL)
		return 0;
	if (sb == NULL) {
		sb = strbuf_open(0);
		alloc = 1;
	}
	if (strbuf_fgets(sb, fp, STRBUF_NOCRLF) == NULL)
		die("%s is empty.", path);
	fclose(fp);
	p = strbuf_value(sb);
	if (!strncmp(p, "postgres: ", strlen("postgres: ")))
		ispostgres = 1;
	if (alloc)
		strbuf_close(sb);
	return ispostgres;
}
/*
 * pgop_extract_from_tagfile: extract a parameter from loaded tag file.
 *
 *	i)	contents contents of loaded tag file
 *	i)	key	key
 *	o)	buf	buffer
 *	i)	size	size of buf
 *	r)		1: exist, 0: not exist
 */
static int
pgop_extract_from_tagfile(contents, key, buf, size)
	const char *contents;
	const char *key;
	char *buf;
	int size;
{
	STRBUF *sb = strbuf_open(0);
	char *p, *limit = buf + size;
	int i;

	strbuf_putc(sb, ' ');
	strbuf_puts(sb, key);
	strbuf_putc(sb, '=');
	p = locatestring(contents, strbuf_value(sb), MATCH_FIRST);
	if (p == NULL)
		return 0;
	p += strbuf_getlen(sb);
	while (*p && *p != ' ') {
		if (buf >= limit)
			die("parameter in postgres tag file too long. '%s'", key);
		*buf++ = *p++;
	}
	*buf = 0;
	return 1;
}
/*
 * pgop_open: open postgres table.
 *
 *	i)	path	tag file path
 *	i)	mode	0: read, 1: write, 2: update
 *	i)	perm	permission for chmod(2)
 *	i)	flags	flags
 */
DBOP *
pgop_open(path, mode, perm, flags)
	const char *path;
	int mode;
	int perm;
	int flags;
{
	DBOP *dbop;
	STRBUF *sb;
	char sql[1024], dir[MAXPATHLEN+1], *name;
	FILE *fp;
	int i;

	/*
	 * make dbop discripter.
 	 */
	if (!(dbop = (DBOP *)malloc(sizeof(DBOP))))
		die("short of memory.");
	/*
	 * make table name.
	 */
	snprintf(dir, sizeof(dir), "%s", path);
	if (!(name = strrchr(dir, '/'))) {
		name = (char *)path;
		dir[0] = 0;
	} else
		*name++ = 0;
	strlimcpy(dbop->tblname, name, sizeof(dbop->tblname));
	dbop->res = NULL;
	dbop->mode = mode;
	dbop->openflags = flags;
	dbop->lastdat   = NULL;

	if (mode == 1) {
		/*
		 * if the -P option includes info string then use it.
		 */
		if (postgres_info) {
			snprintf(sql, sizeof(sql), "postgres: %s\n", postgres_info);
		} else {
			/*
			 * else if GTAGS exist then load from 'GTAGS'.
			 */
			sb = strbuf_open(0);
			strcat(dir, "/GTAGS");
			if (pgop_load_tagfile(dir, sb))
				snprintf(sql, sizeof(sql), "%s\n", strbuf_value(sb));
			else
				die("Please specify info string for PQconnectdb(3) using --postgres='info'.");
			strbuf_close(sb);
		}
		fp = fopen(path, "w");
		if (fp == NULL)
			die("cannot create '%s'.", path);
		fputs(sql, fp);
		fclose(fp);
		chmod(path, perm);
	}
	/*
	 * load postgres tag file and make a connection.
	 */
	{
		char *p, *q;

		sb = strbuf_open(0);
		if (!pgop_load_tagfile(path, sb))
			die("%s not found.", path);
		/* skip 'postgres: ' */
		p = strbuf_value(sb);
		if (strchr(p, ':'))
			p = strchr(p, ':') + 1;
		while (*p == ' ')
			p++;
		/* make connection to postgres */
		pgop_connect(p);
		strbuf_close(sb);
	}

	/*
	 * create table (GTAGS, GRTAGS, GSYMS, GPATH).
	 */
	if (mode == 1) {
		snprintf(sql, sizeof(sql), "@DROP TABLE %s", dbop->tblname);
		pgop_execute(dbop, sql, __LINE__);
		snprintf(sql, sizeof(sql),
		"CREATE TABLE %s (key TEXT, dat TEXT, fid INT4)", dbop->tblname);
		pgop_execute(dbop, sql, __LINE__);
		snprintf(sql, sizeof(sql), "GRANT ALL ON %s TO public", dbop->tblname);
		pgop_execute(dbop, sql, __LINE__);
		/*
		 * Create order for speed up:
		 * GTAGS,GRTAGS,GSYMS
		 *		create table -> put data -> create index
		 * GPATH	create table -> create index -> put data
		 */
		if (!strcmp(name, "GPATH")) {
			snprintf(sql, sizeof(sql),
			"CREATE INDEX %s_i1 ON %s (key)", dbop->tblname, dbop->tblname);
			pgop_execute(dbop, sql, __LINE__);
			snprintf(sql, sizeof(sql),
			"CREATE INDEX %s_i2 ON %s (fid)", dbop->tblname, dbop->tblname);
			pgop_execute(dbop, sql, __LINE__);
		}
	}
	/* construct SQL statement. */
	/*
	 * pgop_delete(), pgop_delete_by_fid()
	 */
	sb = strbuf_open(0);
	strbuf_puts(sb, "DELETE FROM ");
	strbuf_puts(sb, dbop->tblname);
	strbuf_puts(sb, " WHERE ");
	strbuf_puts(sb, "fid = ");
	dbop->delete_stmt = sb;
	dbop->delete_stmt_len = strbuf_getlen(sb);
	/*
	 * pgop_get(), pgop_getkey_by_fid()
	 */
	sb = strbuf_open(0);
	strbuf_puts(sb, "SELECT * FROM ");
	strbuf_puts(sb, dbop->tblname);
	strbuf_puts(sb, " WHERE ");
	dbop->get_stmt = sb;
	dbop->get_stmt_len = strbuf_getlen(sb);
	/*
	 * pgop_put()
	 */
	sb = strbuf_open(0);
	strbuf_puts(sb, "INSERT INTO ");
	strbuf_puts(sb, dbop->tblname);
	strbuf_puts(sb, " VALUES ");
	dbop->put_stmt = sb;
	dbop->put_stmt_len = strbuf_getlen(sb);
	/*
	 * pgop_first(), pgop_next()
	 */
	sb = strbuf_open(0);
	strbuf_puts(sb, "FETCH 1 FROM c_");
	strbuf_puts(sb, dbop->tblname);
	dbop->fetch_stmt = sb;
	dbop->fetch_stmt_len = strbuf_getlen(sb);

        return dbop;
}

/*
 * pgop_get: get data by a key.
 *
 *	i)	dbop	descripter
 *	i)	name	name
 *	r)		pointer to data
 */
char	*
pgop_get(dbop, name)
	DBOP *dbop;
	const char *name;
{
	STRBUF *sb = dbop->get_stmt;

	strbuf_setlen(sb, dbop->get_stmt_len);		/* truncate */
	strbuf_puts(sb, "key = ");
	strbuf_putc(sb, '\'');
	strbuf_puts(sb, name);
	strbuf_putc(sb, '\'');
	pgop_execute(dbop, strbuf_value(sb), __LINE__);
	if (PQntuples(dbop->res) == 0)
		return NULL;
	dbop->lastdat = PQgetvalue(dbop->res, 0, 1);
	return dbop->lastdat;
}
/*
 * pgop_put: put data by a key.
 *
 *	i)	dbop	descripter
 *	i)	name	key
 *	i)	data	data
 *	i)	fid	file id
 */
void
pgop_put(dbop, key, dat, fid)
	DBOP *dbop;
	const char *key;
	const char *dat;
	const char *fid;
{
	STRBUF *sb = dbop->put_stmt;
	const char *p;

	strbuf_setlen(sb, dbop->put_stmt_len);		/* truncate */
	strbuf_puts(sb, "('");
	strbuf_puts(sb, key);
	strbuf_puts(sb, "', '");
	for (p = dat; *p; p++) {
		switch (*p) {
		case '\\':
		case '\'':
			strbuf_putc(sb, '\\');
			break;
		}
		strbuf_putc(sb, *p);
	}
	strbuf_puts(sb, "', ");
	strbuf_puts(sb, fid);
	strbuf_puts(sb, ")");
	pgop_execute(dbop, strbuf_value(sb), __LINE__);
}
/*
 * pgop_delete: delete record by a key.
 *
 *	i)	dbop	descripter
 *	i)	path	path name
 */
void
pgop_delete(dbop, path)
	DBOP *dbop;
	const char *path;
{
	STRBUF *sb = dbop->delete_stmt;

	strbuf_setlen(sb, dbop->delete_stmt_len);	/* truncate */
	strbuf_puts(sb, path);
	pgop_execute(dbop, strbuf_value(sb), __LINE__);
}
/*
 * pgop_getkey_by_fid: get key by fid.
 *
 *	i)	dbop	descripter
 *	i)	fid	file id
 *	r)		pointer to data
 */
char	*
pgop_getkey_by_fid(dbop, fid)
	DBOP *dbop;
	const char *fid;
{
	STRBUF *sb = dbop->get_stmt;

	strbuf_setlen(sb, dbop->get_stmt_len);	/* truncate */
	strbuf_puts(sb, "fid = ");
	strbuf_puts(sb, fid);
	pgop_execute(dbop, strbuf_value(sb), __LINE__);
	if (PQntuples(dbop->res) == 0)
		return NULL;
	dbop->lastdat = PQgetvalue(dbop->res, 0, 1);
	return PQgetvalue(dbop->res, 0, 0);
}
/*
 * pgop_delete_by_fid: delete record by fid.
 *
 *	i)	dbop	descripter
 *	i)	fid	file id
 */
void
pgop_delete_by_fid(dbop, fid)
	DBOP *dbop;
	const char *fid;
{
	STRBUF *sb = dbop->delete_stmt;

	strbuf_setlen(sb, dbop->delete_stmt_len);	/* truncate */
	strbuf_puts(sb, fid);
	pgop_execute(dbop, strbuf_value(sb), __LINE__);
}
/*
 * pgop_update: update record.
 *
 *	i)	dbop	descripter
 *	i)	key	key
 *	i)	dat	data
 *	i)	fid	file id
 */
void
pgop_update(dbop, key, dat, fid)
	DBOP *dbop;
	const char *key;
	const char *dat;
	const char *fid;
{
	char *p;
	STRBUF *sb = strbuf_open(0);

	strbuf_puts(sb, "UPDATE ");
	strbuf_puts(sb, dbop->tblname);
	strbuf_puts(sb, " SET dat = ");
	strbuf_puts(sb, dat);
	strbuf_puts(sb, " WHERE key = '");
	strbuf_puts(sb, key);
	strbuf_puts(sb, "'");
	pgop_execute(dbop, strbuf_value(sb), __LINE__);
	p = PQcmdTuples(dbop->res);
	if (!p || !*p || !strcmp(p, "0"))
		(void)pgop_put(dbop, key, dat, fid);
}
/*
 * pgop_first: get first record. 
 * 
 *	i)	dbop	dbop descripter
 *	i)	name	key value or prefix
 *			includes regular expression: ~ read
 *			!=NULL: = read
 *			==NULL: sequential read
 *	i)	preg	compiled regular expression if any.
 *	i)	flags	following pgop_next call take over this.
 *			DBOP_KEY	read key part
 *			DBOP_PREFIX	prefix read
 *					only valied when sequential read
 *	r)		data
 */
char	*
pgop_first(dbop, name, preg, flags)
	DBOP *dbop;
	const char *name;
	regex_t *preg;
	int	flags;
{
	STRBUF *sb = strbuf_open(0);
	char *key;

	dbop->preg = preg;
	dbop->ioflags = flags;
	if (dbop->ioflags & DBOP_PREFIX && !name)
		dbop->ioflags &= ~DBOP_PREFIX;
	pgop_execute(dbop, "BEGIN", __LINE__);
	/*
	 * construct SQL for read tags.
	 */
	strbuf_puts(sb,	"DECLARE c_");
	strbuf_puts(sb,	dbop->tblname);
	strbuf_puts(sb,	" CURSOR FOR SELECT key, dat FROM ");
	strbuf_puts(sb,	dbop->tblname);
	if (name) {
		strbuf_puts(sb,	" WHERE key");
		if (isregex(name) || dbop->ioflags & DBOP_PREFIX) {
			strbuf_puts(sb,	" ~ '");
			if (dbop->ioflags & DBOP_PREFIX)
				strbuf_putc(sb,	'^');
			strbuf_puts(sb,	name);
			strbuf_puts(sb,	"'");
		} else {
			strbuf_puts(sb,	" = '");
			strbuf_puts(sb,	name);
			strbuf_puts(sb,	"'");
		}
	}
	strbuf_puts(sb,	" ORDER BY key");
	pgop_execute(dbop, strbuf_value(sb), __LINE__);
	strbuf_close(sb);
	for (;;) {
		pgop_execute(dbop, strbuf_value(dbop->fetch_stmt), __LINE__);
		if (PQntuples(dbop->res) == 0) {
			STRBUF *sb = strbuf_open(0);
			strbuf_puts(sb,	"CLOSE c_");
			strbuf_puts(sb,	dbop->tblname);
			pgop_execute(dbop, strbuf_value(sb), __LINE__);
			strbuf_close(sb);
			pgop_execute(dbop, "COMMIT", __LINE__);
			return NULL;
		}
		if (preg && regexec(preg, PQgetvalue(dbop->res, 0, 0), 0, 0, 0) != 0)
			continue;
		dbop->lastdat = PQgetvalue(dbop->res, 0, 1);
		if (dbop->lastdat && *(dbop->lastdat) != ' ')
			break;
	}
	return PQgetvalue(dbop->res, 0, (dbop->ioflags & DBOP_KEY) ? 0 : 1);
}
/*
 * pgop_next: get next record. 
 * 
 *	i)	dbop	dbop descripter
 *	r)		data
 *
 * Db_next always skip meta records.
 */
char	*
pgop_next(dbop)
	DBOP *dbop;
{
	char *fetch = strbuf_value(dbop->fetch_stmt);
	for (;;) {
		pgop_execute(dbop, fetch, __LINE__);
		if (PQntuples(dbop->res) == 0) {
			STRBUF *sb = strbuf_open(0);
			strbuf_puts(sb,	"CLOSE c_");
			strbuf_puts(sb,	dbop->tblname);
			pgop_execute(dbop, strbuf_value(sb), __LINE__);
			strbuf_close(sb);
			pgop_execute(dbop, "COMMIT", __LINE__);
			return NULL;
		}
		if (dbop->preg && regexec(dbop->preg, PQgetvalue(dbop->res, 0, 0), 0, 0, 0) != 0)
			continue;
		dbop->lastdat = PQgetvalue(dbop->res, 0, 1);
		if (dbop->lastdat && *(dbop->lastdat) != ' ')
			break;
	}

	return PQgetvalue(dbop->res, 0, (dbop->ioflags & DBOP_KEY) ? 0 : 1);
}
/*
 * pgop_lastdat: get last data
 * 
 *	i)	dbop	dbop descripter
 *	r)		last data
 */
char *
pgop_lastdat(dbop)
	DBOP *dbop;
{
	return dbop->lastdat;
}
/*
 * pgop_close: close db
 * 
 *	i)	dbop	dbop descripter
 */
void
pgop_close(dbop)
	DBOP *dbop;
{
	if (dbop->mode == 1 && strncmp(dbop->tblname, "GPATH", strlen("GPATH"))) {
		char sql[1024];

		snprintf(sql, sizeof(sql),
		"CREATE INDEX %s_i1 ON %s (key)", dbop->tblname, dbop->tblname);
		pgop_execute(dbop, sql, __LINE__);
		snprintf(sql, sizeof(sql),
		"CREATE INDEX %s_i2 ON %s (fid)", dbop->tblname, dbop->tblname);
		pgop_execute(dbop, sql, __LINE__);
	}
	if (dbop->res)
		PQclear(dbop->res);
	(void)free(dbop);
}
#endif /* USE_POSTGRES */
