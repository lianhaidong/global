/*
 * Copyright (c) 1996, 1997, 1998, 1999 Shigio Yamaguchi
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
#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#include <ctype.h>

#include "global.h"
#include "anchor.h"
#include "cache.h"
#include "common.h"
#include "incop.h"
#include "path2url.h"
#include "htags.h"

/*
 * Input/Output descriptor.
 */
FILE *out;
FILE *in;

static STRBUF *outbuf;
static char *curpfile;
static int warned;
static int last_lineno;

/*
 * IO routine.
 */
static FILE *
open_input_file(file)
char *file;
{
	char command[MAXFILLEN];
	FILE *ip;

	snprintf(command, sizeof(command), "gtags --expand -%d < %s", tabs, file);
	ip = popen(command, "r");
	if (!ip)
		die("cannot execute '%s'.", command);
	curpfile = file;
	warned = 0;
	return ip;
}
static void
close_input_file()
{
	if (pclose(in) < 0)
		die("command 'gtags --expand -%d' failed.", tabs);
}
static FILE *
open_output_file(file)
	char *file;
{
	char command[MAXFILLEN];
	FILE *op;
	extern int cflag;

	if (cflag) {
		snprintf(command, sizeof(command), "gzip -c >%s", file);
		op = popen(command, "w");
		if (!op)
			die("cannot execute '%s'.", command);
	} else {
		op = fopen(file, "w");
		if (!op)
			die("cannot create file '%s'.", file);
	}
	if (!outbuf)
		outbuf = strbuf_open(0);
	else
		strbuf_reset(outbuf);
	return op;
}
static void
close_output_file()
{
	extern int cflag;

	if (cflag) {
		if (pclose(out) < 0)
			die("command 'gzip -c' failed.");
	} else
		fclose(out);
}
/*
 * reverse of atoi
 */
static char *
itoa(n)
	int n;
{
	static char buf[32];

	snprintf(buf, sizeof(buf), "%d", n);
	return buf;
}
void
echoc(int c)
{
        strbuf_putc(outbuf, c);
}
void
echos(const char *s)
{
        strbuf_puts(outbuf, s);
}

/*
 * fill_anchor: fill anchor into file name
 *
 *       i)      $root   root or index page
 *       i)      $path   path name
 *       r)              hypertext file name string
 */
char *
fill_anchor(root, path)
	char *root;
	char *path;
{
	static STRBUF *sb = NULL;
	char buf[MAXBUFLEN], *limit, *p;

	if (sb)
		strbuf_reset(sb);
	else
		sb = strbuf_open(0);
	strlimcpy(buf, path, sizeof(buf));
	for (p = buf; *p; p++)
		if (*p == sep)
			*p = '\0';
	limit = p;

	strbuf_puts(sb, "<A HREF=");
	strbuf_puts(sb, root);
	strbuf_puts(sb, ">root</A>/");
	{
		char *next;

		for (p = buf; p < limit; p += strlen(p) + 1) {
			char *path = buf;
			char *unit = p;

			next = p + strlen(p) + 1;
			if (next > limit) {
				strbuf_puts(sb, unit);
				break;
			}
			if (p > buf)
				*(p - 1) = sep;
			strbuf_sprintf(sb, "<A HREF=../files/%s>%s</A>/",
				path2url(path), unit);
		}
	}
        return strbuf_value(sb);
}

/*
 * link_format: make hypertext from anchor array.
 *
 *	i)	(previous, next, first, last, top, bottom)
 *	r)	HTML
 */
