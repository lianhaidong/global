/*
 * Copyright (c) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005
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
#include <ctype.h>

#include "global.h"
#include "anchor.h"
#include "cache.h"
#include "common.h"
#include "incop.h"
#include "path2url.h"
#include "htags.h"

/*----------------------------------------------------------------------*/
/* Parser switch							*/
/*----------------------------------------------------------------------*/
/*
 * This is the linkage section of each parsers.
 * If you want to support new language, you must define two procedures:
 *	1. Initializing procedure.
 *		Called once first with an input file descripter.
 *	2. Executing procedure.
 *		Called repeatedly until returning EOF.
 *		It should read from above descripter and write HTML
 *		using output procedures in this module.
 */
struct lang_entry {
	char *lang_name;
	void (*init_proc)(FILE *);		/* initializing procedure */
	int (*exec_proc)(void);			/* executing procedure */
};

/* initializing procedures */
void c_parser_init(FILE *);
void yacc_parser_init(FILE *);
void cpp_parser_init(FILE *);
void java_parser_init(FILE *);
void php_parser_init(FILE *);
void asm_parser_init(FILE *);

/* executing procedures */
int clex(void);
int cpplex(void);
int javalex(void);
int phplex(void);
int asmlex(void);

/*
 * The first entry is default language.
 */
struct lang_entry lang_switch[] = {
	/* lang_name	init_proc 		exec_proc */
	{"c",		c_parser_init,		clex},		/* DEFAULT */
	{"yacc",	yacc_parser_init,	clex},
	{"cpp",		cpp_parser_init,	cpplex},
	{"java",	java_parser_init,	javalex},
	{"php",		php_parser_init,	phplex},
	{"asm",		asm_parser_init,	asmlex}
};
#define DEFAULT_ENTRY &lang_switch[0]

/*
 * get language entry.
 *
 *	i)	lang	language name (NULL means 'not specified'.)
 *	r)		language entry
 */
static struct lang_entry *
get_lang_entry(lang)
	const char *lang;
{
	int i, size = sizeof(lang_switch) / sizeof(struct lang_entry);

	/*
	 * if language not specified, it assumes default language.
	 */
	if (lang == NULL)
		return DEFAULT_ENTRY;
	for (i = 0; i < size; i++)
		if (!strcmp(lang, lang_switch[i].lang_name))
			return &lang_switch[i];
	/*
	 * if specified language not found, it assumes default language.
	 */
	return DEFAULT_ENTRY;
}
/*----------------------------------------------------------------------*/
/* Input/Output								*/
/*----------------------------------------------------------------------*/
/*
 * Input/Output descriptor.
 */
static FILE *out;
static FILE *in;

static STRBUF *outbuf;
static char *curpfile;
static int warned;
static int last_lineno;

/*
 * Open source file.
 *
 *	i)	file	source file name
 *	r)		file pointer
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
/*
 * Close source file.
 */
static void
close_input_file(ip)
	FILE *ip;
{
	if (pclose(ip) != 0)
		die("command 'gtags --expand -%d' failed.", tabs);
}
/*
 * Open HTML file.
 *
 *	i)	file	HTML file name
 *	r)		file pointer
 */
static FILE *
open_output_file(file)
	char *file;
{
	char command[MAXFILLEN];
	FILE *op;

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
/*
 * Close HTML file.
 */
static void
close_output_file(op)
	FILE *op;
{
	if (cflag) {
		if (pclose(op) != 0)
			die("command 'gzip -c' failed.");
	} else
		fclose(op);
}
/*
 * Put a character to HTML as is.
 *
 * You should use this function to put a control character.
 */
void
echoc(c)
	int c;
{
        strbuf_putc(outbuf, c);
}
/*
 * Put a string to HTML as is.
 *
 * You should use this function to put a control sequence.
 */
void
echos(s)
	const char *s;
{
        strbuf_puts(outbuf, s);
}

/*----------------------------------------------------------------------*/
/* HTML output								*/
/*----------------------------------------------------------------------*/
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

	strbuf_sprintf(sb, "%sroot%s/", gen_href_begin_simple(root), gen_href_end());
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
			strbuf_puts(sb, gen_href_begin("../files", path2fid(path), HTML, NULL));
			strbuf_puts(sb, unit);
			strbuf_puts(sb, gen_href_end());
			strbuf_putc(sb, '/');
		}
	}
        return strbuf_value(sb);
}

