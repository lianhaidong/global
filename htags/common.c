/*
 * Copyright (c) 2004, 2005 Tama Communications Corporation
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
#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <ctype.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#include "global.h"
#include "common.h"
#include "path2url.h"
#include "htags.h"

/*
 * XHTML support.
 *
 * If the --xhtml option is specified then we take 'XHTML 1.1'.
 * If both of the --xhtml and the --frame option are specified
 * then we take 'XHTML 1.0 Frameset'.
 * We define each style for the tags in 'style.css' in this directory.
 */
/*----------------------------------------------------------------------*/
/* Tag definitions							*/
/*----------------------------------------------------------------------*/
char *html_begin;
char *html_end;
char *html_head_begin;
char *html_head_end;
char *html_title_begin;
char *html_title_end;
char *body_begin;
char *body_end;
char *title_begin;
char *title_end;
char *header_begin;
char *header_end;
char *cvslink_begin;
char *cvslink_end;
char *caution_begin;
char *caution_end;
char *list_begin;
char *list_end;
char *item_begin;
char *item_end;
char *define_list_begin;
char *define_list_end;
char *define_term_begin;
char *define_term_end;
char *define_desc_begin;
char *define_desc_end;
char *table_begin;
char *table_end;
char *comment_begin;
char *comment_end;
char *sharp_begin;
char *sharp_end;
char *brace_begin;
char *brace_end;
char *verbatim_begin;
char *verbatim_end;
char *reserved_begin;
char *reserved_end;
char *position_begin;
char *position_end;
char *warned_line_begin;
char *warned_line_end;
char *error_begin;
char *error_end;
char *message_begin;
char *message_end;
char *string_begin;
char *string_end;
char *quote_great;
char *quote_little;
char *quote_amp;
char *quote_space;
char *hr;
char *br;
char *empty_element;
/*
 * Set up HTML tags.
 */
void
setup_html()
{
	html_begin	= "<html>";
	html_end	= "</html>";
	html_head_begin	= "<head>";
	html_head_end	= "</head>";
	html_title_begin= "<title>";
	html_title_end	= "</title>";
	body_begin	= "<body>";
	body_end	= "</body>";
	title_begin	= "<h1><font color='#cc0000'>";
	title_end	= "</font></h1>";
	header_begin	= "<h2>";
	header_end	= "</h2>";
	cvslink_begin	= "<font size='-1'>";
	cvslink_end	= "</font>";
	caution_begin	= "<center>\n<blockquote>";
	caution_end	= "</blockquote>\n</center>";
	list_begin	= "<ol>";
	list_end	= "</ol>";
	item_begin	= "<li>";
	item_end	= "";
	define_list_begin = "<dl>";
	define_list_end   = "</dl>";
	define_term_begin = "<dt>";
	define_term_end   = "";
	define_desc_begin  = "<dd>";
	define_desc_end    = "";
	table_begin	= "<table>";
	table_end	= "</table>";
	comment_begin	= "<i><font color='green'>";
	comment_end	= "</font></i>";
	sharp_begin	= "<font color='darkred'>";
	sharp_end	= "</font>";
	brace_begin	= "<font color='blue'>";
	brace_end	= "</font>";
	verbatim_begin	= "<pre>";
	verbatim_end	= "</pre>";
	reserved_begin	= "<b>";
	reserved_end	= "</b>";
	position_begin	= "<font color='gray'>";
	position_end	= "</font>";
	warned_line_begin = "<span style='background-color:yellow'>";
	warned_line_end   = "</span>";
	error_begin     = "<h1><font color='#cc0000'>";
	error_end       = "</font></h1>";
	message_begin   = "<h3>";
	message_end     = "</h3>";
	string_begin	= "<u>";
	string_end	= "</u>";
	quote_great	= "&gt;";
	quote_little	= "&lt;";
	quote_amp	= "&amp;";
	quote_space	= "&nbsp;";
	hr              = "<hr>";
	br              = "<br>";
	empty_element	= "";
}
/*
 * Set up XHTML tags.
 */
