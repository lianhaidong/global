/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2006,
 *	2009, 2010, 2014
 *	Tama Communications Corporation
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
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>
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
#include <errno.h>

#include "char.h"
#include "checkalloc.h"
#include "dbop.h"
#include "die.h"
#include "env.h"
#include "locatestring.h"
#include "strbuf.h"
#include "strlimcpy.h"
#include "test.h"

#ifdef USE_SQLITE3
int is_sqlite3(const char *);
DBOP *dbop3_open(const char *, int, int, int);
const char *dbop3_get(DBOP *, const char *);
const char *dbop3_getflag(DBOP *);
char *dbop3_quote(char *);
void dbop3_put(DBOP *, const char *, const char *, const char *);
void dbop3_delete(DBOP *, const char *);
void dbop3_update(DBOP *, const char *, const char *);
const char *dbop3_first(DBOP *, const char *, regex_t *, int);
const char *dbop3_next(DBOP *);
void dbop3_close(DBOP *);
#endif

/**
 * Though the prefix of the key of meta record is currently only a @CODE{' '} (blank),
 * this will be enhanced in the future.
 */
#define ismeta(p)	(*((char *)(p)) <= ' ')

/**
 * Stuff for #DBOP_SORTED_WRITE
 */
#define SORT_SEP '\t'

/**
 * Two functions required for sorted writing.
 *
 * @fn static void start_sort_process(DBOP *dbop)
 * (1) start_sort_process: start sort process for sorted writing
 *
 *	@param[in]	dbop	DBOP descriptor
 *
 * @fn static void terminate_sort_process(DBOP *dbop)
 * (2) terminate_sort_process: terminate sort process
 *
 *	@param[in]	dbop	DBOP descriptor
 */
static void start_sort_process(DBOP *);
static void terminate_sort_process(DBOP *);
static char *sortnotfound = "POSIX sort program not found. If available, the program will be speed up.\nPlease see ./configure --help.";
/*
 * 1. DJGPP
 */
#if defined(__DJGPP__)
/*
 * Just ignored. DJGPP version doesn't use sorted writing.
 */
static void
start_sort_process(DBOP *dbop) {
	return;
}
static void
terminate_sort_process(DBOP *dbop) {
	return;
}
/*
 * 2. WIN32
 */
#elif defined(_WIN32) && !defined(__CYGWIN__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
/*
 * sort is included with the binary distribution
 */