char *
link_format(ref)
        int ref[A_SIZE];
{
	static STRBUF *sb = NULL;
	char **label = icon_list ? anchor_comment : anchor_label;
	char **icons = anchor_icons;
	int i;

	if (sb)
		strbuf_reset(sb);
	else
		sb = strbuf_open(0);
	for (i = 0; i < A_LIMIT; i++) {
		if (i == A_INDEX) {
			strbuf_puts(sb, "<A HREF=../mains.");
			strbuf_puts(sb, normal_suffix);
			strbuf_putc(sb, '>');
		} else if (i == A_HELP) {
			strbuf_puts(sb, "<A HREF=../help.");
			strbuf_puts(sb, normal_suffix);
			strbuf_putc(sb, '>');
		} else if (ref[i]) {
			strbuf_puts(sb, "<A HREF=#");
			if (ref[i] == -1)
				strbuf_puts(sb, "TOP");
			else if (ref[i] == -2)
				strbuf_puts(sb, "BOTTOM");
			else
				strbuf_putn(sb, ref[i]);
			strbuf_putc(sb, '>');
		}
		if (icon_list) {
			strbuf_puts(sb, "<IMG SRC=../icons/");
			if (i != A_INDEX && i != A_HELP && ref[i] == 0)
				strbuf_puts(sb, "n_");
			strbuf_puts(sb, icons[i]);
			strbuf_putc(sb, '.');
			strbuf_puts(sb, icon_suffix);
			strbuf_sprintf(sb, " ALT=[%s] %s>", label[i], icon_spec);
		} else {
			strbuf_sprintf(sb, "[%s]", label[i]);
		}
		if (i == A_INDEX || i == A_HELP || ref[i] != 0)
			strbuf_puts(sb, "</A>");
	}
        return strbuf_value(sb);
}
/*
 * generate_guide: generate guide string for definition line.
 *
 *	i)	lineno	line number
 *	r)		guide string
 */
char *
generate_guide(lineno)
	int lineno;
{
	static STRBUF *sb = NULL;
	int i = 0;

	if (!sb)
		sb = strbuf_open(0);
	else
		strbuf_reset(sb);
	if (definition_header == RIGHT_HEADER)
		i = 4;
	else if (nflag)
		i = ncol + 1;
	if (i > 0)
		for (; i > 0; i--)
			strbuf_putc(sb, ' ');
	strbuf_sprintf(sb, "%s/* ", comment_begin);
	strbuf_puts(sb, link_format(anchor_getlinks(lineno)));
	if (show_position)
		strbuf_sprintf(sb, "%s[+%d %s]%s",
			position_begin, lineno, curpfile, position_end);
	strbuf_sprintf(sb, " */%s", comment_end);

	return strbuf_value(sb);
}
/*
 * tooltip: generate tooltip string
 *
 *	i)	type	I,R,Y,D,M
 *	i)	lno	line number
 *	i)	opt	
 *	r)		tooltip string
 */
char *
tooltip(type, lno, opt)
	int type;
	int lno;
	char *opt;
{
	static STRBUF *sb = NULL;

	if (!sb)
		sb = strbuf_open(0);
	else
		strbuf_reset(sb);
	if (lno > 0) {
		if (type == 'I')
			strbuf_puts(sb, "Included from");
		else if (type == 'R')
			strbuf_puts(sb, "Defined at");
		else if (type == 'Y')
			strbuf_puts(sb, "Used at");
		else
			strbuf_puts(sb, "Refered from");
		strbuf_putc(sb, ' ');
		strbuf_putn(sb, lno);
		if (opt) {
			strbuf_puts(sb, " in ");
			strbuf_puts(sb, opt);
		}
	} else {
		strbuf_puts(sb, "Multiple ");
		if (type == 'I')
			strbuf_puts(sb, "included from");
		else if (type == 'R')
			strbuf_puts(sb, "defined in");
		else if (type == 'Y')
			strbuf_puts(sb, "used in");
		else
			strbuf_puts(sb, "refered from");
		strbuf_putc(sb, ' ');
		strbuf_puts(sb, opt);
		strbuf_putc(sb, ' ');
		strbuf_puts(sb, "places");
	}
	strbuf_putc(sb, '.');
	return strbuf_value(sb);
}
/*
 * put_anchor: output HTML anchor.
 *
 *	i)	name	tag
 *	i)	type	tag type
 *	i)	lineno	current line no
 */
