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
 * Tag definitions
 *
 * Htags generates HTML tag by default.
 */
char *html_begin	= "<html>";
char *html_end		= "</html>";
char *html_head_begin	= "<head>";
char *html_head_end	= "</head>";
char *html_title_begin	= "<title>";
char *html_title_end	= "</title>";
char *body_begin	= "<body>";
char *body_end		= "</body>";
char *title_begin	= "<h1><font color='#cc0000'>";
char *title_end		= "</font></h1>";
char *header_begin	= "<h2>";
char *header_end	= "</h2>";
char *cvslink_begin	= "<font size='-1'>";
char *cvslink_end	= "</font>";
char *caution_begin	= "<center>\n<blockquote>";
char *caution_end	= "</blockquote>\n</center>";
char *list_begin	= "<ol>";
char *list_end		= "</ol>";
char *item_begin	= "<li>";
char *item_end		= "";
char *define_list_begin = "<dl>";
char *define_list_end   = "</dl>";
char *define_term_begin = "<dt>";
char *define_term_end   = "";
char *define_desc_begin	= "<dd>";
char *define_desc_end	= "";
char *table_begin	= "<table>";
char *table_end		= "</table>";
char *comment_begin	= "<i><font color='green'>";
char *comment_end	= "</font></i>";
char *sharp_begin	= "<font color='darkred'>";
char *sharp_end		= "</font>";
char *brace_begin	= "<font color='blue'>";
char *brace_end		= "</font>";
char *verbatim_begin	= "<pre>";
char *verbatim_end	= "</pre>";
char *reserved_begin	= "<b>";
char *reserved_end	= "</b>";
char *position_begin	= "<font color='gray'>";
char *position_end	= "</font>";
char *warned_line_begin = "<span style='background-color:yellow'>";
char *warned_line_end   = "</span>";
char *error_begin	= "<h1><font color='#cc0000'>";
char *error_end		= "</font></h1>";
char *message_begin	= "<h3>";
char *message_end	= "</h3>";
char *string_begin	= "<u>";
char *string_end	= "</u>";
char *quote_great	= "&gt;";
char *quote_little	= "&lt;";
char *quote_amp		= "&amp;";
char *quote_space	= "&nbsp;";
char *hr		= "<hr>";
char *br		= "<br>";
char *empty_element	= "";
char *noframes_begin	= "<noframes>";
char *noframes_end	= "</noframes>";

/*
 * print string and new line.
 *
 * This function is a replacement of fprintf(op, "%s\n", s) in htags.
 */
int
fputs_nl(s, op)
	const char *s;
	FILE *op;
{
	fputs(s, op);
	putc('\n', op);
	return 0;
}
/*
 * XHTML support.
 *
 * If the --xhtml option is specified then we take 'XHTML 1.1'.
 * If both of the --xhtml and the --frame option are specified
 * then we take 'XHTML 1.0 Frameset'.
 * We define each style for the tags in 'style.css' in this directory.
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
	noframes_begin	= "<noframes>";
	noframes_end	= "</noframes>";
}
/*
 * Generate upper directory.
 */
char *
upperdir(dir)
	const char *dir;
{
	STATIC_STRBUF(sb);

	strbuf_clear(sb);
	strbuf_sprintf(sb, "../%s", dir);
	return strbuf_value(sb);
}
/*
 * Generate beginning of page
 *
 *	i)	title	title of this page
 *	i)	subdir	1: this page is in subdirectory
 */