/*
 * link_format: make hypertext from anchor array.
 *
 *	i)	(previous, next, first, last, top, bottom)
 *		-1: top, -2: bottom, other: line number
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
			strbuf_puts(sb, gen_href_begin("..", "mains", normal_suffix, NULL));
		} else if (i == A_HELP) {
			strbuf_puts(sb, gen_href_begin("..", "help", normal_suffix, NULL));
		} else if (ref[i]) {
			char tmp[32], *key = tmp;

			if (ref[i] == -1)
				key = "TOP";
			else if (ref[i] == -2)
				key = "BOTTOM";
			else
				snprintf(tmp, sizeof(tmp), "%d", ref[i]);
			strbuf_puts(sb, gen_href_begin(NULL, NULL, NULL, key));
		}
		if (icon_list) {
			char tmp[MAXPATHLEN];
			snprintf(tmp, sizeof(tmp), "%s%s", (i != A_INDEX && i != A_HELP && ref[i] == 0) ? "n_" : "", icons[i]);
			strbuf_puts(sb, gen_image(PARENT, tmp, label[i]));
		} else {
			strbuf_sprintf(sb, "[%s]", label[i]);
		}
		if (i == A_INDEX || i == A_HELP || ref[i] != 0)
			strbuf_puts(sb, gen_href_end());
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
		/*
		 * About cache record format, please see the comment in cache.c.
		 */
		if (*line == ' ') {
			char tmp[MAXPATHLEN];
			char *id = strmake(++line, " ");
			char *count = locatestring(line, " ", MATCH_FIRST) + 1;
			char *dir, *file, *suffix = NULL;

			if (dynamic) {
				dir = (*action == '/') ? NULL : "..";
				char *s;

				if (db == GTAGS)
					s = "definitions";
				else if (db == GRTAGS)
					s = "reference";
				else
					s = "symbol";
				snprintf(tmp, sizeof(tmp), "%s?pattern=%s&type=%s",
					action, name, s);
				file = tmp;
			} else {
				if (type == 'R')
					dir = upperdir(DEFS);
				else if (type == 'Y')
					dir = upperdir(SYMS);
				else	/* 'D', 'M' or 'T' */
					dir = upperdir(REFS);
				file = id;
				suffix = HTML;
			}
			strbuf_puts(outbuf, gen_href_begin_with_title(dir, file, suffix, NULL, tooltip(type, -1, count)));
			strbuf_puts(outbuf, name);
			strbuf_puts(outbuf, gen_href_end());
		} else {
			char lno[32];
			char *filename;

			strlimcpy(lno, strmake(line, " "), sizeof(lno));
			filename = strmake(locatestring(line, " ", MATCH_FIRST) + 1, " ")
						+ 2;	/* remove './' */
			strbuf_puts(outbuf, gen_href_begin_with_title(upperdir(SRCS), path2fid(filename), HTML, lno, tooltip(type, atoi(lno), filename)));
			strbuf_puts(outbuf, name);
			strbuf_puts(outbuf, gen_href_end());
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
	if (inc->count == 1)
		strbuf_puts(outbuf, gen_href_begin(NULL, path2fid(strbuf_value(inc->contents)), HTML, NULL));
	else {
		char id[32];
		snprintf(id, sizeof(id), "%d", inc->id);
		strbuf_puts(outbuf, gen_href_begin(upperdir(INCS), id, HTML, NULL));
	}
	strbuf_puts(outbuf, path);
	strbuf_puts(outbuf, gen_href_end());
}
/*
 * Put a reserved word. (if, while, ...)
 */
