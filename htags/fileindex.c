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
#include "config.h"
#endif
#include <ctype.h>
#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "regex.h"
#include "queue.h"
#include "global.h"
#include "incop.h"
#include "path2url.h"
#include "htags.h"
#include "common.h"

/*----------------------------------------------------------------------*/
/* File queue								*/
/*----------------------------------------------------------------------*/
/*
 * Usage:
 *
 * FILE *op;
 *
 * open_file_queue("a");
 * open_file_queue("b");
 * op = select_file_queue("a");
 * fputs("xxxxx", op);		-- write to file 'a'
 * op = select_file_queue("b");
 * fputs("xxxxx", op);		-- write to file 'b'
 * close_file_queue("a");
 * close_file_queue("b");
*/
struct file {
	SLIST_ENTRY(file) ptr;
	char path[MAXPATHLEN];
	FILE *op;
	int compress;
};

static SLIST_HEAD(, file) file_q;

/*
 * open_file_queue: open file and return file pointer.
 *
 *	i)	path	path name or command line.
 *	r)		file pointer
 *
 * You can get file pointer any time using select_file_queue() with path.
 */
static FILE *
open_file_queue(path)
	char *path;
{
	struct file *file = (struct file *)malloc(sizeof(struct file));

	if (!file)
		die("short of memory.");
	if (strlen(path) > MAXPATHLEN)
		die("path name too long.");
	strlimcpy(file->path, path, sizeof(file->path));
	if (cflag) {
		char command[MAXFILLEN];

		snprintf(command, sizeof(command), "gzip -c >%s", path);
		file->op = popen(command, "w");
		if (file->op == NULL)
			die("cannot execute command '%s'.", command);
		file->compress = 1;
	} else {
		file->op = fopen(path, "w");
		if (file->op == NULL)
			die("cannot create file '%s'.", path);
		file->compress = 0;
	}
	SLIST_INSERT_HEAD(&file_q, file, ptr);
	return file->op;
}
/*
 * select_file_queue: return file pointer for path.
 *
 *	i)	path	path name
 *	r)		file pointer
 *			NULL: path not found.
 */
static FILE *
select_file_queue(path)
	char *path;
{
	struct file *file;

	SLIST_FOREACH(file, &file_q, ptr) {
		if (!strcmp(file->path, path))
			return file->op;
	}
	die("cannot select no existent file.");
}
/*
 * close_file_queue: close file
 *
 *	i)	path	path name
 */
static void
close_file_queue(path)
	char *path;
{
	struct file *file;

	SLIST_FOREACH(file, &file_q, ptr) {
		if (!strcmp(file->path, path))
			break;
	}
	if (file == NULL)
		die("cannot close no existent file.");
	if (file->compress) {
		if (pclose(file->op) != 0) {
			char command[MAXFILLEN];
			snprintf(command, sizeof(command), "gzip -c >%s", file->path);
			die("command '%s' failed.", command);
		}
	} else
		fclose(file->op);
	SLIST_REMOVE(&file_q, file, file, ptr);
	free(file);
}
/*----------------------------------------------------------------------*/
/* Directory stack							*/
/*----------------------------------------------------------------------*/
/*
 * Usage:
 *
 * struct dirstack *sp = make_stack("stack1");
 * set_stack(sp, "aaa/bbb/ccc");
 *
 *	sp = ("aaa" "bbb" "ccc")
 *
 * push_stack(sp, "ddd");
 *
 *      sp = ("aaa" "bbb" "ccc" "ddd")
 *
 * char *s = top_stack(sp);
 *
 *	sp = ("aaa" "bbb" "ccc" "ddd")
 *	s = "ddd"
 *
 * char *s = pop_stack(sp);
 *
 *      sp = ("aaa" "bbb" "ccc")
 *	s = "ddd"
 *
 * char *s = shift_stack(sp);
 *
 *      sp = ("bbb" "ccc")
 *	s = "aaa"
 *
 * char *s = join_stack(sp);
 *
 *      sp = ("bbb" "ccc")
 *	s = "bbb/ccc"
 *
 * delete_stack(sp);
 */
static int trace = 0;

#define TOTAL_STRING_SIZE       2048