void
setup_xhtml()
{
	html_begin	= "<html xmlns='http://www.w3.org/1999/xhtml'>";
	html_end	= "</html>";
	html_head_begin	= "<head>";
	html_head_end	= "</head>";
	html_title_begin= "<title>";
	html_title_end	= "</title>";
	body_begin	= "<body>";
	body_end	= "</body>";
	title_begin	= "<h1 class='title'>";
	title_end	= "</h1>";
	header_begin	= "<h2 class='header'>";
	header_end	= "</h2>";
	cvslink_begin	= "<span class='cvs'>";
	cvslink_end	= "</span>";
	caution_begin	= "<div class='caution'>";
	caution_end	= "</div>";
	list_begin	= "<ol>";
	list_end	= "</ol>";
	item_begin	= "<li>";
	item_end	= "</li>";
	define_list_begin = "<dl>";
	define_list_end   = "</dl>";
	define_term_begin = "<dt>";
	define_term_end   = "</dt>";
	define_desc_begin  = "<dd>";
	define_desc_end    = "</dd>";
	table_begin	= "<table>";
	table_end	= "</table>";
	comment_begin	= "<em class='comment'>";
	comment_end	= "</em>";
	sharp_begin	= "<em class='sharp'>";
	sharp_end	= "</em>";
	brace_begin	= "<em class='brace'>";
	brace_end	= "</em>";
	verbatim_begin	= "<pre>";
	verbatim_end	= "</pre>";
	reserved_begin	= "<strong class='reserved'>";
	reserved_end	= "</strong>";
	position_begin	= "<em class='position'>";
	position_end	= "</em>";
	warned_line_begin = "<em class='warned'>";
	warned_line_end   = "</em>";
	error_begin     = "<h2 class='error'>";
	error_end       = "</h2>";
	message_begin   = "<h3 class='message'>";
	message_end     = "</h3>";
	string_begin	= "<em class='string'>";
	string_end	= "</em>";
	quote_great	= "&gt;";
	quote_little	= "&lt;";
	quote_amp	= "&amp;";
	quote_space	= "&nbsp;";
	hr              = "<hr />";
	br              = "<br />";
	empty_element	= " /";
}
/*
 * Generate upper directory.
 */
char *
upperdir(dir)
	const char *dir;
{
	static char path[MAXPATHLEN];
	snprintf(path, sizeof(path), "../%s", dir);
	return path;
}
/*
 * Generate beginning of page
 *
 *	i)	title	title of this page
 *	i)	subdir	1: this page is in subdirectory
 */
char *
gen_page_begin(title, subdir)
	char *title;
	int subdir;
{
	static STRBUF *sb = NULL;
	char *dir = subdir ? "../" : "";

	if (sb == NULL)
		sb = strbuf_open(0);
	else
		strbuf_reset(sb);
	if (enable_xhtml) {
		strbuf_puts(sb, "<?xml version='1.0' encoding='ISO-8859-1'?>\n");
		strbuf_sprintf(sb, "<?xml-stylesheet type='text/css' href='%sstyle.css'?>\n", dir);
		if (Fflag)
			strbuf_puts(sb, "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Frameset//EN' 'http:://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd'>\n");
		else
			strbuf_puts(sb, "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.1//EN' 'http:://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd'>\n");
	}
	strbuf_sprintf(sb, "%s\n", html_begin);
	strbuf_sprintf(sb, "%s\n", html_head_begin);
	strbuf_puts(sb, html_title_begin);
	strbuf_puts(sb, title);
	strbuf_sprintf(sb, "%s\n", html_title_end);
	strbuf_sprintf(sb, "<meta name='robots' content='noindex,nofollow'%s>\n", empty_element);
	strbuf_sprintf(sb, "<meta name='generator' content='GLOBAL-%s'%s>\n", get_version(), empty_element);
	if (enable_xhtml) {
		strbuf_sprintf(sb, "<meta http-equiv='Content-Style-Type' content='text/css'%s>\n", empty_element);
		strbuf_sprintf(sb, "<link rel='stylesheet' type='text/css' href='%sstyle.css'%s>\n", dir, empty_element);
	}
	if (style_sheet)
		strbuf_puts(sb, style_sheet);
	strbuf_puts(sb, html_head_end);
	return strbuf_value(sb);
}
/*
 * Generate end of page
 */
char *
gen_page_end()
{
	return html_end;
}

/*
 * Generate image tag.
 *
 *	i)	where	Where is the icon directory?
 *			CURRENT: current directory
 *			PARENT: parent directory
 *	i)	file	icon file without suffix.
 *	i)	alt	alt string
 */