void
put_reserved_word(word)
        char *word;
{
	strbuf_puts(outbuf, reserved_begin);
	strbuf_puts(outbuf, word);
	strbuf_puts(outbuf, reserved_end);
}
/*
 * Put a macro (#define,#undef,...) 
 */
void
put_macro(word)
        char *word;
{
	strbuf_puts(outbuf, sharp_begin);
	strbuf_puts(outbuf, word);
	strbuf_puts(outbuf, sharp_end);
}
/*
 * Print warning message when unkown preprocessing directive is found.
 */
void
unknown_preprocessing_directive(word, lineno)
	char *word;
	int lineno;
{
	word = strtrim(word, TRIM_ALL, NULL);
	warning("unknown preprocessing directive '%s'. [+%d %s]", word, lineno, curpfile);
	if (colorize_warned_line)
		warned = 1;
}
/*
 * Print warning message when unexpected eof.
 */
void
unexpected_eof(lineno)
	int lineno;
{
	warning("unexpected eof. [+%d %s]", lineno, curpfile);
	if (colorize_warned_line)
		warned = 1;
}
/*
 * Print warning message when unknown yacc directive is found.
 */
void
unknown_yacc_directive(word, lineno)
	char *word;
	int lineno;
{
	warning("unknown yacc directive '%s'. [+%d %s]", word, lineno, curpfile);
	if (colorize_warned_line)
		warned = 1;
}
/*
 * Print warning message when unmatched brace is found.
 */
void
missing_left(word, lineno)
	char *word;
	int lineno;
{
	warning("missing left '%s'. [+%d %s]", word, lineno, curpfile);
	if (colorize_warned_line)
		warned = 1;
}
/*
 * Put a character with HTML quoting.
 *
 * If you want to put '<', '>' and '&', you should echoc() instead.
 */
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
/*
 * Put a string with HTML quoting.
 *
 * If you want to put HTML tag itself, you should echoc() instead.
 */
void
put_string(s)
        char *s;
{
	for (; *s; s++)
		put_char(*s);
}
/*
 * Put brace ('{', '}')
 */
void
put_brace(text)
        char *text;
{
	strbuf_puts(outbuf, brace_begin);
	strbuf_puts(outbuf, text);
	strbuf_puts(outbuf, brace_end);
}

/*
 * common procedure for line control.
 */
static char lineno_format[32];
static char *guide = NULL;

/*
 * Begin of line processing.
 */
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
/*
 * End of line processing.
 *
 *	i)	lineno	current line number
 *	gi)	outbuf	HTML line image
 *
 * The outbuf(string buffer) has HTML image of the line.
 * This function flush and clear it.
 */