static char argv[] = "sort -k 1,1";
static void
start_sort_process(DBOP *dbop) {
	HANDLE opipe[2], ipipe[2];
	SECURITY_ATTRIBUTES sa;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	const char* lc_all;
	char sort[MAX_PATH];
	char* path;
	static int informed;

	if (!strcmp(POSIX_SORT, "no"))
		return;
	if (informed)
		return;
	/*
	 * force using sort in the same directory as the program, to avoid
	 * using the Windows one
	 */
	path = strrchr(_pgmptr, '\\');
	sprintf(sort, "%.*s\\sort.exe", path - _pgmptr, _pgmptr);
	if (!test("fx", sort)) {
		warning(sortnotfound);
		informed = 1;
		return;
	}

	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	if (!CreatePipe(&opipe[0], &opipe[1], &sa, 0) ||
	    !CreatePipe(&ipipe[0], &ipipe[1], &sa, 0))
		die("CreatePipe failed.");
	SetHandleInformation(opipe[1], HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(ipipe[0], HANDLE_FLAG_INHERIT, 0);
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.hStdInput = opipe[0];
	si.hStdOutput = ipipe[1];
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	si.dwFlags = STARTF_USESTDHANDLES;
	lc_all = getenv("LC_ALL");
	if (lc_all == NULL)
		lc_all = "";
	set_env("LC_ALL", "C");
	CreateProcess(sort, argv, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	set_env("LC_ALL", lc_all);
	CloseHandle(opipe[0]);
	CloseHandle(ipipe[1]);
	CloseHandle(pi.hThread);
	dbop->pid = pi.hProcess;
	dbop->sortout = fdopen(_open_osfhandle((long)opipe[1], _O_WRONLY), "w");
	dbop->sortin = fdopen(_open_osfhandle((long)ipipe[0], _O_RDONLY), "r");
	if (dbop->sortout == NULL || dbop->sortin == NULL)
		die("fdopen failed.");
}
static void
terminate_sort_process(DBOP *dbop) {
	WaitForSingleObject(dbop->pid, INFINITE);
	CloseHandle(dbop->pid);
}
/*
 * 3. UNIX and CYGWIN
 */
#else
#include <sys/wait.h>
/*
 * Though it doesn't understand why, GNU sort with no option is faster
 * than 'sort -k 1,1'. But we should use '-k 1,1' here not to rely on
 * a specific command.
 */
static char *argv[] = {
	POSIX_SORT,
	"-k",
	"1,1",
	NULL
};
static void
start_sort_process(DBOP *dbop) {
	int opipe[2], ipipe[2];

	if (!strcmp(POSIX_SORT, "no"))
		return;
	if (!test("fx", POSIX_SORT)) {
		static int informed;

		if (!informed) {
			warning(sortnotfound);
			informed = 1;
		}
		return;
	}
	/*
	 * Setup pipe for two way communication
	 *
	 *	Parent(gtags)				Child(sort)
	 *	---------------------------------------------------
	 *	(dbop->sortout) opipe[1] =====> opipe[0] (stdin)
	 *	(dbop->sortin)  ipipe[0] <===== ipipe[1] (stdout)
	 */
	if (pipe(opipe) < 0 || pipe(ipipe) < 0)
		die("pipe(2) failed.");
	dbop->pid = fork();
	if (dbop->pid == 0) {
		/* child process */
		close(opipe[1]);
		close(ipipe[0]);
		if (dup2(opipe[0], 0) < 0 || dup2(ipipe[1], 1) < 0)
			die("dup2(2) failed.");
		close(opipe[0]);
		close(ipipe[1]);
		/*
		 * Use C locale in order to avoid the degradation of performance 	 
		 * by internationalized sort command. 	 
		 */
		set_env("LC_ALL", "C");
		execvp(POSIX_SORT, argv);
	} else if (dbop->pid < 0)
		die("fork(2) failed.");
	/* parent process */
	close(opipe[0]);
	close(ipipe[1]);
	fcntl(ipipe[0], F_SETFD, FD_CLOEXEC);
	fcntl(opipe[1], F_SETFD, FD_CLOEXEC);
	dbop->sortout = fdopen(opipe[1], "w");
	dbop->sortin = fdopen(ipipe[0], "r");
	if (dbop->sortout == NULL || dbop->sortin == NULL)
		die("fdopen(3) failed.");
}
static void
terminate_sort_process(DBOP *dbop) {
	int ret, status;

	while ((ret = waitpid(dbop->pid, &status, 0)) < 0 && errno == EINTR)
		;
	if (ret < 0)
		die("waitpid(2) failed.");
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		die("terminated abnormally. '%s'", POSIX_SORT);
}
#endif

#ifdef USE_SQLITE3
static const char *sqlite_header = "SQLite format 3";
int
is_sqlite3(const char *path) {
	char buf[32];
	int sqlite3 = 0;
	int fd = open(path, 0);

	if (fd >= 0) {
		if (read(fd, buf, sizeof(buf)) == sizeof(buf)) {
			if (!strncmp(sqlite_header, buf, strlen(sqlite_header)))
				sqlite3 = 1;
		}
		close(fd);
	}
	return sqlite3;
}
#endif
/**
 * dbop_open: open db database.
 *
 *	@param[in]	path	database name
 *	@param[in]	mode	0: read only, 1: create, 2: modify
 *	@param[in]	perm	file permission
 *	@param[in]	flags
 *			#DBOP_DUP: allow duplicate records. <br>
 *			#DBOP_SORTED_WRITE: use sorted writing. This requires @NAME{POSIX} sort.
 *	@return		descripter for @NAME{dbop_xxx()}
 *
 * Sorted wirting is fast because all writing is done by not insertion but addition.
 */
DBOP *
dbop_open(const char *path, int mode, int perm, int flags)
{
	DB *db;
	int rw = 0;
	DBOP *dbop;
	BTREEINFO info;

#ifdef USE_SQLITE3
	if (mode != 1 && is_sqlite3(path))
		flags |= DBOP_SQLITE3;
	if (flags & DBOP_SQLITE3) {
		dbop = dbop3_open(path, mode, perm, flags);
		goto finish;
	}
#endif
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
	 * Decide cache size. The default value is 5MB.
	 * See libutil/gparam.h for the details.
	 */
	info.cachesize = GTAGSCACHE;
	if (getenv("GTAGSCACHE") != NULL)
		info.cachesize = atoi(getenv("GTAGSCACHE"));
	if (info.cachesize < GTAGSMINCACHE)
		info.cachesize = GTAGSMINCACHE;

	/*
	 * if unlink do job normally, those who already open tag file can use
	 * it until closing.
	 */
	if (path != NULL && mode == 1 && test("f", path))
		(void)unlink(path);
	db = dbopen(path, rw, 0600, DB_BTREE, &info);
	if (!db)
		return NULL;
	dbop = (DBOP *)check_calloc(sizeof(DBOP), 1);
	if (path == NULL)
		dbop->dbname[0] = '\0';
	else
		strlimcpy(dbop->dbname, path, sizeof(dbop->dbname));
	dbop->db	= db;
	dbop->openflags	= flags;
	dbop->perm	= (mode == 1) ? perm : 0;
	dbop->lastdat	= NULL;
	dbop->lastsize	= 0;
	dbop->sortout	= NULL;
	dbop->sortin	= NULL;
	/*
	 * Setup sorted writing.
	 */
	if (mode != 0 && dbop->openflags & DBOP_SORTED_WRITE)
		start_sort_process(dbop);
finish:
	return dbop;
}
/**
 * dbop_get: get data by a key.
 *
 *	@param[in]	dbop	descripter
 *	@param[in]	name	name
 *	@return		pointer to data
 */
const char *
dbop_get(DBOP *dbop, const char *name)
{
	DB *db = dbop->db;
	DBT key, dat;
	int status;

#ifdef USE_SQLITE3
	if (dbop->openflags & DBOP_SQLITE3)
		return dbop3_get(dbop, name);
#endif
	key.data = (char *)name;
	key.size = strlen(name)+1;

	status = (*db->get)(db, &key, &dat, 0);
	dbop->lastdat = (char *)dat.data;
	dbop->lastsize = dat.size;
	switch (status) {
	case RET_SUCCESS:
		break;
	case RET_ERROR:
		die("dbop_get failed.");
	case RET_SPECIAL:
		return (NULL);
	}
	return (dat.data);
}
/**
 * dbop_put: put data by a key.
 *
 *	@param[in]	dbop	descripter
 *	@param[in]	name	key
 *	@param[in]	data	data
 */
void
dbop_put(DBOP *dbop, const char *name, const char *data)
{
	DB *db = dbop->db;
	DBT key, dat;
	int status;
	int len;

#ifdef USE_SQLITE3
	if (dbop->openflags & DBOP_SQLITE3) {
		dbop3_put(dbop, name, data, NULL);
		return;
	}
#endif
	if (!(len = strlen(name)))
		die("primary key size == 0.");
	if (len > MAXKEYLEN)
		die("primary key too long.");
	/* sorted writing */
	if (dbop->sortout != NULL) {
		fputs(name, dbop->sortout);
		putc(SORT_SEP, dbop->sortout);
		fputs(data, dbop->sortout);
		putc('\n', dbop->sortout);
		return;
	}
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
		die(dbop->put_errmsg ? dbop->put_errmsg : "dbop_put failed.");
	}
}
/**
 * dbop_put_tag: put a tag
 *
 *	@param[in]	dbop	descripter
 *	@param[in]	name	key
 *	@param[in]	data	data
 */
void
dbop_put_tag(DBOP *dbop, const char *name, const char *data)
{
#ifdef USE_SQLITE3
	if (dbop->openflags & DBOP_SQLITE3) {
		int len;
		char fid[MAXFIDLEN], *q = fid;
		const char *p = data;

		/* extract fid */
		while (*p && isdigit(*p))
			*q++ = *p++;
		*q = '\0';
		/* trim line */
		len = strlen(data);
		if (data[len-1] == '\n')
			len--;
		if (data[len-1] == '\r')
			len--;
		if (data[len] == '\r' || data[len] == '\n') {
			STATIC_STRBUF(sb);

			strbuf_clear(sb);
			strbuf_nputs(sb, data, len);
			data = strbuf_value(sb);
		}
		dbop3_put(dbop, name, data, fid);
		return;
	}
#endif
	dbop_put(dbop, name, data);
	return;
}
/**
 * dbop_put_path: put data and flag by a key.
 *
 *	@param[in]	dbop	descripter
 *	@param[in]	name	key
 *	@param[in]	data	data
 *	@param[in]	flag	flag
 *
 * @note This function doesn't support sorted writing.
 */
void
dbop_put_path(DBOP *dbop, const char *name, const char *data, const char *flag)
{
	STATIC_STRBUF(sb);
	DB *db = dbop->db;
	DBT key, dat;
	int status;
	int len;

#ifdef USE_SQLITE3
	if (dbop->openflags & DBOP_SQLITE3) {
		dbop3_put(dbop, name, data, flag);
		return;
	}
#endif
	if (!(len = strlen(name)))
		die("primary key size == 0.");
	if (len > MAXKEYLEN)
		die("primary key too long.");
	strbuf_clear(sb);
	strbuf_puts0(sb, data);
	if (flag)
		strbuf_puts0(sb, flag);
	key.data = (char *)name;
	key.size = strlen(name)+1;
	dat.data = strbuf_value(sb);
	dat.size = strbuf_getlen(sb);

	status = (*db->put)(db, &key, &dat, 0);
	switch (status) {
	case RET_SUCCESS:
		break;
	case RET_ERROR:
	case RET_SPECIAL:
		die(dbop->put_errmsg ? dbop->put_errmsg : "dbop_put_path failed.");
	}
}
/**
 * dbop_delete: delete record by path name.
 *
 *	@param[in]	dbop	descripter
 *	@param[in]	path	path name
 */
void
dbop_delete(DBOP *dbop, const char *path)
{
	DB *db = dbop->db;
	DBT key;
	int status;

#ifdef USE_SQLITE3
	if (dbop->openflags & DBOP_SQLITE3) {
		dbop3_delete(dbop, path);
		return;
	}
#endif
	if (path) {
		key.data = (char *)path;
		key.size = strlen(path)+1;
		status = (*db->del)(db, &key, 0);
	} else
		status = (*db->del)(db, &key, R_CURSOR);
	if (status == RET_ERROR)
		die("dbop_delete failed.");
}
/**
 * dbop_update: update record.
 *
 *	@param[in]	dbop	descripter
 *	@param[in]	key	key
 *	@param[in]	dat	data
 */
void
dbop_update(DBOP *dbop, const char *key, const char *dat)
{
#ifdef USE_SQLITE3
	if (dbop->openflags & DBOP_SQLITE3)
		return dbop3_update(dbop, key, dat);
#endif
	dbop_put(dbop, key, dat);
}
/**
 * dbop_first: get first record. 
 * 
 *	@param[in]	dbop	dbop descripter
 *	@param[in]	name	key value or prefix <br>
 *			!=NULL: indexed read by key <br>
 *			==NULL: sequential read
 *	@param[in]	preg	compiled regular expression if any.
 *	@param[in]	flags	following dbop_next call take over this. <br>
 *			#DBOP_KEY:	read key part <br>
 *			#DBOP_PREFIX:	prefix read; only valied when sequential read
 *	@return		data
 */
const char *
dbop_first(DBOP *dbop, const char *name, regex_t *preg, int flags)
{
	DB *db = dbop->db;
	DBT key, dat;
	int status;

	dbop->preg = preg;
	dbop->ioflags = flags;
	if (flags & DBOP_PREFIX && !name)
		flags &= ~DBOP_PREFIX;
#ifdef USE_SQLITE3
	if (dbop->openflags & DBOP_SQLITE3)
		return dbop3_first(dbop, name, preg, flags);
#endif
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
			dbop->readcount++;
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
			dbop->readcount++;
			/* skip meta records */
			if (ismeta(key.data) && !(dbop->openflags & DBOP_RAW))
				continue;
			if (preg && regexec(preg, (char *)key.data, 0, 0, 0) != 0)
				continue;
			break;
		}
	}
	dbop->lastdat = (char *)dat.data;
	dbop->lastsize = dat.size;
	dbop->lastkey = (char *)key.data;
	dbop->lastkeysize = key.size;
	switch (status) {
	case RET_SUCCESS:
		break;
	case RET_ERROR:
		die("dbop_first failed.");
	case RET_SPECIAL:
		return (NULL);
	}
	if (flags & DBOP_KEY) {
		strlimcpy(dbop->prev, (char *)key.data, sizeof(dbop->prev));
		return (char *)key.data;
	}
	return ((char *)dat.data);
}
/**
 * dbop_next: get next record. 
 * 
 *	@param[in]	dbop	dbop descripter
 *	@return		data
 *
 * @note dbop_next() always skip meta records.
 */