char *
gen_image(where, file, alt)
	int where;
	const char *file;
	const char *alt;
{
	static char buf[1024];
	char *dir = (where == PARENT) ? "../icons" : "icons";
		
	if (enable_xhtml)
		snprintf(buf, sizeof(buf), "<img class='icon' src='%s/%s.%s' alt='[%s]'%s>",
			dir, file, icon_suffix, alt, empty_element);
	else
		snprintf(buf, sizeof(buf), "<img src='%s/%s.%s' alt='[%s]' %s%s>",
			dir, file, icon_suffix, alt, icon_spec, empty_element);
	return buf;
}
/*
 * Generate name tag.
 */
char *
gen_name_number(number)
	int number;
{
	static char buf[128];
	char *id = enable_xhtml ? "id" : "name";
	snprintf(buf, sizeof(buf), "<a %s='%d'%s>", id, number, empty_element);
	return buf;
}
/*
 * Generate name tag.
 */
char *
gen_name_string(name)
	const char *name;
{
	static char buf[128];
	char *id = enable_xhtml ? "id" : "name";
	snprintf(buf, sizeof(buf), "<a %s='%s'%s>", id, name, empty_element);
	return buf;
}
/*
 * Generate anchor begin tag.
 * (complete format)
 *
 *	i)	dir	directory
 *	i)	file	file
 *	i)	suffix	suffix
 *	i)	key	key
 *	i)	title	title='xxx'
 *	i)	target	target='xxx'
 *	r)		generated anchor tag
 */
char *
gen_href_begin_with_title_target(dir, file, suffix, key, title, target)
	const char *dir;
	const char *file;
	const char *suffix;
	const char *key;
	const char *title;
	const char *target;
{
	static STRBUF *sb = NULL;

	if (sb == NULL)
		sb = strbuf_open(0);
	else
		strbuf_reset(sb);
	/*
	 * Construct URL.
	 * href='dir/file.suffix#key'
	 */
	strbuf_puts(sb, "<a href='");
	if (file) {
		if (dir) {
			strbuf_puts(sb, dir);
			strbuf_putc(sb, '/');
		}
		strbuf_puts(sb, file);
		if (suffix) {
			strbuf_putc(sb, '.');
			strbuf_puts(sb, suffix);
		}
	}
	if (key) {
		strbuf_putc(sb, '#');
		strbuf_puts(sb, key);
	}
	strbuf_putc(sb, '\'');
	if (target)
		strbuf_sprintf(sb, " target='%s'", target);
	if (title)
		strbuf_sprintf(sb, " title='%s'", title);
	strbuf_putc(sb, '>');
	return strbuf_value(sb);
}
/*
 * Generate simple anchor begin tag.
 */
char *
gen_href_begin_simple(file)
	const char *file;
{
	return gen_href_begin_with_title_target(NULL, file, NULL, NULL, NULL, NULL);
}
/*
 * Generate anchor begin tag without title and target.
 */
char *
gen_href_begin(dir, file, suffix, key)
	const char *dir;
	const char *file;
	const char *suffix;
	const char *key;
{
	return gen_href_begin_with_title_target(dir, file, suffix, key, NULL, NULL);
}
/*
 * Generate anchor begin tag without target.
 */
char *
gen_href_begin_with_title(dir, file, suffix, key, title)
	const char *dir;
	const char *file;
	const char *suffix;
	const char *key;
	const char *title;
{
	return gen_href_begin_with_title_target(dir, file, suffix, key, title, NULL);
}
/*
 * Generate anchor end tag.
 */
char *
gen_href_end()
{
	return "</a>";
}
/*
 * Generate list begin tag.
 */
char *
gen_list_begin()
{
	static char buf[1024];

	if (table_list) {
		if (enable_xhtml) {
			snprintf(buf, sizeof(buf), "%s\n%s%s%s%s",
				table_begin, 
				"<tr><th class='tag'>tag</th>",
				"<th class='line'>line</th>",
				"<th class='file'>file</th>",
				"<th class='code'>source code</th></tr>");
		} else {
			snprintf(buf, sizeof(buf), "%s\n%s%s%s%s",
				table_begin, 
				"<tr><th nowrap align='left'>tag</th>",
				"<th nowrap align='right'>line</th>",
				"<th nowrap align='center'>file</th>",
				"<th nowrap align='left'>source code</th></tr>");
		}
	} else {
		strlimcpy(buf, verbatim_begin, sizeof(buf));
	}
	return buf;
}
/*
 * Generate list body.
 *
 * s must be choped.
 */