char *
gen_page_begin(title, subdir)
	const char *title;
	int subdir;
{
	STATIC_STRBUF(sb);
	char *dir = subdir ? "../" : "";

	strbuf_clear(sb);
	if (enable_xhtml) {
		strbuf_puts_nl(sb, "<?xml version='1.0' encoding='ISO-8859-1'?>");
		strbuf_sprintf(sb, "<?xml-stylesheet type='text/css' href='%sstyle.css'?>\n", dir);
		if (Fflag)
			strbuf_puts_nl(sb, "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Frameset//EN' 'http:://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd'>");
		else
			strbuf_puts_nl(sb, "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.1//EN' 'http:://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd'>");
	}
	strbuf_puts_nl(sb, html_begin);
	strbuf_puts_nl(sb, html_head_begin);
	strbuf_puts(sb, html_title_begin);
	strbuf_puts(sb, title);
	strbuf_puts_nl(sb, html_title_end);
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
	STATIC_STRBUF(sb);
	char *dir = (where == PARENT) ? "../icons" : "icons";

	strbuf_clear(sb);
	if (enable_xhtml)
		strbuf_sprintf(sb, "<img class='icon' src='%s/%s.%s' alt='[%s]'%s>",
			dir, file, icon_suffix, alt, empty_element);
	else
		strbuf_sprintf(sb, "<img src='%s/%s.%s' alt='[%s]' %s%s>",
			dir, file, icon_suffix, alt, icon_spec, empty_element);
	return strbuf_value(sb);
}
/*
 * Generate name tag.
 */
char *
gen_name_number(number)
	int number;
{
	STATIC_STRBUF(sb);
	char *id = enable_xhtml ? "id" : "name";

	strbuf_clear(sb);
	strbuf_sprintf(sb, "<a %s='%d'%s>", id, number, empty_element);
	return strbuf_value(sb);
}
/*
 * Generate name tag.
 */
char *
gen_name_string(name)
	const char *name;
{
	STATIC_STRBUF(sb);
	char *id = enable_xhtml ? "id" : "name";

	strbuf_clear(sb);
	strbuf_sprintf(sb, "<a %s='%s'%s>", id, name, empty_element);
	return strbuf_value(sb);
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
	STATIC_STRBUF(sb);

	strbuf_clear(sb);
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
	STATIC_STRBUF(sb);

	if (strbuf_empty(sb)) {
		strbuf_clear(sb);
		if (table_list) {
			if (enable_xhtml) {
				strbuf_sprintf(sb, "%s\n%s%s%s%s",
					table_begin, 
					"<tr><th class='tag'>tag</th>",
					"<th class='line'>line</th>",
					"<th class='file'>file</th>",
					"<th class='code'>source code</th></tr>");
			} else {
				strbuf_sprintf(sb, "%s\n%s%s%s%s",
					table_begin, 
					"<tr><th nowrap align='left'>tag</th>",
					"<th nowrap align='right'>line</th>",
					"<th nowrap align='center'>file</th>",
					"<th nowrap align='left'>source code</th></tr>");
			}
		} else {
			strbuf_puts(sb, verbatim_begin);
		}
	}
	return strbuf_value(sb);
}
/*
 * Generate list body.
 *
 * s must be choped.
 */