void
put_anchor(name, type, lineno)
	char *name;
	int type;
	int lineno;
{
	char *line;
	int db;

	if (type == 'R')
		db = GTAGS;
	else if (type == 'Y')
		db = GSYMS;
	else	/* 'D', 'M' or 'T' */
		db = GRTAGS;
	line = cache_get(db, name);
	if (line == NULL) {
		if ((type == 'R' || type == 'Y') && wflag) {
			warning("%s %d %s(%c) found but not defined.",
				curpfile, lineno, name, type);
			if (colorize_warned_line)
				warned = 1;
		}
		strbuf_puts(outbuf, name);
	} else {
		if (*line == ' ') {
			int id = atoi(++line);
			char *count = locatestring(line, " ", MATCH_FIRST) + 1;

			strbuf_puts(outbuf, "<A HREF=");
			if (dynamic) {
				char *dir = (*action == '/') ? "" : "../";
				char *s;

				if (db == GTAGS)
					s = "definitions";
				else if (db == GRTAGS)
					s = "reference";
				else
					s = "symbol";
				strbuf_sprintf(outbuf, "%s%s?pattern=%s&type=%s",
					dir, action, name, s);
			} else {
				char *dir;

				if (type == 'R')
					dir = DEFS;
				else if (type == 'Y')
					dir = SYMS;
				else	/* 'D', 'M' or 'T' */
					dir = REFS;
				strbuf_sprintf(outbuf, "../%s/%d.%s", dir, id, HTML);
			}
			strbuf_sprintf(outbuf, " TITLE=\"%s\">%s</A>", tooltip(type, -1, count), name);
		} else {
			int lno = atoi(line);
			char *filename = strmake(locatestring(line, " ", MATCH_FIRST) + 1, " ")
						+ 2;	/* remove './' */
			char *url = path2url(filename);
			strbuf_sprintf(outbuf, "<A HREF=../%s/%s#%d TITLE=\"%s\">%s</A>",
				SRCS, url, lno, tooltip(type, lno, filename), name);
		}
	}
}
/*
 * put_include_anchor: output HTML anchor.
 *
 *	i)	inc	inc structure
 *	i)	path	path name for display
 */
void
put_include_anchor(inc, path)
	struct data *inc;
	char *path;
{
	strbuf_puts(outbuf, "<A HREF=");
	if (inc->count == 1)
		strbuf_puts(outbuf, path2url(strbuf_value(inc->contents)));
	else
		strbuf_sprintf(outbuf, "../%s/%d.%s", INCS, inc->id, HTML);
	strbuf_putc(outbuf, '>');
	strbuf_puts(outbuf, path);
	strbuf_puts(outbuf, "</A>");
}
/*
 * Tag level output functions.
 */
void
put_reserved_word(word)
        char *word;
{
	strbuf_puts(outbuf, reserved_begin);
	strbuf_puts(outbuf, word);
	strbuf_puts(outbuf, reserved_end);
}
void
put_macro(word)
        char *word;
{
	strbuf_puts(outbuf, sharp_begin);
	strbuf_puts(outbuf, word);
	strbuf_puts(outbuf, sharp_end);
}
void
put_char(c)
        int c;
{
        if (c == '<')
		strbuf_puts(outbuf, quote_little);
        else if (c == '>')
		strbuf_puts(outbuf, quote_great);
        else if (c == '&')
		strbuf_puts(outbuf, quote_amp);
        else
		strbuf_putc(outbuf, c);
}
void
put_string(s)
        char *s;
{
	for (; *s; s++)
		put_char(*s);
}
void
put_brace(text)
        char *text;
{
	strbuf_puts(outbuf, brace_begin);
	strbuf_puts(outbuf, text);
	strbuf_puts(outbuf, brace_end);
}

static char lineno_format[32];
/*
 * common procedure for line control.
 */
static char lineno_format[32];
static char *guide = NULL;