struct dirstack {
        char name[32];
        char buf[TOTAL_STRING_SIZE];
        char join[TOTAL_STRING_SIZE];
        char shift[TOTAL_STRING_SIZE];
        char *start;
        char *last;
        int leaved;
        int count;
};

#define count_stack(sp) ((sp)->count)
#define bottom_stack(sp) ((sp)->start)

void
settrace()
{
	trace = 1;
}
/*
 * dump_stack: print stack list to stderr.
 *
 *	i)	sp	stack descriptor
 */
void
static dump_stack(sp, label)
	struct dirstack *sp;
	char *label;
{
	char *start = sp->start;
	char *last = sp->last - 1;
	char *p;

	fprintf(stderr, "%s(%s): ", label, sp->name);
	for (p = sp->buf; p < last; p++) {
		if (p == start)
			fprintf(stderr, "[");
		fputc((*p == 0) ? ' ' : *p, stderr);
	}
	if (start == sp->last)
		fprintf(stderr, "[");
	fprintf(stderr, "]\n");
}
/*
 * make_stack: make new stack.
 *
 *	r)		stack descriptor
 */
static struct dirstack *
make_stack(name)
	char *name;
{
	struct dirstack *sp = (struct dirstack *)malloc(sizeof(struct dirstack));
	if (!sp)
		die("short of memory.");
	strlimcpy(sp->name, name, TOTAL_STRING_SIZE); 
	sp->start = sp->last = sp->buf;
	sp->leaved = TOTAL_STRING_SIZE;
	sp->count = 0;
	return sp;
}
/*
 * set_stack: set path with splitting by '/'.
 *
 *	i)	sp	stack descriptor
 *	i)	path	path name
 *
 * path = 'aaa/bbb/ccc';
 *
 *	sp->buf
 *	+---------------------------+
 *	|aaa\0bbb\0ccc\0            |
 *	+---------------------------+
 *	^               ^
 * 	sp->start       sp->last
 *
 *
 */
static void
set_stack(sp, path)
	struct dirstack *sp;
	char *path;
{
	int length = strlen(path) + 1;
	char *p;

	if (length > TOTAL_STRING_SIZE)
		die("path name too long.");
	sp->start = sp->buf;
	sp->last = sp->buf + length;
	sp->leaved = TOTAL_STRING_SIZE - length;
	if (sp->leaved < 0)
		abort();
	sp->count = 1;
	strlimcpy(sp->buf, path, TOTAL_STRING_SIZE);
	/*
	 * Split path by sep char.
	 */
	for (p = sp->buf; *p; p++) {
		if (*p == sep) {
			*p = '\0';
			sp->count++;
		}
	}
	if (trace)
		dump_stack(sp, "set_stack");
}
/*
 * push_stack: push new string on the stack.
 *
 *	i)	sp	stack descriptor
 *	i)	s	string
 */
static void
push_stack(sp, s)
	struct dirstack *sp;
	char *s;
{
	int length = strlen(s) + 1;

	if (sp->leaved < length) {
		fprintf(stderr, "s = %s, leaved = %d, length = %d, bufsize = %d\n",
			s, sp->leaved, length, sp->last - sp->buf);
		dump_stack(sp, "abort in push_stack");
		abort();
	}
	strlimcpy(sp->last, s, length);
	sp->last += length;
	sp->leaved -= length;
	if (sp->leaved < 0)
		abort();
	sp->count++;
	if (trace)
		dump_stack(sp, "push_stack");
}
/*
 * top_stack: return the top value of the stack.
 *
 *	i)	sp	stack descriptor
 *	r)		string
 */
static char *
top_stack(sp)
	struct dirstack *sp;
{
	char *start = sp->start;
	char *last = sp->last;

	if (start > last)
		die("internal error in top_stack(1).");
	if (start == last)
		return NULL;
	last--;
	if (*last)
		die("internal error in top_stack(2).");
	if (start == last)
		return last;	/* return NULL string */
	for (last--; start < last && *last != 0; last--)
		;
	if (start < last)
		last++;
	return last;
}
/*
 * next_stack: return the next value of the stack.
 *
 *	i)	sp	stack descriptor
 *	i)	cur	current value
 *	r)		string
 */