const char *
dbop_next(DBOP *dbop)
{
	DB *db = dbop->db;
	int flags = dbop->ioflags;
	DBT key, dat;
	int status;

	if (dbop->unread) {
		dbop->unread = 0;
		return dbop->lastdat;
	}
#ifdef USE_SQLITE3
	if (dbop->openflags & DBOP_SQLITE3)
		return dbop3_next(dbop);
#endif
	while ((status = (*db->seq)(db, &key, &dat, R_NEXT)) == RET_SUCCESS) {
		dbop->readcount++;
		assert(dat.data != NULL);
		/* skip meta records */
		if (!(dbop->openflags & DBOP_RAW)) {
			if (flags & DBOP_KEY && ismeta(key.data))
				continue;
			else if (ismeta(dat.data))
				continue;
		}
		if (flags & DBOP_KEY) {
			if (!strcmp(dbop->prev, (char *)key.data))
				continue;
			if (strlen((char *)key.data) > MAXKEYLEN)
				die("primary key too long.");
			strlimcpy(dbop->prev, (char *)key.data, sizeof(dbop->prev));
		}
		dbop->lastdat	= (char *)dat.data;
		dbop->lastsize	= dat.size;
		dbop->lastkey = (char *)key.data;
		dbop->lastkeysize = key.size;
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
/**
 * dbop_unread: unread record to read again.
 * 
 *	@param[in]	dbop	dbop descripter
 *
 * @note dbop_next() will read this record later.
 */
void
dbop_unread(DBOP *dbop)
{
	dbop->unread = 1;
}
/**
 * dbop_lastdat: get last data
 * 
 *	@param[in]	dbop	dbop descripter
 *	@param[out]	size
 *	@return		last data
 */
const char *
dbop_lastdat(DBOP *dbop, int *size)
{
	if (size)
		*size = dbop->lastsize;
	return dbop->lastdat;
}
/**
 * get_flag: get flag value
 */
const char *
dbop_getflag(DBOP *dbop)
{
	int size;
	const char *dat = dbop_lastdat(dbop, &size);
	const char *flag = "";

#ifdef USE_SQLITE3
	if (dbop->openflags & DBOP_SQLITE3)
		return dbop3_getflag(dbop);
#endif
	/*
	 * Dat format is like follows.
	 * dat 'xxxxxxx\0ffff\0'
	 *      (data)   (flag)
	 */
	if (dat) {
		int i = strlen(dat) + 1;
		if (size > i)
			flag = dat + i;
	}
	return flag;
}
/**
 * dbop_getoption: get option
 */
const char *
dbop_getoption(DBOP *dbop, const char *key)
{
	static char buf[1024];
	const char *p;

	if ((p = dbop_get(dbop, key)) == NULL)
		return NULL;
	if (dbop->lastsize < strlen(key))
		die("invalid format (dbop_getoption).");
	for (p += strlen(key); *p && isspace((unsigned char)*p); p++)
		;
	strlimcpy(buf, p, sizeof(buf));
	return buf;
}
/**
 * dbop_putoption: put option
 */
void
dbop_putoption(DBOP *dbop, const char *key, const char *string)
{
	char buf[1024];

	if (string)
		snprintf(buf, sizeof(buf), "%s %s", key, string);
	else
		snprintf(buf, sizeof(buf), "%s", key);
	dbop_put(dbop, key, buf);
}
/**
 * dbop_getversion: get format version
 */
int
dbop_getversion(DBOP *dbop)
{
	int format_version = 1;			/* default format version */
	const char *p;

	if ((p = dbop_getoption(dbop, VERSIONKEY)) != NULL)
		format_version = atoi(p);
	return format_version;
}
/**
 * dbop_putversion: put format version
 */
void
dbop_putversion(DBOP *dbop, int version)
{
	char number[32];

	snprintf(number, sizeof(number), "%d", version);
	dbop_putoption(dbop, VERSIONKEY, number);
}
/**
 * dbop_close: close db
 * 
 *	@param[in]	dbop	dbop descripter
 */
void
dbop_close(DBOP *dbop)
{
	DB *db = dbop->db;

	/*
	 * Load sorted tag records and write them to the tag file.
	 */
	if (dbop->sortout != NULL) {
		STRBUF *sb = strbuf_open(256);
		char *p;

		/*
		 * End of the former stage of sorted writing.
		 * fclose() and sortout = NULL is important.
		 *
		 * fclose(): enables reading from sortin descriptor.
		 * sortout = NULL: makes the following dbop_put write to the tag file directly.
		 */
		fclose(dbop->sortout);
		dbop->sortout = NULL;
		/*
		 * The last stage of sorted writing.
		 */
		while (strbuf_fgets(sb, dbop->sortin, STRBUF_NOCRLF)) {
			for (p = strbuf_value(sb); *p && *p != SORT_SEP; p++)
				;
			if (!*p)
				die("unexpected end of record.");
			*p++ = '\0';
			dbop_put(dbop, strbuf_value(sb), p);
		}
		fclose(dbop->sortin);
		strbuf_close(sb);
		terminate_sort_process(dbop);
	}
#ifdef USE_SQLITE3
	if (dbop->openflags & DBOP_SQLITE3) {
		dbop3_close(dbop);
		return;
	}
#endif
#ifdef USE_DB185_COMPAT
	(void)db->close(db);
#else
	/*
	 * If dbname = NULL, omit writing to the disk in __bt_close().
	 */
	(void)db->close(db, dbop->dbname[0] == '\0' ? 1 : 0);
#endif
	if (dbop->dbname[0] != '\0') {
		if (dbop->perm && chmod(dbop->dbname, dbop->perm) < 0)
			die("chmod(2) failed.");
	}
	(void)free(dbop);
}
#ifdef USE_SQLITE3
DBOP *
dbop3_open(const char *path, int mode, int perm, int flags) {
	int rc, rw = 0;
	char *errmsg = 0;
	DBOP *dbop;
	sqlite3 *db3;
	const char *tblname;
	int cache_size = 0;
	STRBUF *sql = strbuf_open_tempbuf();
	char buf[1024];

	/*
	 * When the path is NULL string and private, temporary file is used.
	 * The database will be removed when the session is closed.
	 */
	if (path == NULL) {
		path = "";
		tblname = "temp";
	} else {
		/*
		 * In case of creation.
		 */
		if (mode == 1)
			(void)truncate(path, 0);
		tblname = "db";
	}
	/*
	 * setup arguments.
	 */
	switch (mode) {
	case 0:
		rw = SQLITE_OPEN_READONLY;
		break;
	case 1:
		rw = SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE;
		break;
	case 2:
		rw = SQLITE_OPEN_READWRITE;
		break;
	default:
		assert(0);
	}
	/*
	 * When the forth argument is NULL, sqlite3_vfs is used.
	 */
	rc = sqlite3_open_v2(path, &db3, rw, NULL);
	if (rc != SQLITE_OK)
		die("sqlite3_open_v2 failed. (rc = %d)", rc);
	dbop = (DBOP *)check_calloc(sizeof(DBOP), 1);
	strlimcpy(dbop->dbname, path, sizeof(dbop->dbname));
	dbop->sb        = strbuf_open(0);
	dbop->db3       = db3;
	dbop->openflags	= flags;
	dbop->perm	= (mode == 1) ? perm : 0;
	dbop->mode      = mode;
	dbop->lastdat	= NULL;
	dbop->lastflag	= NULL;
	dbop->lastsize	= 0;
	dbop->sortout	= NULL;
	dbop->sortin	= NULL;
	dbop->stmt      = NULL;
	dbop->tblname   = check_strdup(tblname);
	/*
	 * create table (GTAGS, GRTAGS, GSYMS, GPATH).
	 */
	if (mode == 1) {
		/* drop table */
		strbuf_clear(sql);
		strbuf_puts(sql, "drop table ");
		strbuf_puts(sql, dbop->tblname);
		rc = sqlite3_exec(dbop->db3, strbuf_value(sql), NULL, NULL, &errmsg);
        	if (rc != SQLITE_OK) {
			/* ignore */
		}
		/* create table */
		strbuf_clear(sql);
		strbuf_puts(sql, "create table ");
		strbuf_puts(sql, dbop->tblname);
		strbuf_puts(sql, " (key text, dat text, extra text");
		if (!(flags & DBOP_DUP))
			strbuf_puts(sql, ", primary key(key)");
		strbuf_putc(sql, ')');
		rc = sqlite3_exec(dbop->db3, strbuf_value(sql), NULL, NULL, &errmsg);
        	if (rc != SQLITE_OK)
			die("create table error: %s", errmsg);
	}
	/*
	rc = sqlite3_exec(dbop->db3, "pragma synchronous=off", NULL, NULL, &errmsg);
       	if (rc != SQLITE_OK)
		die("pragma synchronous=off error: %s", errmsg);
	*/
	/*
         * Decide cache size.
         * See libutil/gparam.h for the details.
         */
	cache_size = GTAGSCACHE;
	if (getenv("GTAGSCACHE") != NULL)
		cache_size = atoi(getenv("GTAGSCACHE"));
	if (cache_size < GTAGSMINCACHE)
		cache_size = GTAGSMINCACHE;
	snprintf(buf, sizeof(buf), "pragma cache_size=%d", cache_size);
	rc = sqlite3_exec(dbop->db3, buf,  NULL, NULL, &errmsg);
       	if (rc != SQLITE_OK)
		die("pragma cache_size error: %s", errmsg);
	/*
	 * Maximum file size is DBOP_PAGESIZE * 2147483646.
	 * if DBOP_PAGESIZE == 8192 then maximum file size is 17592186028032 (17T).
	 */
	snprintf(buf, sizeof(buf), "pragma page_size=%d", DBOP_PAGESIZE);
	rc = sqlite3_exec(dbop->db3, buf,  NULL, NULL, &errmsg);
       	if (rc != SQLITE_OK)
		die("pragma page_size error: %s", errmsg);
	sqlite3_exec(dbop->db3, "pragma journal_mode=memory", NULL, NULL, &errmsg);
       	if (rc != SQLITE_OK)
		die("pragma journal_mode=memory error: %s", errmsg);
	sqlite3_exec(dbop->db3, "pragma pragma synchronous=off", NULL, NULL, &errmsg);
       	if (rc != SQLITE_OK)
		die("pragma synchronous=off error: %s", errmsg);
	rc = sqlite3_exec(dbop->db3, "begin transaction", NULL, NULL, &errmsg);
       	if (rc != SQLITE_OK)
		die("pragma begin transaction error: %s", errmsg);
	strbuf_release_tempbuf(sql);
	return dbop;
}
static int
single_select_callback(void *v, int argc, char **argv, char **colname) {
	STATIC_STRBUF(sb);
	DBOP *dbop = (DBOP *)v;

	if (argc > 0) {
		strbuf_clear(sb);
		strbuf_puts(sb, argv[0]);
		dbop->lastsize = strbuf_getlen(sb);
		if (argv[1]) {
			strbuf_putc(sb, '\0');
			strbuf_puts(sb, argv[1]);
		}
		dbop->lastdat = strbuf_value(sb);
		dbop->lastflag = argv[1] ? dbop->lastdat + dbop->lastsize + 1 : NULL;
	} else {
		dbop->lastdat = NULL;
		dbop->lastflag = NULL;
		dbop->lastsize = 0;
	}
	return SQLITE_OK;
}
const char *
dbop3_get(DBOP *dbop, const char *name) {
	int rc;
	char *errmsg = 0;
	STRBUF *sql = strbuf_open_tempbuf();

	strbuf_sprintf(sql, "select dat, extra from %s where key = '%s' limit 1",
			dbop->tblname, name); 
	dbop->lastdat = NULL;
	dbop->lastsize = 0;
	dbop->lastflag = NULL;
	rc = sqlite3_exec(dbop->db3, strbuf_value(sql), single_select_callback, dbop, &errmsg);
       	if (rc != SQLITE_OK) {
		sqlite3_close(dbop->db3);
		die("dbop3_get failed: %s", errmsg);
	}
	strbuf_release_tempbuf(sql);
	return dbop->lastdat;
}
const char *
dbop3_getflag(DBOP *dbop)
{
	return dbop->lastflag ? dbop->lastflag : "";
}
char *
dbop3_quote(char *string) {
	STATIC_STRBUF(sb);
	char *p;

	strbuf_clear(sb);
	for (p = string; *p; p++) {
		if (*p == '\'')
			strbuf_putc(sb, '\'');
		strbuf_putc(sb, *p);
	}
	return strbuf_value(sb);
}
void
dbop3_put(DBOP *dbop, const char *p1, const char *p2, const char *p3) {
	int rc, len;
	char *errmsg = 0;
	STRBUF *sql = strbuf_open_tempbuf();

	if (!(len = strlen(p1)))
		die("primary key size == 0.");
	if (len > MAXKEYLEN)
		die("primary key too long.");
	if (dbop->stmt_put3 == NULL) {
		strbuf_sprintf(sql, "insert into %s values (?, ?, ?)", dbop->tblname);
		rc = sqlite3_prepare_v2(dbop->db3, strbuf_value(sql), -1, &dbop->stmt_put3, NULL);
		if (rc != SQLITE_OK) {
			die("dbop3_put prepare failed. (rc = %d, sql = %s)", rc, strbuf_value(sql));
		}
	}
	rc = sqlite3_bind_text(dbop->stmt_put3, 1, p1, -1, SQLITE_STATIC);
       	if (rc != SQLITE_OK) {
		die("dbop3_put 1 failed. (rc = %d)", rc);
	}
	rc = sqlite3_bind_text(dbop->stmt_put3, 2, p2, -1, SQLITE_STATIC);
       	if (rc != SQLITE_OK) {
		die("dbop3_put 2 failed. (rc = %d)", rc);
	}
	rc = sqlite3_bind_text(dbop->stmt_put3, 3, p3, -1, SQLITE_STATIC);
       	if (rc != SQLITE_OK) {
		die("dbop3_put 3 failed. (rc = %d)", rc);
	}
	rc = sqlite3_step(dbop->stmt_put3);
       	if (rc != SQLITE_DONE) {
		die("dbop3_put failed. (rc = %d)", rc);
	}
	rc = sqlite3_reset(dbop->stmt_put3);
       	if (rc != SQLITE_OK) {
		die("dbop3_put reset failed. (rc = %d)", rc);
	}
	if (dbop->writecount++ > DBOP_COMMIT_THRESHOLD) {
		dbop->writecount = 0;
		rc = sqlite3_exec(dbop->db3, "end transaction", NULL, NULL, &errmsg);
		if (rc != SQLITE_OK)
			die("pragma error: %s", errmsg);
		rc = sqlite3_exec(dbop->db3, "begin transaction", NULL, NULL, &errmsg);
		if (rc != SQLITE_OK)
			die("pragma error: %s", errmsg);
	}
	strbuf_release_tempbuf(sql);
}
void
dbop3_delete(DBOP *dbop, const char *path) {
	int rc;
	char *errmsg = 0;
	STRBUF *sql = strbuf_open_tempbuf();

	strbuf_puts(sql, "delete from ");
	strbuf_puts(sql, dbop->tblname);
	strbuf_puts(sql, " where");
	if (path) {
		if (*path == '(') {
			strbuf_puts(sql, " extra in ");
			strbuf_puts(sql, path); 
		} else {
			strbuf_puts(sql, " key = '");
			strbuf_puts(sql, path);
			strbuf_puts(sql, "'");
		}
	} else {
		strbuf_puts(sql, " rowid = ");
		strbuf_putn64(sql, dbop->lastrowid);
	}
	rc = sqlite3_exec(dbop->db3, strbuf_value(sql), NULL, NULL, &errmsg);
       	if (rc != SQLITE_OK) {
		sqlite3_close(dbop->db3);
		die("dbop3_delete failed: %s", errmsg);
	}
	strbuf_release_tempbuf(sql);
}
void
dbop3_update(DBOP *dbop, const char *key, const char *dat) {
	int rc;
	char *errmsg = 0;
	STRBUF *sql = strbuf_open_tempbuf();

	strbuf_sprintf(sql, "update %s set dat = '%s' where key = '%s'",
					dbop->tblname, dbop3_quote((char *)dat), key);
	rc = sqlite3_exec(dbop->db3, strbuf_value(sql), NULL, NULL, &errmsg);
       	if (rc != SQLITE_OK) {
		sqlite3_close(dbop->db3);
		die("dbop3_update failed: %s", errmsg);
	}
	if (sqlite3_changes(dbop->db3) == 0) {
		strbuf_clear(sql);
		strbuf_sprintf(sql, "insert into %s values ('%s', '%s', NULL)",
					dbop->tblname, key, dbop3_quote((char *)dat));
		rc = sqlite3_exec(dbop->db3, strbuf_value(sql), NULL, NULL, &errmsg);
		if (rc != SQLITE_OK) {
			sqlite3_close(dbop->db3);
			die("dbop3_updated failed: %s", errmsg);
		}
	}
	strbuf_release_tempbuf(sql);
}
const char *
dbop3_first(DBOP *dbop, const char *name, regex_t *preg, int flags) {
	int rc;
	char *key;
	STRBUF *sql = strbuf_open_tempbuf();

	strbuf_puts(sql, "select rowid, * from ");
	strbuf_puts(sql, dbop->tblname);
	if (name) {
		strbuf_puts(sql, " where key ");
		if (dbop->ioflags & DBOP_PREFIX) {
			/*
			 * In sqlite3, 'like' ignores case. 'glob' does not ignore case.
			 */
			strbuf_puts(sql, "glob '");
			strbuf_puts(sql, name);
			strbuf_puts(sql, "*'");
		} else {
			strbuf_puts(sql, "= '");
			strbuf_puts(sql, name);
			strbuf_puts(sql, "'");
		}
		strlimcpy(dbop->key, name, sizeof(dbop->key));
		dbop->keylen = strlen(name);
	}
	strbuf_puts(sql, " order by key");
	if (dbop->stmt) {
		rc = sqlite3_finalize(dbop->stmt);
		if (rc != SQLITE_OK)
			die("dbop3_finalize failed. (rc = %d)", rc);
		dbop->stmt = NULL;
	}
	rc = sqlite3_prepare_v2(dbop->db3, strbuf_value(sql), -1, &dbop->stmt, NULL);
	if (rc != SQLITE_OK)
		die("dbop3_first: sqlite3_prepare_v2 failed. (rc = %d)", rc);
	/*
	 *	0: rowid
	 *	1: key
	 *	2: dat
	 *	3: flags
	 */
	for (;;) {
		rc = sqlite3_step(dbop->stmt);
		if (rc == SQLITE_ROW) {
			dbop->readcount++;
			dbop->lastrowid = sqlite3_column_int64(dbop->stmt, 0);
			key = (char *)sqlite3_column_text(dbop->stmt, 1);
			if (name) {
				if (dbop->ioflags & DBOP_PREFIX) {
					if (strncmp(key, dbop->key, dbop->keylen))
						goto finish;
				} else {
					if (strcmp(key, dbop->key)) 
						goto finish;
				}
				if (dbop->preg && regexec(dbop->preg, key, 0, 0, 0) != 0)
					continue;
			} else {
				/* skip meta records */
				if (ismeta(key) && !(dbop->openflags & DBOP_RAW))
					continue;
				if (dbop->preg && regexec(dbop->preg, key, 0, 0, 0) != 0)
					continue;
			}
			break;
		} else {
			/*
			 * Sqlite3 returns SQLITE_MISUSE if it is called after SQLITE_DONE.
			 */
			goto finish;
		}
	}
	strbuf_clear(dbop->sb);
	strbuf_puts0(dbop->sb, (char *)sqlite3_column_text(dbop->stmt, 2));
	dbop->lastsize = strbuf_getlen(dbop->sb) - 1;
	dbop->lastflag = (char *)sqlite3_column_text(dbop->stmt, 3);
	if (dbop->lastflag)
		strbuf_puts(dbop->sb, dbop->lastflag);
	dbop->lastdat = strbuf_value(dbop->sb);
	if (dbop->lastflag)
		dbop->lastflag = dbop->lastdat + dbop->lastsize + 1;
	dbop->lastkey = key;
	dbop->lastkeysize = strlen(dbop->lastkey);
	strbuf_release_tempbuf(sql);
	if (flags & DBOP_KEY) {
		strlimcpy(dbop->prev, key, sizeof(dbop->prev));
		return key;
	}
	return dbop->lastdat;
finish:
	strbuf_release_tempbuf(sql);
	dbop->lastdat = NULL;
	dbop->lastsize = 0;
	dbop->lastflag = NULL;
	return dbop->lastdat;
}
const char *
dbop3_next(DBOP *dbop) {
	int rc;
	char *key, *dat;

	/*
	 *	0: rowid
	 *	1: key
	 *	2: dat
	 *	3: flags
	 */
	for (;;) {
		rc = sqlite3_step(dbop->stmt);
		if (rc == SQLITE_ROW) {
			dbop->readcount++;
			dbop->lastrowid = sqlite3_column_int64(dbop->stmt, 0);
			key = (char *)sqlite3_column_text(dbop->stmt, 1);
			dat = (char *)sqlite3_column_text(dbop->stmt, 2);
			/* skip meta records */
			if (!(dbop->openflags & DBOP_RAW)) {
				if (dbop->ioflags & DBOP_KEY && ismeta(key))
					continue;
				else if (ismeta(dat))
					continue;
			}
			if (dbop->ioflags & DBOP_KEY) {
				if (!strcmp(dbop->prev, key))
					continue;
				if (strlen(key) > MAXKEYLEN)
					die("primary key too long.");
				strlimcpy(dbop->prev, key, sizeof(dbop->prev));
			}
			if (dbop->ioflags & DBOP_PREFIX) {
				if (strncmp(key, dbop->key, dbop->keylen))
					goto finish;
			} else if (dbop->keylen) {
				if (strcmp(key, dbop->key)) 
					goto finish;
			}
			if (dbop->preg && regexec(dbop->preg, key, 0, 0, 0) != 0)
				continue;
			break;
		} else {
			/*
			 * Sqlite3 returns SQLITE_MISUSE if it is called after SQLITE_DONE.
			 */
			goto finish;
			die("dbop3_next: sqlite3_step failed. (rc = %d) %s", rc, sqlite3_errmsg(dbop->db3));
		}
	}
	strbuf_clear(dbop->sb);
	strbuf_puts0(dbop->sb, (char *)sqlite3_column_text(dbop->stmt, 2));
	dbop->lastsize = strbuf_getlen(dbop->sb) - 1;
	dbop->lastflag = (char *)sqlite3_column_text(dbop->stmt, 3);
	if (dbop->lastflag)
		strbuf_puts(dbop->sb, dbop->lastflag);
	dbop->lastdat = strbuf_value(dbop->sb);
	if (dbop->lastflag)
		dbop->lastflag = dbop->lastdat + dbop->lastsize + 1;
	dbop->lastkey = key;
	dbop->lastkeysize = strlen(dbop->lastkey);
	if (dbop->ioflags & DBOP_KEY) {
		strlimcpy(dbop->prev, key, sizeof(dbop->prev));
		return key;
	}
	return dbop->lastdat;
finish:
	dbop->lastdat = NULL;
	dbop->lastsize = 0;
	return dbop->lastdat;
}
void
dbop3_close(DBOP *dbop) {
	int rc;
	char *errmsg = 0;

	rc = sqlite3_exec(dbop->db3, "end transaction", NULL, NULL, &errmsg);
       	if (rc != SQLITE_OK)
		die("pragma error: %s", errmsg);
	/*
	 * create index
	 */
	if (dbop->mode == 1 && dbop->openflags & DBOP_DUP) {
		STATIC_STRBUF(sql);

		strbuf_clear(sql);
		strbuf_puts(sql, "create index key_i on ");
		strbuf_puts(sql, dbop->tblname);
		strbuf_puts(sql, "(key)");
		rc = sqlite3_exec(dbop->db3, strbuf_value(sql), NULL, NULL, &errmsg);
		if (rc != SQLITE_OK)
			die("create table error: %s", errmsg);
		strbuf_clear(sql);
		strbuf_puts(sql, "create index fid_i on ");
		strbuf_puts(sql, dbop->tblname);
		strbuf_puts(sql, "(extra)");
		rc = sqlite3_exec(dbop->db3, strbuf_value(sql), NULL, NULL, &errmsg);
		if (rc != SQLITE_OK)
			die("create table error: %s", errmsg);
	}
	if (dbop->stmt) {
		rc = sqlite3_finalize(dbop->stmt);
		if (rc != SQLITE_OK)
			die("sqlite3_finalize failed. (rc = %d)", rc);
		dbop->stmt = NULL;
	}
	if (dbop->stmt_put3) {
		rc = sqlite3_finalize(dbop->stmt_put3);
		if (rc != SQLITE_OK)
			die("dbop3_finalize failed. (rc = %d)", rc);
		dbop->stmt_put3 = NULL;
	}
	rc = sqlite3_close(dbop->db3);
	if (rc != SQLITE_OK)
		die("sqlite3_close failed. (rc = %d)", rc);
	dbop->db3 = NULL;
	if (dbop->tblname)
		free((void *)dbop->tblname);
	strbuf_close(dbop->sb);
	free(dbop);
}
#endif /* USE_SQLITE3 */