void
put_begin_of_line(lineno)
        int lineno;
{
        if (definition_header != NO_HEADER) {
                if (define_line(lineno))
                        guide = generate_guide(lineno);
                else
                        guide = NULL;
        }
        if (guide && definition_header == BEFORE_HEADER) {
                fputs(guide, out);
                fputc('\n', out);
                guide = NULL;
        }
}
void
put_end_of_line(lineno)
	int lineno;
{
	fprintf(out, "<A NAME=%d>", lineno);
        if (nflag)
                fprintf(out, lineno_format, lineno);
	if (warned)
		fputs(warned_line_begin, out);

	/* flush output buffer */
	fputs(strbuf_value(outbuf), out);
	strbuf_reset(outbuf);

	if (warned)
		fputs(warned_line_end, out);
	if (guide == NULL)
        	fputc('\n', out);
	else {
		if (definition_header == RIGHT_HEADER)
			fputs(guide, out);
		fputc('\n', out);
		if (definition_header == AFTER_HEADER) {
			fputs(guide, out);
			fputc('\n', out);
		}
		guide = NULL;
	}
	warned = 0;

	/* save for the other job in this module */
	last_lineno = lineno;
}
/*
 *
 * src2html: convert source code into HTML
 *
 *       i)      src   source file     - Read from
 *       i)      html  HTML file       - Write to
 *       i)      notsource 1: isn't source, 0: source.
 */
void c_parser_init(FILE *);
void cpp_parser_init(FILE *);
void java_parser_init(FILE *);
void php_parser_init(FILE *);