static char *
next_stack(sp, cur)
	struct dirstack *sp;
	char *cur;
{
	char *last = sp->last;

	if (cur >= last)
		return NULL;
	for (; cur < last && *cur != 0; cur++)
		;
	cur++;
	if (cur >= last)
		return NULL;
	return cur;
}
/*
 * pop_stack: return the top of the stack and discard it.
 *
 *	i)	sp	stack descriptor
 *	r)		string
 */
static char *
pop_stack(sp)
	struct dirstack *sp;
{
	char *last = top_stack(sp);
	int length = strlen(last) + 1;

	if (!last)
		return NULL;
	sp->count--;
	sp->last = last;
	sp->leaved += length;
	if (trace)
		dump_stack(sp, "pop_stack");
	return last;
}
/*
 * shift_stack: return the bottom of the stack and discard it.
 *
 *	i)	sp	stack descriptor
 *	r)		string
 */
static char *
shift_stack(sp)
	struct dirstack *sp;
{
	char *start = sp->start;
	char *last = sp->last;
	int length = strlen(start) + 1;

	if (start == last)
		return NULL;
	sp->start += length;
	sp->count--;
	if (sp->count == 0) {
		strlimcpy(sp->shift, start, TOTAL_STRING_SIZE); 
		start = sp->shift;
		sp->start = sp->last = sp->buf;
		sp->leaved = TOTAL_STRING_SIZE;
	}
	if (trace)
		dump_stack(sp, "shift_stack");
	return start;
}
/*
 * copy_stack: make duplicate stack.
 *
 *	i)	to	stack descriptor
 *	i)	from	stack descriptor
 */
static void
copy_stack(to, from)
	struct dirstack *to;
	struct dirstack *from;
{
	char *start = from->start;
	char *last = from->last;
	char *p = to->buf;

	to->start = to->buf;
	to->last = to->start + (last - start);
	to->leaved = from->leaved;
	to->count = from->count;
	while (start < last)
		*p++ = *start++;
}
/*
 * join_stack: join each unit and make a path.
 *
 *	i)	sp	stack descriptor
 *	r)		path name
 */
static char *
join_stack(sp)
	struct dirstack *sp;
{
	char *start = sp->start;
	char *last = sp->last - 1;
	char *p = sp->join;

	for (; start < last; start++)
		*p++ = (*start) ? *start : sep;
	*p = 0;
	if (trace)
		dump_stack(sp, "join_stack");
	return (char *)sp->join;
}
/*
 * delete_stack: delete stack.
 *
 *	i)	sp	stack descriptor
 */
static void
delete_stack(sp)
	struct dirstack *sp;
{
	free(sp);
}
/*----------------------------------------------------------------------*/
/* Main procedure							*/
/*----------------------------------------------------------------------*/
/*
 * extract_lastname: extract the last name of include line.
 *
 *	i)	image	source image of include
 *	i)	is_php	1: is PHP source
 *	r)		last name
 */
static char *
extract_lastname(image, is_php)
	char *image;
	int is_php;
{
	static char buf[MAXBUFLEN];
	char *p, *q;
	int sep;

	/*
	 * C:	#include <xxx/yyy/zzz.h>
	 *	#include "xxx/yyy/zzz.h"
	 * PHP: include('xxx/yyy/zzz');
	 */
	p = image;
	while (*p && isspace((unsigned char)*p))		/* skip space */
		p++;
	if (!*p)
		return NULL;
	if (*p == '#') {
		if (is_php)
			return NULL;
		p++;
		while (*p && isspace((unsigned char)*p))	/* skip space */
			p++;
		if (!*p)
			return NULL;
	}
	/*
	 * If match to one of the include keywords then points
	 * the following character of the keyword.
	 *            p
	 *            v
	 * ... include ....
	 */
	if (is_php) {
		if ((p = locatestring(p, "include", MATCH_AT_FIRST)) == NULL)
			return NULL;
	} else {
		char *q;

		if (((q = locatestring(p, "include_next", MATCH_AT_FIRST)) == NULL) &&
		    ((q = locatestring(p, "import", MATCH_AT_FIRST)) == NULL) &&
		    ((q = locatestring(p, "include", MATCH_AT_FIRST)) == NULL))
			return NULL;
		p = q;
	}
	while (*p && isspace((unsigned char)*p))		/* skip space */
		p++;
	if (is_php && *p == '(') {
		p++;
		while (*p && isspace((unsigned char)*p))	/* skip space */
			p++;
	}
	sep = *p;
	if (is_php) {
		if (sep != '\'' && sep != '"')
			return NULL;
	} else {
		if (sep != '<' && sep != '"')
			return NULL;
	}
	if (sep == '<')
		sep = '>';
	p++;
	if (!*p)
		return NULL;
	q = buf; 
	while (*p && *p != '\n' && *p != sep)
		*q++ = *p++;
	*q = '\0';
	if (*p == sep) {
		p = locatestring(buf, "/", MATCH_LAST);
		if (p)
			p++;
		else
			p = buf;
		return p;
	}
	return NULL;
}
/*
 * makefileindex: make file index.
 *
 *	i)	file
 *	o)	files
 */
