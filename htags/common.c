/*
 * Copyright (c) 2004 Tama Communications Corporation
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
#include <fcntl.h>

#include "global.h"
#include "common.h"
#include "path2url.h"
#include "htags.h"

/*----------------------------------------------------------------------*/
/* Tag definitions							*/
/*----------------------------------------------------------------------*/
char *html_begin;
char *html_end;
char *body_begin;
char *body_end;
char *head_begin;
char *head_end;
char *title_begin;
char *title_end;
char *list_begin;
char *list_item;
char *list_end;
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
char *string_begin;
char *string_end;
char *quote_great;
char *quote_little;
char *quote_amp;
char *quote_space;
char *hr;
char *br;
char *status_line;
char msgbuf[1024];
/*
 * HTML tag.
 */
void
setup_html()
{
	html_begin	= "<HTML>";
	html_end	= "</HTML>";
	body_begin	= "<BODY>";
	body_end	= "</BODY>";
	head_begin	= "<HEAD>";
	head_end	= "</HEAD>";
	title_begin	= "<H1><FONT COLOR=#cc0000>";
	title_end	= "</FONT></H1>";
	list_begin	= "<OL>";
	list_item	= "<LI>";
	list_end	= "</OL>";
	table_begin	= "<TABLE>";
	table_end	= "</TABLE>";
	comment_begin	= "<I><FONT COLOR=green>";
	comment_end	= "</FONT></I>";
	sharp_begin	= "<FONT COLOR=darkred>";
	sharp_end	= "</FONT>";
	brace_begin	= "<FONT COLOR=blue>";
	brace_end	= "</FONT>";
	verbatim_begin	= "<PRE>";
	verbatim_end	= "</PRE>";
	reserved_begin	= "<B>";
	reserved_end	= "</B>";
	position_begin	= "<FONT COLOR=gray>";
	position_end	= "</FONT>";
	warned_line_begin = "<SPAN STYLE=\"background-color:yellow\">";
	warned_line_end   = "</SPAN>";
	string_begin	= "<U>";
	string_end	= "</U>";;
	quote_great	= "&gt;";
	quote_little	= "&lt;";
	quote_amp	= "&amp;";
	quote_space	= "&nbsp;";
	hr              = "<HR>";
	br              = "<BR>";
}
/*
 * XHTML tag.
 */
void
setup_xhtml()
{
	setup_html();
}
/*
 * TEX tag.
 */
void
setup_tex()
{
	setup_html();
}
char *
header_record(title)
	char *title;
{
	static char buf[1024];

	snprintf(buf, sizeof(buf), "%s\n%s%s%s\n%s%s\n",
		head_begin, title_begin, title, title_end, meta_record(), head_end);
	return buf;
}
char *
meta_record()
{
	static char buf[512];
	char *s1 = "META NAME='ROBOTS' CONTENT='NOINDEX,NOFOLLOW'";
	char *s2 = "META NAME='GENERATOR'";

	snprintf(buf, sizeof(buf), "<%s>\n<%s CONTENT='GLOBAL-%s'>\n", s1, s2, get_version());
	return buf;
}
char *
Hn(n, label)
	int n;
	char *label;
{
	static char buf[512];
	if (n < 1)
		n = 1;
	if (n > 5)
		n = 5;
	snprintf(buf, sizeof(buf), "<H%d>%s</H%d>", n, label, n);
	return buf;
}
char *
anchor(label, link)
	char *label;
	char *link;
{
	static char buf[512];
	snprintf(buf, sizeof(buf), "<A HREF=%s>%s</A>", link, label);
	return buf;
}
static STRBUF *
edit_buffer()
{
	static STRBUF *sb = NULL;

	if (sb == NULL)
		sb = strbuf_open(0);
	else
		strbuf_reset(sb);
	return sb;
}

char *
set_header(title)
	char *title;
{
	STRBUF *sb = edit_buffer();

	strbuf_puts(sb, "<HEAD>\n<TITLE>");
	strbuf_puts(sb, title);
	strbuf_puts(sb, "</TITLE>\n");
	strbuf_puts(sb, meta_record());
	if (style_sheet)
		strbuf_puts(sb, style_sheet);
	strbuf_puts(sb, "</HEAD>\n");
	return strbuf_value(sb);
}


/*
 * list_begin:
 */
char *
gen_list_begin()
{
	STRBUF *sb = edit_buffer();

	if (table_list) {
		strbuf_puts(sb, table_begin);
		strbuf_putc(sb, '\n');
		strbuf_puts(sb, "<TR><TH NOWRAP ALIGN=left>tag</TH>");
		strbuf_puts(sb, "<TH NOWRAP ALIGN=right>line</TH>");
		strbuf_puts(sb, "<TH NOWRAP ALIGN=center>file</TH>");
		strbuf_puts(sb, "<TH NOWRAP ALIGN=left>source code</TH></TR>");
	} else {
		strbuf_puts(sb, "<PRE>");
	}
	return strbuf_value(sb);
}
/*
 * list_body:
 *
 * s must be choped.
 */
char *
gen_list_body(srcdir, string)
	char *srcdir;
	char *string;
{
	STRBUF *sb = edit_buffer();
	char *name, *lno, *filename, *line, *html;
	char *p;
	SPLIT ptable;

	if (split(string, 4, &ptable) < 4) {
		recover(&ptable);
		die("too small number of parts in list_body().\n'%s'", string);
	}
	name = ptable.part[0].start;
	lno = ptable.part[1].start;
	filename = ptable.part[2].start;
	line = ptable.part[3].start;
	filename += 2;				/* remove './' */
	html = path2url(filename);

	if (table_list) {
		strbuf_sprintf(sb, "<TR><TD NOWRAP><A HREF=%s/%s#%s>%s</A></TD>",
			srcdir, html, lno, name);
		strbuf_sprintf(sb, "<TD NOWRAP ALIGN=right>%s</TD><TD NOWRAP ALIGN=left>%s</TD><TD NOWRAP>",
			lno, filename);

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
		strbuf_puts(sb, "</TD></TR>");
		recover(&ptable);
	} else {
		int done = 0;

		strbuf_sprintf(sb, "<A HREF=%s/%s#%s>%s</A>",
			srcdir, html, lno, name);
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
 * list_end:
 */
char *
gen_list_end()
{
	STRBUF *sb = edit_buffer();

	if (table_list)
		strbuf_puts(sb, table_end);
	else
		strbuf_puts(sb, "</PRE>");
	return strbuf_value(sb);
}

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
		c = buf[i];
		if (c == 0 || c > 127)
			return 1;
	}
	return 0;
}
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
		int c = *p;

                if (isalnum(c))
			strbuf_putc(sb, c);
		else
			strbuf_sprintf(sb, "%%%02x", c);
        }

	return strbuf_value(sb);
}