char *
gen_list_body(srcdir, string)
	char *srcdir;
	char *string;
{
	static STRBUF *sb = NULL;
	char *name, *lno, *filename, *line, *fid;
	char *p;
	SPLIT ptable;

	if (sb == NULL)
		sb = strbuf_open(0);
	else
		strbuf_reset(sb);
	if (split(string, 4, &ptable) < 4) {
		recover(&ptable);
		die("too small number of parts in list_body().\n'%s'", string);
	}
	name = ptable.part[0].start;
	lno = ptable.part[1].start;
	filename = ptable.part[2].start;
	line = ptable.part[3].start;
	filename += 2;				/* remove './' */
	fid = path2fid(filename);

	if (table_list) {
		if (enable_xhtml) {
			strbuf_puts(sb, "<tr><td class='tag'>");
			strbuf_puts(sb, gen_href_begin(srcdir, fid, HTML, lno));
			strbuf_puts(sb, name);
			strbuf_puts(sb, gen_href_end());
			strbuf_sprintf(sb, "</td><td class='line'>%s</td><td class='file'>%s</td><td class='code'>",
				lno, filename);
		} else {
			strbuf_puts(sb, "<tr><td nowrap>");
			strbuf_puts(sb, gen_href_begin(srcdir, fid, HTML, lno));
			strbuf_puts(sb, name);
			strbuf_puts(sb, gen_href_end());
			strbuf_sprintf(sb, "</td><td nowrap align='right'>%s</td><td nowrap align='left'>%s</td><td nowrap>",
				lno, filename);
		}

		for (p = line; *p; p++) {
			unsigned char c = *p;

			if (c == '&')
				strbuf_puts(sb, quote_amp);
			else if (c == '<')
				strbuf_puts(sb, quote_little);
			else if (c == '>')
				strbuf_puts(sb, quote_great);
			else if (c == ' ')
				strbuf_puts(sb, quote_space);
			else if (c == '\t') {
				strbuf_puts(sb, quote_space);
				strbuf_puts(sb, quote_space);
			} else
				strbuf_putc(sb, c);
		}
		strbuf_puts(sb, "</td></tr>");
		recover(&ptable);
	} else {
		int done = 0;

		strbuf_puts(sb, gen_href_begin(srcdir, fid, HTML, lno));
		strbuf_puts(sb, name);
		strbuf_puts(sb, gen_href_end());
		p = string + strlen(name);
		recover(&ptable);

		for (; *p; p++) {
			unsigned char c = *p;

			/* ignore "./" in path name */
			if (!done && c == '.' && *(p + 1) == '/') {
				p++;
				done = 1;
			} else if (c == '&')
				strbuf_puts(sb, quote_amp);
			else if (c == '<')
				strbuf_puts(sb, quote_little);
			else if (c == '>')
				strbuf_puts(sb, quote_great);
			else
				strbuf_putc(sb, c);
		}
	}
	p = strbuf_value(sb);
	return p;
}
/*
 * Generate list end tag.
 */
char *
gen_list_end()
{
	return table_list ? table_end : verbatim_end;
}
/*
 * Generate div begin tag.
 */
char *
gen_div_begin(align)
	const char *align;
{
	if (align) {
		static char buf[32];
		snprintf(buf, sizeof(buf), "<div align='%s'>", align);
		return buf;
	}
	return "<div>";
}
/*
 * Generate div end tag.
 */
char *
gen_div_end()
{
	return "</div>";
}

/*
 * Decide whether or not the path is binary file.
 *
 *	i)	path
 *	r)	0: is not binary, 1: is binary
 */
int
is_binary(path)
	char *path;
{
	int ip;
	char buf[32];
	int i, c, size;

	ip = open(path, 0);
	if (ip < 0)
		die("cannot open file '%s' in read mode.", path);
	size = read(ip, buf, sizeof(buf));
	close(ip);
	if (size < 0)
		return 1;
	if (!strncmp(buf, "!<arch>", 7))
		return 1;
	for (i = 0; i < size; i++) {
		c = (unsigned char)buf[i];
		if (c == 0 || c > 127)
			return 1;
	}
	return 0;
}
/*
 * Encode URL.
 *
 *	i)	url	URL
 *	r)		encoded URL
 */
char *
encode(url)
        char *url;
{
	static STRBUF *sb = NULL;
        char *p;

	if (sb)
		strbuf_reset(sb);
	else
		sb = strbuf_open(0);
        for (p = url; *p; p++) {
		int c = (unsigned char)*p;

                if (isalnum(c))
			strbuf_putc(sb, c);
		else
			strbuf_sprintf(sb, "%%%02x", c);
        }

	return strbuf_value(sb);
}