int
makefileindex(file, files)
	char *file;
	STRBUF *files;
{
	FILE *FIND, *FILEMAP, *FILES, *STDOUT, *op = NULL;
	char *_;
	int count = 0;
	char indexlink[80];
	char edit[MAXURLLEN];
	STRBUF *sb = strbuf_open(0);
	STRBUF *input = strbuf_open(0);
	char *target = (Fflag) ? "mains" : "_top";
	struct dirstack *dirstack = make_stack("dirstack");
	struct dirstack *fdstack = make_stack("fdstack");
	char *command = (other_files) ? "gtags --find --other | gnusort -t / -k 2"
					: "gtags --find";
	struct dirstack *push = make_stack("push");
	struct dirstack *pop = make_stack("pop");
	/*
	 * for collecting include files.
	 */
	int flags = REG_EXTENDED;
	regex_t is_include_file;

	if (w32)
		flags |= REG_ICASE;
	strbuf_reset(sb);
	strbuf_puts(sb, "\\.(");
	{
		char *p = include_file_suffixes;
		int c;

		while ((c = (unsigned char)*p++) != '\0') {
			if (isregexchar(c))
				strbuf_putc(sb, '\\');
			else if (c == ',')
				c = '|';
			strbuf_putc(sb, c);
		}
	}
	strbuf_puts(sb, ")$");
	if (regcomp(&is_include_file, strbuf_value(sb), flags) != 0)
		die("cannot compile regular expression '%s'.", strbuf_value(sb));

	/*
	 * preparations.
	 */
	snprintf(indexlink, sizeof(indexlink),
		(Fflag) ? "../files.%s" : "../mains.%s", normal_suffix);

	if ((FIND = popen(command, "r")) == NULL)
		die("cannot fork.");
	if ((FILES = fopen(makepath(distpath, file, NULL), "w")) == NULL)
		die("cannot open file '%s'.", file);

	fprintf(FILES, "%s\n", html_begin);
	fprintf(FILES, set_header(title_file_index));
	fprintf(FILES, "%s\n", body_begin);
	snprintf(edit, sizeof(edit), "files.%s", normal_suffix); 
	fprintf(FILES, "%s\n", anchor(Hn(2, title_file_index), edit));
	if (!no_order_list)
		fprintf(FILES, "%s\n", list_begin);
	STDOUT = FILES;

	FILEMAP = NULL;
	if (map_file) {
		if (!(FILEMAP = fopen(makepath(distpath, "FILEMAP", NULL), "w")))
                        die("cannot open '%s'.", makepath(distpath, "FILEMAP", NULL));
	}
	while ((_ = strbuf_fgets(input, FIND, STRBUF_NOCRLF)) != NULL) {
		char fname[MAXPATHLEN];
		int notsource = 0;

		/* It seems like README or ChangeLog. */
		if (*_ == ' ') {
			if (!other_files)
				continue;
			_++;
			if (is_binary(_))
				continue;
			notsource = 1;
		}
		_ += 2;			/* remove './' */
		count++;
		message(" [%d] adding %s", count, _);
		set_stack(push, _);	
		strlimcpy(fname, pop_stack(push), sizeof(fname));
		copy_stack(pop, dirstack);
		while (count_stack(push) && count_stack(pop) && !strcmp(bottom_stack(push), bottom_stack(pop))) {
			(void)shift_stack(push);
			(void)shift_stack(pop);
		}
		if (count_stack(push) || count_stack(pop)) {
			char *parent, *path;

			while (count_stack(pop)) {
				(void)pop_stack(dirstack);
				parent = count_stack(dirstack) ? path2url(join_stack(dirstack)) : indexlink;
				if (no_order_list)
					fprintf(STDOUT, "%s\n", br);
				else
					fprintf(STDOUT, "%s\n", list_end);
				fprintf(STDOUT, "<a href='%s' title='Parent Directory'>", parent);
				if (icon_list)
					fprintf(STDOUT, "<img src='../icons/%s.%s' alt='[..]' %s>", back_icon, icon_suffix, icon_spec);
				else
					fprintf(STDOUT, "[..]");
				fprintf(STDOUT, "</a>\n");
				fprintf(STDOUT, "%s\n", body_end);
				fprintf(STDOUT, "%s\n", html_end);
				path = pop_stack(fdstack);	
				close_file_queue(path);
				file_count++;
				if (count_stack(fdstack))
					STDOUT = select_file_queue(top_stack(fdstack));
				pop_stack(pop);
			}
			while (count_stack(push)) {
				char cur[MAXPATHLEN];
				char *last;

				parent = count_stack(dirstack) ? path2url(join_stack(dirstack)) : indexlink;
				push_stack(dirstack, shift_stack(push));
				path = join_stack(dirstack);
				snprintf(cur, sizeof(cur), "%s/files/%s", distpath, path2url(path));
				last = (full_path) ? path : top_stack(dirstack);

				strbuf_reset(sb);
				if (!no_order_list)
					strbuf_puts(sb, list_item);

				strbuf_sprintf(sb, "<a href='%s%s' title='%s/'>",
					count_stack(dirstack) == 1 ? "files/" : "",
					path2url(path),
					path);
				if (icon_list) {
					strbuf_sprintf(sb, "<img src='%sicons/%s.%s' alt='[%s/]' hspace='3' %s>",
						count_stack(dirstack) == 1 ? "" : "../", dir_icon, icon_suffix, path, icon_spec);
				}
				strbuf_sprintf(sb, "%s/</a>\n", last);
				if (no_order_list)
					strbuf_puts(sb, br);
				if (count_stack(dirstack) == 1)
					strbuf_puts(files, strbuf_value(sb));
				else
					fprintf(STDOUT, "%s", strbuf_value(sb));
				op = open_file_queue(cur);
				STDOUT = op;
				push_stack(fdstack, cur);
				fprintf(STDOUT, "%s\n", html_begin);
				strbuf_reset(sb);
				strbuf_puts(sb, path);
				strbuf_putc(sb, '/');
				fprintf(STDOUT, set_header(strbuf_value(sb)));
				fprintf(STDOUT, "%s\n", body_begin);
				fprintf(STDOUT, "<h2>");
				fprintf(STDOUT, "<a href='%s'>root</a>/", indexlink);
				{
					struct dirstack *p = make_stack("tmp");
					char *s;
					int anchor;

					for (s = bottom_stack(dirstack); s; s = next_stack(dirstack, s)) {
						push_stack(p, s);
						anchor = count_stack(p) < count_stack(dirstack) ? 1 : 0;
						if (anchor)
							fprintf(STDOUT, "<a href='%s'>", path2url(join_stack(p)));
						fprintf(STDOUT, s);
						if (anchor)
							fprintf(STDOUT, "</a>");
						fprintf(STDOUT, "/");
					}
					delete_stack(p);
				}
				fprintf(STDOUT, "</h2>\n");
				fprintf(STDOUT, "<a href='%s' title='Parent Directory'>", parent);
				if (icon_list)
					fprintf(STDOUT, "<img src='../icons/%s.%s' alt='[..]' %s>", back_icon, icon_suffix, icon_spec);
				else
					fprintf(STDOUT, "[..]");
				fprintf(STDOUT, "</a>\n");
				if (!no_order_list)
					fprintf(STDOUT, "%s\n", list_begin);
				else
					fprintf(STDOUT, "%s%s\n", br, br);
			}
		}
		/*
                 * We assume the file which has one of the following suffixes
                 * as a candidate of include file.
                 *
                 * C: .h
                 * C++: .hxx, .hpp, .H
                 * PHP: .inc.php
		 */
		if (regexec(&is_include_file, _, 0, 0, 0) == 0)
			put_inc(fname, _, count);
		strbuf_reset(sb);
		if (!no_order_list)
			strbuf_puts(sb, list_item);
		strbuf_puts(sb, "<a href='");
		if (notsource && dynamic) {
			if (!(*action == '/' || count_stack(dirstack) == 0))
				strbuf_puts(sb, "../");
			strbuf_sprintf(sb, "%s?pattern=%s&type=source",
				action, encode(_));
		} else {
			if (count_stack(dirstack))
				strbuf_puts(sb, "../");
			strbuf_sprintf(sb, "%s/%s", SRCS, path2url(_));
		}
		strbuf_sprintf(sb, "' target='%s' title='%s'>", target, _);
		if (icon_list) {
			const char *lang, *suffix, *text_icon;

			if ((suffix = locatestring(_, ".", MATCH_LAST)) != NULL
			    && (lang = decide_lang(suffix)) != NULL
			    && (strcmp(lang, "c") == 0 || strcmp(lang, "cpp") == 0
			       || strcmp(lang, "yacc") == 0))
				text_icon = c_icon;
			else
				text_icon = file_icon;
			strbuf_puts(sb, "<img src='");
			if (count_stack(dirstack))
				strbuf_puts(sb, "../");
			strbuf_sprintf(sb, "icons/%s.%s' alt='[%s]' hspace='3' %s>",
				text_icon, icon_suffix, _, icon_spec);
		}
		if (full_path) {
			strbuf_puts(sb, _);
		} else {
			char *last = locatestring(_, "/", MATCH_LAST);
			if (last)
				last++;
			else
				last = _;
			strbuf_puts(sb, last);
		}
		strbuf_puts(sb, "</a>\n");
		if (no_order_list)
			strbuf_sprintf(sb, "%s\n", br);
		if (map_file)
			fprintf(FILEMAP, "%s\t%s/%s\n", _, SRCS, path2url(_));
		if (count_stack(dirstack) == 0)
			strbuf_puts(files, strbuf_value(sb));
		else
			fprintf(STDOUT, "%s", strbuf_value(sb));
	}
	if (map_file)
		fclose(FILEMAP);
	if (pclose(FIND) != 0)
		die("cannot traverse directory.(%s)", command);
	while (count_stack(dirstack) > 0) {
		char *parent;

		pop_stack(dirstack);
		parent = (count_stack(dirstack) > 0) ? path2url(join_stack(dirstack)) : indexlink;
		if (no_order_list)
			fprintf(STDOUT, "%s\n", br);
		else
			fprintf(STDOUT, "%s\n", list_end);
		fprintf(STDOUT, "<a href='%s' title='Parent Directory'>", parent);
		if (icon_list)
			fprintf(STDOUT, "<img src='../icons/%s.%s' alt='[..]' %s>", back_icon, icon_suffix, icon_spec);
		else
			fprintf(STDOUT, "[..]");
		fprintf(STDOUT, "</a>\n");
		fprintf(STDOUT, "%s\n", body_end);
		fprintf(STDOUT, "%s\n", html_end);
		close_file_queue(pop_stack(fdstack));
		file_count++;
		if (count_stack(fdstack) > 0)
			STDOUT = select_file_queue(top_stack(fdstack));
	}
	fputs(strbuf_value(files), FILES);
	if (no_order_list)
		fprintf(FILES, "%s\n", br);
	else
		fprintf(FILES, "%s\n", list_end);
	fprintf(FILES, "%s\n", body_end);
	fprintf(FILES, "%s\n", html_end);
	fclose(FILES);
	file_count++;

	delete_stack(dirstack);
	delete_stack(fdstack);
	delete_stack(push);
	delete_stack(pop);

	strbuf_close(sb);
	strbuf_close(input);
	return count;
}
void
makeincludeindex()
{
	FILE *PIPE;
	STRBUF *input = strbuf_open(0);
	char *command;
	char *_;
	struct data *inc;
	char *target = (Fflag) ? "mains" : "_top";

	/*
	 * Pick up include pattern.
	 *
	 * C: #include "xxx.h"
	 * PHP: include("xxx.inc.php");
	 */
	command = "global -gnx \"^[ \\t]*(#[ \\t]*(import|include)|include[ \\t]*\\()\"";
	if ((PIPE = popen(command, "r")) == NULL)
		die("cannot fork.");
	strbuf_reset(input);
	while ((_ = strbuf_fgets(input, PIPE, STRBUF_NOCRLF)) != NULL) {
		SPLIT ptable;
                char *nouse, *lno, *filename, *image;
		char *last, buf[MAXBUFLEN];
		int is_php = 0;
		const char *lang, *suffix;

		if (split(_, 4, &ptable) < 4) {
			recover(&ptable);
			die("too small number of parts in makefileindex().");
		}
		nouse = ptable.part[0].start;
		lno = ptable.part[1].start;
		filename = ptable.part[2].start;
		image = ptable.part[3].start;

		if ((suffix = locatestring(filename, ".", MATCH_LAST)) != NULL
		    && (lang = decide_lang(suffix)) != NULL
		    && strcmp(lang, "php") == 0)
			is_php = 1;
		last = extract_lastname(image, is_php);
		if (last == NULL || get_inc(last) == NULL)
			continue;
		recover(&ptable);
		/*
		 * s/^[^ \t]+/$last/;
		 */
		{
			char *p, *q = buf;

			for (p = last; *p; p++)
				*q++ = *p;
			for (p = _; *p && *p != ' ' && *p != '\t'; p++)
				;
			for (; *p; p++)
				*q++ = *p;
			*q = '\0';
		}
		put_included(last, buf);
	}
	if (pclose(PIPE) != 0)
		die("terminated abnormally.");

	for (inc = first_inc(); inc; inc = next_inc()) {
		char *last = inc->name;
		int no = inc->id;
		struct data *data;
		FILE *INCLUDE;

		if (inc->count > 1) {
			char path[MAXPATHLEN];

			snprintf(path, sizeof(path), "%s/%s/%d.%s", distpath, INCS, no, HTML);
			INCLUDE = open_file_queue(path);
			fprintf(INCLUDE, "%s\n", html_begin);
			fprintf(INCLUDE, set_header(last));
			fprintf(INCLUDE, "%s\n", body_begin);
			fprintf(INCLUDE, "%s\n", verbatim_begin);
			{
				char *filename = strbuf_value(inc->contents);
				int count = inc->count;

				for (; count; filename += strlen(filename) + 1, count--)
					fprintf(INCLUDE, "<a href='../%s/%s' target='%s'>%s</a>\n", SRCS, path2url(filename), target, filename);
			}
			fprintf(INCLUDE, "%s\n", verbatim_end);
			fprintf(INCLUDE, "%s\n", body_end);
			fprintf(INCLUDE, "%s\n", html_end);
			close_file_queue(path);
			file_count++;
			/*
			 * inc->path == NULL means that information already
			 * written to file.
			 */
			strbuf_reset(inc->contents);
		}
		data = get_included(last);
		if (!data)
			continue;
		if (data->count == 1) {
			SPLIT ptable;
			char *nouse, *lno, *filename;
			char buf[1024];

			if (split(strbuf_value(data->contents), 4, &ptable) < 4) {
				recover(&ptable);
				die("too small number of parts in makefileindex().");
			}
			nouse = ptable.part[0].start;
			lno = ptable.part[1].start;
			filename = ptable.part[2].start;
			snprintf(buf, sizeof(buf), "%s %s", lno, filename);
			recover(&ptable);
			strbuf_reset(data->contents);
			strbuf_puts(data->contents, buf);
		} else {
			char path[MAXPATHLEN];

			snprintf(path, sizeof(path), "%s/%s/%d.%s", distpath, INCREFS, no, HTML);
			INCLUDE = open_file_queue(path);
			fprintf(INCLUDE, "%s\n", html_begin);
			fprintf(INCLUDE, set_header(last));
			fprintf(INCLUDE, "%s\n", body_begin);
			fprintf(INCLUDE, "%s\n", gen_list_begin());
			{
				char *line = strbuf_value(data->contents);
				int count = data->count;

				for (; count; line += strlen(line) + 1, count--)
					fprintf(INCLUDE, "%s\n", gen_list_body("../S", line));
			}
			fprintf(INCLUDE, "%s\n", gen_list_end());
			fprintf(INCLUDE, "%s\n", body_end);
			fprintf(INCLUDE, "%s\n", html_end);
			close_file_queue(path);
			file_count++;
			/*
			 * inc->path == NULL means that information already
			 * written to file.
			 */
			strbuf_reset(data->contents);
			data->id = no;
		}
	}
	strbuf_close(input);
}