void
src2html(src, html, notsource)
	char *src;
	char *html;
	int notsource;
{
	char indexlink[128];
	int clex(void), cpplex(void), javalex(void), phplex(void);

	/*
	 * setup lineno format.
	 */
	snprintf(lineno_format, sizeof(lineno_format), "%%%dd ", ncol);

	in  = open_input_file(src);
	out = open_output_file(html);

	if (Fflag)
		snprintf(indexlink, sizeof(indexlink), "../files.%s", normal_suffix);
	else
		snprintf(indexlink, sizeof(indexlink), "../mains.%s", normal_suffix);
	/*
	 * load tags belonging to this file.
	 */
	if (!notsource) {
		anchor_load(src);
	}
        fprintf(out, "%s\n", html_begin);
        fputs(set_header(src), out);
        fprintf(out, "%s\n", body_begin);
	/*
         * print the header
         */
        fputs("<A NAME=TOP><H2>", out);
        fputs(fill_anchor(indexlink, src), out);
	if (cvsweb_url) {
		static STRBUF *sb = NULL;
		char *p;

		if (sb)
			strbuf_reset(sb);
		else
			sb = strbuf_open(0);
		for (p = src; *p; p++) {
			int c = (unsigned char)*p;

			if (c == '/' || isalnum(c))
				strbuf_putc(sb, c);
			else
				strbuf_sprintf(sb, "%%%02x", c);
		}
        	fprintf(out, "%s<A HREF=%s%s", quote_space, cvsweb_url, strbuf_value(sb));
		if (cvsweb_cvsroot)
        		fprintf(out, "?cvsroot=%s", cvsweb_cvsroot);
        	fprintf(out, "><FONT SIZE=-1>[CVS]</FONT></A>\n");
		/* doesn't close string buffer */
	}
	fprintf(out, "</H2>\n");
        fprintf(out, "%s/* ", comment_begin);

	fputs(link_format(anchor_getlinks(0)), out);
	if (show_position)
		fprintf(out, "%s[+1 %s]%s", position_begin, src, position_end);
	fprintf(out, " */%s", comment_end);
	fprintf(out, "\n%s\n", hr);
        /*
         * It is not source file.
         */
        if (notsource) {
		STRBUF *sb = strbuf_open(0);
		char *_;

		fprintf(out, "%s\n", verbatim_begin);
		last_lineno = 0;
		while ((_ = strbuf_fgets(sb, in, STRBUF_NOCRLF)) != NULL) {
			fprintf(out, "<A NAME=%d>", ++last_lineno);
			for (; *_; _++) {
				int c = *_;

				if (c == '&')
					fputs(quote_amp, out);
				else if (c == '<')
					fputs(quote_little, out);
				else if (c == '>')
					fputs(quote_great, out);
				else
					fputc(c, out);
			}
			fputc('\n', out);
		}
		fprintf(out, "%s\n", verbatim_end);
        }
	/*
	 * It's source code.
	 */
	else {
		char *basename;
		struct data *incref;
		struct anchor *ancref;
		static STRBUF *define_index = NULL;

                /*
                 * INCLUDED FROM index.
                 */
		basename = locatestring(src, "/", MATCH_LAST);;
		if (basename)
			basename++;
		else
			basename = src;
		incref = get_included(basename);
		if (incref) {
			fputs("<H2><A HREF=", out);
			if (incref->count > 1) {
				fprintf(out, "../%s/%d.%s", INCREFS, incref->id, HTML);
				fprintf(out, " TITLE='%s'>", tooltip('I', -1, itoa(incref->count)));
			} else {
				char *lno, *filename, *save;
				char *p = strbuf_value(incref->contents);

				lno = p;
				while (*p != ' ')
					p++;
				save = p;
				*p++ = '\0';
				filename = p;
				if (filename[0] == '.' && filename[1] == '/')
					filename += 2;
				fprintf(out, "%s#%s", path2url(filename), lno);
				fprintf(out, " TITLE='%s'>", tooltip('I', atoi(lno), filename));
				*save = ' ';
			}
			fprintf(out, "%s</A></H2>\n", title_included_from);
			fprintf(out, "%s\n", hr);
		}
		/*
		 * DEFINITIONS index.
		 */
		if (define_index)
			strbuf_reset(define_index);
		else
			define_index = strbuf_open(0);
		for (ancref = anchor_first(); ancref; ancref = anchor_next()) {
			if (ancref->type == 'D') {
				strbuf_sprintf(define_index, "<LI><A HREF=#%d TITLE=\"%s\">%s</A>\n",
					ancref->lineno,
					tooltip('R', ancref->lineno, NULL),
					gettag(ancref));
			}
		}
		if (strbuf_getlen(define_index) > 0) {
			fprintf(out, "<H2>%s</H2>\n", title_define_index);
			fputs("This source file includes following definitions.\n", out);
			fputs("<OL>\n", out);
			fputs(strbuf_value(define_index), out);
			fputs("</OL>\n", out);
			fprintf(out, "%s\n", hr);
		}
		/*
		 * print source code
		 */
        	fprintf(out, "%s\n", verbatim_begin);
		if (locatestring(src, ".java", MATCH_AT_LAST)) {
			java_parser_init(in);
			while (javalex())
				;
		} else if (locatestring(src, ".php", MATCH_AT_LAST) ||
			locatestring(src, ".php3", MATCH_AT_LAST) ||
			locatestring(src, ".phtml", MATCH_AT_LAST)) {
			php_parser_init(in);
			while (phplex())
				;
		} else if (locatestring(src, ".h", MATCH_AT_LAST) ||
			locatestring(src, ".c++", MATCH_AT_LAST) ||
			locatestring(src, ".cc", MATCH_AT_LAST) ||
			locatestring(src, ".cpp", MATCH_AT_LAST) ||
			locatestring(src, ".cxx", MATCH_AT_LAST) ||
			locatestring(src, ".hxx", MATCH_AT_LAST) ||
			locatestring(src, ".hpp", MATCH_AT_LAST) ||
			locatestring(src, ".C", MATCH_AT_LAST) ||
			locatestring(src, ".H", MATCH_AT_LAST)) {
			cpp_parser_init(in);
			while (cpplex())
				;
		} else {
			c_parser_init(in);
			while (clex())
				;
		}
        	fprintf(out, "%s\n", verbatim_end);
	}
	fprintf(out, "%s\n", hr);
	fputs("<A NAME=BOTTOM>\n", out);
        fprintf(out, "%s/* ", comment_begin);
	fputs(link_format(anchor_getlinks(-1)), out);
	if (show_position)
		fprintf(out, "%s[+%d %s]%s", position_begin, last_lineno, src, position_end);
        fprintf(out, " */%s\n", comment_end);
        fprintf(out, "%s\n", body_end);
        fprintf(out, "%s\n", html_end);

	if (!notsource)
		anchor_unload();
	close_output_file();
	close_input_file();
}