void
put_end_of_line(lineno)
	int lineno;
{
	fputs(gen_name_number(lineno), out);
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
void
src2html(src, html, notsource)
	char *src;
	char *html;
	int notsource;
{
	char indexlink[128];

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
	fprintf(out, "%s\n", gen_page_begin(src, 1));
	fprintf(out, "%s\n", body_begin);
	/*
         * print the header
         */
	fputs(gen_name_string("TOP"), out);
	fputs(header_begin, out);
        fputs(fill_anchor(indexlink, src), out);
	if (cvsweb_url) {
		static STRBUF *sb = NULL;
		char *p;

		if (sb)
			strbuf_reset(sb);
		else
			sb = strbuf_open(0);
		strbuf_puts(sb, cvsweb_url);
		for (p = src; *p; p++) {
			int c = (unsigned char)*p;

			if (c == '/' || isalnum(c))
				strbuf_putc(sb, c);
			else
				strbuf_sprintf(sb, "%%%02x", c);
		}
		if (cvsweb_cvsroot)
			strbuf_sprintf(sb, "?cvsroot=%s", cvsweb_cvsroot);
		fprintf(out, "%s%s", quote_space, gen_href_begin_simple(strbuf_value(sb)));
		fprintf(out, "%s[CVS]%s", cvslink_begin, cvslink_end);
		fprintf(out, "%s\n", gen_href_end());
		/* doesn't close string buffer */
	}
	fprintf(out, "%s\n", header_end);
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
			fputs(gen_name_number(++last_lineno), out);
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
		strbuf_close(sb);
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
		basename = locatestring(src, "/", MATCH_LAST);
		if (basename)
			basename++;
		else
			basename = src;
		incref = get_included(basename);
		if (incref) {
			char s_id[32];
			char *dir, *file, *suffix, *key, *title;

			fputs(header_begin, out);
			if (incref->count > 1) {
				char s_count[32];

				snprintf(s_count, sizeof(s_count), "%d", incref->count);
				snprintf(s_id, sizeof(s_id), "%d", incref->id);
				dir = upperdir(INCREFS);
				file = s_id;
				suffix = HTML;
				key = NULL;
				title = tooltip('I', -1, s_count);
			} else {
				char *p = strbuf_value(incref->contents);
				char *lno = strmake(p, " ");
				char *filename;

				p = locatestring(p, " ", MATCH_FIRST);
				if (p == NULL)
					die("internal error.(incref->contents)");
				filename = p + 1;
				if (filename[0] == '.' && filename[1] == '/')
					filename += 2;
				dir = NULL;
				file = path2fid(filename);
				suffix = HTML;
				key = lno;
				title = tooltip('I', atoi(lno), filename);
			}
			fputs(gen_href_begin_with_title(dir, file, suffix, key, title), out);
			fputs(title_included_from, out);
			fputs(gen_href_end(), out);
			fprintf(out, "%s\n", header_end);
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
				char tmp[32];
				snprintf(tmp, sizeof(tmp), "%d", ancref->lineno);
				strbuf_puts(define_index, item_begin);
				strbuf_puts(define_index, gen_href_begin_with_title(NULL, NULL, NULL, tmp, tooltip('R', ancref->lineno, NULL)));
				strbuf_puts(define_index, gettag(ancref));
				strbuf_puts(define_index, gen_href_end());
				strbuf_puts(define_index, item_end);
				strbuf_putc(define_index, '\n');
			}
		}
		if (strbuf_getlen(define_index) > 0) {
			fputs(header_begin, out);
			fputs(title_define_index, out);
			fprintf(out, "%s\n", header_end);
			fputs("This source file includes following definitions.\n", out);
			fprintf(out, "%s\n", list_begin);
			fputs(strbuf_value(define_index), out);
			fprintf(out, "%s\n", list_end);
			fprintf(out, "%s\n", hr);
		}
		/*
		 * print source code
		 */
		fprintf(out, "%s\n", verbatim_begin);
		{
			const char *suffix = locatestring(src, ".", MATCH_LAST);
			const char *lang = NULL;
			struct lang_entry *ent;

			/*
			 * Decide language.
			 */
			if (suffix)
				lang = decide_lang(suffix);
			/*
			 * Select parser.
			 * If lang == NULL then default parser is selected.
			 */
			ent = get_lang_entry(lang);
			/*
			 * Initialize parser.
			 */
			ent->init_proc(in);
			/*
			 * Execute parser.
			 * Exec_proc() is called repeatedly until returning EOF.
			 */
			while (ent->exec_proc())
				;
		}
        	fprintf(out, "%s\n", verbatim_end);
	}
	fprintf(out, "%s\n", hr);
	fprintf(out, "%s\n", gen_name_string("BOTTOM"));
        fprintf(out, "%s/* ", comment_begin);
	fputs(link_format(anchor_getlinks(-1)), out);
	if (show_position)
		fprintf(out, "%s[+%d %s]%s", position_begin, last_lineno, src, position_end);
        fprintf(out, " */%s\n", comment_end);
	fprintf(out, "%s\n", body_end);
	fprintf(out, "%s\n", gen_page_end());
	if (!notsource)
		anchor_unload();
	close_output_file(out);
	close_input_file(in);
}