char *
gen_list_body(srcdir, string)
	const char *srcdir;
	char *string;
{
	STATIC_STRBUF(sb);
	char *p, *filename, *fid;
	SPLIT ptable;

	strbuf_clear(sb);
	if (split(string, 4, &ptable) < 4) {
		recover(&ptable);
		die("too small number of parts in list_body().\n'%s'", string);
	}
	filename = ptable.part[2].start + 2;		/* remove './' */
	fid = path2fid(filename);
	if (table_list) {
		if (enable_xhtml) {
			strbuf_puts(sb, "<tr><td class='tag'>");
			strbuf_puts(sb, gen_href_begin(srcdir, fid, HTML, ptable.part[PART_LNO].start));
			strbuf_puts(sb, ptable.part[PART_TAG].start);
			strbuf_puts(sb, gen_href_end());
			strbuf_sprintf(sb, "</td><td class='line'>%s</td><td class='file'>%s</td><td class='code'>",
				ptable.part[PART_LNO].start, filename);
		} else {
			strbuf_puts(sb, "<tr><td nowrap>");
			strbuf_puts(sb, gen_href_begin(srcdir, fid, HTML, ptable.part[PART_LNO].start));
			strbuf_puts(sb, ptable.part[PART_TAG].start);
			strbuf_puts(sb, gen_href_end());
			strbuf_sprintf(sb, "</td><td nowrap align='right'>%s</td><td nowrap align='left'>%s</td><td nowrap>",
				ptable.part[PART_LNO].start, filename);
		}

		for (p = ptable.part[PART_LINE].start; *p; p++) {
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

		strbuf_puts(sb, gen_href_begin(srcdir, fid, HTML, ptable.part[PART_LNO].start));
		strbuf_puts(sb, ptable.part[PART_TAG].start);
		strbuf_puts(sb, gen_href_end());
		p = string + strlen(ptable.part[PART_TAG].start);
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
	return strbuf_value(sb);
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
 *
 *	i)	align	right,left,center
 */
char *
gen_div_begin(align)
	const char *align;
{
	STATIC_STRBUF(sb);

	if (strbuf_empty(sb)) {
		strbuf_clear(sb);
		if (align) {
			/*
			 * In XHTML, alignment is defined in the file 'style.css'.
			 */
			if (enable_xhtml)
				strbuf_sprintf(sb, "<div class='%s'>", align);
			else
				strbuf_sprintf(sb, "<div align='%s'>", align);
		} else {
			strbuf_puts(sb, "<div>");
		}
	}
	return strbuf_value(sb);
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
 * Generate beginning of form
 *
 *	i)	target	target
 */
char *
gen_form_begin(target)
	const char *target;
{
	STATIC_STRBUF(sb);

	strbuf_clear(sb);
	strbuf_sprintf(sb, "<form method='get' action='%s'", action);
	if (target)
		strbuf_sprintf(sb, " target='%s'", target);
	strbuf_sprintf(sb, "%s>", empty_element);
	return strbuf_value(sb);
}
/*
 * Generate end of form
 */
char *
gen_form_end()
{
	return "</form>";
}
/*
 * Generate input tag
 */
char *
gen_input(name, value, type)
	const char *name;
	const char *value;
	const char *type;
{
	return gen_input_with_title_checked(name, value, type, 0, NULL);
}
/*
 * Generate input radiobox tag
 */
char *
gen_input_radio(name, value, checked, title)
	const char *name;
	const char *value;
	int checked;
	const char *title;
{
	return gen_input_with_title_checked(name, value, "radio", checked, title);
}
/*
 * Generate input checkbox tag
 */
char *
gen_input_checkbox(name, value, title)
	const char *name;
	const char *value;
	const char *title;
{
	return gen_input_with_title_checked(name, value, "checkbox", 0, title);
}
/*
 * Generate input radio tag
 */
char *
gen_input_with_title_checked(name, value, type, checked, title)
	const char *name;
	const char *value;
	const char *type;
	int checked;
	const char *title;
{
	STATIC_STRBUF(sb);

	strbuf_clear(sb);
	strbuf_puts(sb, "<input");
	if (type)
		strbuf_sprintf(sb, " type='%s'", type);
	if (name)
		strbuf_sprintf(sb, " name='%s'", name);
	if (value)
		strbuf_sprintf(sb, " value='%s'", value);
	if (checked)
		strbuf_puts(sb, " checked");
	if (title)
		strbuf_sprintf(sb, " title='%s'", title);
	strbuf_sprintf(sb, "%s>", empty_element);
	return strbuf_value(sb);
}
/*
 * Generate beginning of frameset
 *
 *	i)	target	target
 */
char *
gen_frameset_begin(contents)
	const char *contents;
{
	STATIC_STRBUF(sb);

	strbuf_clear(sb);
	strbuf_sprintf(sb, "<frameset %s%s>", contents, empty_element);
	return strbuf_value(sb);
}
/*
 * Generate end of frameset
 */
char *
gen_frameset_end()
{
	return "</frameset>";
}
/*
 * Generate beginning of frame
 *
 *	i)	target	target
 */
char *
gen_frame(name, src)
	const char *name;
	const char *src;
{
	STATIC_STRBUF(sb);

	strbuf_clear(sb);
	strbuf_sprintf(sb, "<frame name='%s' id='%s' src='%s'%s>", name, name, src, empty_element);
	return strbuf_value(sb);
}
