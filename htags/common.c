/*
 * Copyright (c) 2004, 2005, 2008, 2010, 2011 Tama Communications Corporation
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
#include "anchor.h"
#include "common.h"
#include "htags.h"
#include "path2url.h"

/**
 * @name Tag definitions
 */
/** @{ */
const char *html_begin		= "<html xmlns='http://www.w3.org/1999/xhtml'>";
const char *html_end		= "</html>";
const char *html_head_begin	= "<head>";
const char *html_head_end	= "</head>";
const char *html_title_begin	= "<title>";
const char *html_title_end	= "</title>";
const char *body_begin		= "<body>";
const char *body_end		= "</body>";
const char *title_begin		= "<h1 class='title'>";
const char *title_end		= "</h1>";
const char *header_begin	= "<h2 class='header'>";
const char *header_end		= "</h2>";
const char *poweredby_begin	= "<div class='poweredby'>";
const char *poweredby_end	= "</div>";
const char *cvslink_begin	= "<span class='cvs'>";
const char *cvslink_end		= "</span>";
const char *caution_begin	= "<div class='caution'>";
const char *caution_end		= "</div>";
const char *list_begin		= "<ol>";
const char *list_end		= "</ol>";
const char *item_begin		= "<li>";
const char *item_end		= "</li>";
const char *flist_begin		= "<table class='flist'>";
const char *flist_end		= "</table>";
const char *fline_begin		= "<tr class='flist'>";
const char *fline_end		= "</tr>";
const char *fitem_begin		= "<td class='flist'>";
const char *fitem_end		= "</td>";
const char *define_list_begin	= "<dl>";
const char *define_list_end	= "</dl>";
const char *define_term_begin	= "<dt>";
const char *define_term_end	= "</dt>";
const char *define_desc_begin	= "<dd>";
const char *define_desc_end	= "</dd>";
const char *table_begin		= "<table>";
const char *table_end		= "</table>";
const char *comment_begin	= "<em class='comment'>";
const char *comment_end		= "</em>";
const char *sharp_begin		= "<em class='sharp'>";
const char *sharp_end		= "</em>";
const char *brace_begin		= "<em class='brace'>";
const char *brace_end		= "</em>";
const char *verbatim_begin	= "<pre>";
const char *verbatim_end	= "</pre>";
const char *reserved_begin	= "<strong class='reserved'>";
const char *reserved_end	= "</strong>";
const char *position_begin	= "<em class='position'>";
const char *position_end	= "</em>";
const char *warned_line_begin	= "<em class='warned'>";
const char *warned_line_end	= "</em>";
const char *current_line_begin	= "<span class='curline'>";
const char *current_line_end	= "</span>";
const char *current_row_begin	= "<tr class='curline'>";
const char *current_row_end	= "</tr>";
const char *error_begin		= "<h2 class='error'>";
const char *error_end		= "</h2>";
const char *message_begin	= "<h3 class='message'>";
const char *message_end		= "</h3>";
const char *string_begin	= "<em class='string'>";
const char *string_end		= "</em>";
const char *quote_great		= "&gt;";
const char *quote_little	= "&lt;";
const char *quote_amp		= "&amp;";
const char *quote_space		= "&nbsp;";
const char *hr			= "<hr />";
const char *br			= "<br />";
const char *empty_element	= " /";
const char *noframes_begin	= "<noframes>";
const char *noframes_end	= "</noframes>";
/** @} */

/** @name tree view tag (--tree-view) */
/** @{ */
const char *tree_control	= "<div id='control'>All <a href='#'>close</a> | <a href='#'>open</a></div>";
const char *tree_loading	= "<span id='init' class='loading'>Under construction...</span>";
const char *tree_begin		= "<ul id='tree'>";
const char *tree_begin_using	= "<ul id='tree' class='%s'>";
const char *tree_end		= "</ul>";
const char *dir_begin		= "<li><span class='folder'>";
const char *dir_title_end	= "</span>";
const char *dir_end		= "</li>";
const char *file_begin		= "<li><span class='file'>";
const char *file_end		= "</span></li>";
/** @} */

/** @name fixed guide tag (--fixed-guide) */
/** @{ */
const char *guide_begin		= "<div id='guide'><ul>";
const char *guide_end		= "</ul></div>";
const char *guide_unit_begin	= "<li>";
const char *guide_unit_end	= "</li>";
const char *guide_path_begin	= "<li class='standout'><span>";
const char *guide_path_end	= "</span></li>";
/** @} */

static const char *fix_attr_value(const char *);

/**
 * print string and new line.
 *
 * This function is a replacement of @CODE{fprintf(op, \"\%s\\n\", s)} in @NAME{htags}.
 */
int
fputs_nl(const char *s, FILE *op)
{
	fputs(s, op);
	putc('\n', op);
	return 0;
}
/**
 * @name These methods is used to tell lex() the current path infomation.
 */
/** @{ */
static char current_path[MAXPATHLEN];
static char current_dir[MAXPATHLEN];
static char current_file[MAXPATHLEN];
/** @} */

/**
 * save path infomation
 */
void
save_current_path(const char *path)
{
	char *startp, *p;

	strlimcpy(current_path, path, sizeof(current_path));
	/* Extract directory name and file name from path */
	strlimcpy(current_dir, path, sizeof(current_path));
	startp = current_dir;
	p = startp + strlen(current_dir);
	while (p > startp) {
		if (*--p == '/') {
			*p = '\0';
			strlimcpy(current_file, p + 1, sizeof(current_file));
			return;
		}
	}
	/* It seems that the path doesn't include '/' */
	strlimcpy(current_file, path, sizeof(current_file));
}
char *
get_current_dir(void)
{
	return current_dir;
}
char *
get_current_file(void)
{
	return current_file;
}

/**
 * Generate upper directory.
 *
 * Just returns the parent path of @a dir. (Adds @FILE{../} to it).
 */
const char *
upperdir(const char *dir)
{
	STATIC_STRBUF(sb);

	strbuf_clear(sb);
	strbuf_sprintf(sb, "../%s", dir);
	return strbuf_value(sb);
}
/**
 * Load text from file with replacing @CODE{\@PARENT_DIR\@} macro.
 * Macro @CODE{\@PARENT_DIR\@} is replaced with the parent directory
 * of the @FILE{HTML} directory.
 */
static const char *
sed(FILE *ip, int place)
{
	STATIC_STRBUF(sb);
	const char *parent_dir = (place == SUBDIR) ? "../.." : "..";
	int c, start_position = -1;

	strbuf_clear(sb);
	while ((c = fgetc(ip)) != EOF) {
		strbuf_putc(sb, c);
		if (c == '@') {
			int curpos = strbuf_getlen(sb);
			if (start_position == -1) {
				start_position = curpos - 1;
			} else {
				if (!strncmp("@PARENT_DIR@",
					strbuf_value(sb) + start_position,
					curpos - start_position))
				{
					strbuf_setlen(sb, start_position);
					strbuf_puts(sb, parent_dir);
					start_position = -1;
				} else {
					start_position = curpos - 1;
				}
			}
		} else if (!isalpha(c) && c != '_') {
			if (start_position != -1)
				start_position = -1;
		}
	}
	return strbuf_value(sb);
}
/**
 * Generate custom header.
 */
const char *
gen_insert_header(int place)
{
	static FILE *ip;

	if (ip != NULL) {
		rewind(ip);
	} else {
		ip = fopen(insert_header, "r");
		if (ip == NULL)
			die("cannot open include header file '%s'.", insert_header);
	}
	return sed(ip, place);
}
/**
 * Generate custom footer.
 */
const char *
gen_insert_footer(int place)
{
	static FILE *ip;

	if (ip != NULL) {
		rewind(ip);
	} else {
		ip = fopen(insert_footer, "r");
		if (ip == NULL)
			die("cannot open include footer file '%s'.", insert_footer);
	}
	return sed(ip, place);
}
/**
 * Generate beginning of generic page
 *
 *	@param[in]	title	title of this page
 *	@param[in]	place	#SUBDIR: this page is in sub directory <br>
 *			#TOPDIR: this page is in the top directory
 *	@param[in]	use_frameset
 *			use frameset document type or not
 *	@param[in]	header_item
 *			item which should be inserted into the header
 */
static const char *
gen_page_generic_begin(const char *title, int place, int use_frameset, const char *header_item)
{
	STATIC_STRBUF(sb);
	const char *dir = NULL;

	switch (place) {
	case TOPDIR:
		dir = "";
		break;
	case SUBDIR:
		 dir = "../";
		break;
	case CGIDIR:
		 dir = "$basedir/";	/* decided by the CGI script */
		break;
	}
	strbuf_clear(sb);
	if (enable_xhtml) {
		/*
		 * If the --frame option are specified then we take
		 * 'XHTML 1.0 Frameset' for index.html
		 * and 'XHTML 1.0 Transitional' for other files.
		 */
		if (use_frameset)
			strbuf_puts_nl(sb, "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Frameset//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd'>");
		else
			strbuf_puts_nl(sb, "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Transitional//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'>");
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
	if (header_item)
		strbuf_puts(sb, header_item);		/* internal use */
	if (html_header)
		strbuf_puts(sb, html_header);		/* --html-header=file */
	strbuf_puts(sb, html_head_end);
	return strbuf_value(sb);
}
/**
 * Generate beginning of normal page
 *
 *	@param[in]	title	title of this page
 *	@param[in]	place	#SUBDIR: this page is in sub directory <br>
 *			#TOPDIR: this page is in the top directory
 */
const char *
gen_page_begin(const char *title, int place)
{
	return gen_page_generic_begin(title, place, 0, NULL);
}
/**
 * beginning of normal page for index page
 *
 *	@param[in]	title	title of this page
 *	@param[in]	header_item	an item which should be inserted into the header
 */
const char *
gen_page_index_begin(const char *title, const char *header_item)
{
	return gen_page_generic_begin(title, TOPDIR, 0, header_item);
}
/**
 * Generate beginning of frameset page (@CODE{\<frameset\>})
 *
 *	@param[in]	title	title of this page
 */
const char *
gen_page_frameset_begin(const char *title)
{
	return gen_page_generic_begin(title, TOPDIR, 1, NULL);
}
/**
 * Generate end of page (@CODE{\</html\>})
 */
const char *
gen_page_end(void)
{
	return html_end;
}

/**
 * Generate image tag (@CODE{\<img\>})
 *
 *	@param[in]	where	Where is the icon directory? <br>
 *			#CURRENT: current directory <br>
 *			#PARENT: parent directory
 *	@param[in]	file	icon file without suffix.
 *	@param[in]	alt	alt string (the @CODE{alt} attribute is always added)
 *
 *	@note Images are assumed to be in the @FILE{icons} or @FILE{../icons} directory, only.
 */
const char *
gen_image(int where, const char *file, const char *alt)
{
	STATIC_STRBUF(sb);
	const char *dir = (where == PARENT) ? "../icons" : "icons";

	strbuf_clear(sb);
	if (enable_xhtml)
		strbuf_sprintf(sb, "<img class='icon' src='%s/%s.%s' alt='[%s]'%s>",
			dir, file, icon_suffix, fix_attr_value(alt), empty_element);
	else
		strbuf_sprintf(sb, "<img src='%s/%s.%s' alt='[%s]' %s%s>",
			dir, file, icon_suffix, fix_attr_value(alt), icon_spec, empty_element);
	return strbuf_value(sb);
}
/**
 * Generate name tag.
 */
const char *
gen_name_number(int number)
{
	static char buf[32];

	snprintf(buf, sizeof(buf), "L%d", number);
	return gen_name_string(buf);
}
/**
 * Generate name tag (@CODE{\<a name='xxx'\>}).
 *
 * Uses attribute @CODE{'id'}, if is @NAME{XHTML}.
 */
const char *
gen_name_string(const char *name)
{
	STATIC_STRBUF(sb);

	strbuf_clear(sb);
	if (enable_xhtml) {
		/*
		 * Since some browser cannot understand "<a id='xxx' />",
		 * we put both of 'id=' and 'name='.
		 */
		strbuf_sprintf(sb, "<a id='%s' name='%s'></a>", name, name);
	} else {
		strbuf_sprintf(sb, "<a name='%s'></a>", name);
	}
	return strbuf_value(sb);
}
/**
 * Generate anchor begin tag (@CODE{\<a href='dir/file.suffix\#key'\>}).
 * (complete format)
 *
 *	@param[in]	dir	directory
 *	@param[in]	file	file
 *	@param[in]	suffix	suffix (file extension e.g. @CODE{'.txt'}). A @CODE{'.'} (dot) will be added.
 *	@param[in]	key	key
 *	@param[in]	title	@CODE{title='xxx'} attribute; if @VAR{NULL}, doesn't add it.
 *	@param[in]	target	@CODE{target='xxx'} attribute; if @VAR{NULL}, doesn't add it.
 *	@return		generated anchor tag
 *
 *	@note @a dir, @a file, @a suffix, @a key, @a target and @a title may be @VAR{NULL}.
 *	@note Single quote (@CODE{'}) characters are used with the attribute values.
 */
const char *
gen_href_begin_with_title_target(const char *dir, const char *file, const char *suffix, const char *key, const char *title, const char *target)
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
		/*
		 * If the key starts with a digit, it assumed line number.
		 * XHTML 1.1 profibits number as an anchor.
		 */
		if (isdigit(*key))
			strbuf_putc(sb, 'L');
		strbuf_puts(sb, key);
	}
	strbuf_putc(sb, '\'');
	if (Fflag && target)
		strbuf_sprintf(sb, " target='%s'", fix_attr_value(target));
	if (title)
		strbuf_sprintf(sb, " title='%s'", fix_attr_value(title));
	strbuf_putc(sb, '>');
	return strbuf_value(sb);
}
/**
 * Generate simple anchor begin tag.
 *
 * @par Uses:
 *		gen_href_begin_with_title_target()
 */
const char *
gen_href_begin_simple(const char *file)
{
	return gen_href_begin_with_title_target(NULL, file, NULL, NULL, NULL, NULL);
}
/**
 * Generate anchor begin tag without title and target.
 *
 * @par Uses:
 *		gen_href_begin_with_title_target()
 */
const char *
gen_href_begin(const char *dir, const char *file, const char *suffix, const char *key)
{
	return gen_href_begin_with_title_target(dir, file, suffix, key, NULL, NULL);
}
/**
 * Generate anchor begin tag without target.
 *
 * @par Uses:
 *		gen_href_begin_with_title_target()
 */
const char *
gen_href_begin_with_title(const char *dir, const char *file, const char *suffix, const char *key, const char *title)
{
	return gen_href_begin_with_title_target(dir, file, suffix, key, title, NULL);
}
/**
 * Generate anchor end tag (@CODE{\</a\>}).
 */
const char *
gen_href_end(void)
{
	return "</a>";
}
/**
 * Generate list begin tag.
 */
const char *
gen_list_begin(void)
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
					"<tr><th nowrap='nowrap' align='left'>tag</th>",
					"<th nowrap='nowrap' align='right'>line</th>",
					"<th nowrap='nowrap' align='center'>file</th>",
					"<th nowrap='nowrap' align='left'>source code</th></tr>");
			}
		} else {
			strbuf_puts(sb, verbatim_begin);
		}
	}
	return strbuf_value(sb);
}
/**
 * Generate list body.
 *
 * @NAME{ctags_x} with the @CODE{--encode-path=\" \\t\"}
 */
const char *
gen_list_body(const char *srcdir, const char *ctags_x, const char *fid)	/* virtually const */
{
	STATIC_STRBUF(sb);
	char path[MAXPATHLEN];
	const char *p;
	SPLIT ptable;

	strbuf_clear(sb);
	if (split((char *)ctags_x, 4, &ptable) < 4) {
		recover(&ptable);
		die("too small number of parts in list_body().\n'%s'", ctags_x);
	}
	strlimcpy(path, decode_path(ptable.part[PART_PATH].start + 2), sizeof(path));
	if (fid == NULL)
		fid = path2fid(path);
	if (table_list) {
		strbuf_puts(sb, current_row_begin);
		if (enable_xhtml) {
			strbuf_puts(sb, "<td class='tag'>");
			strbuf_puts(sb, gen_href_begin(srcdir, fid, HTML, ptable.part[PART_LNO].start));
			strbuf_puts(sb, ptable.part[PART_TAG].start);
			strbuf_puts(sb, gen_href_end());
			strbuf_sprintf(sb, "</td><td class='line'>%s</td><td class='file'>%s</td><td class='code'>",
				ptable.part[PART_LNO].start, path);
		} else {
			strbuf_puts(sb, "<td nowrap='nowrap'>");
			strbuf_puts(sb, gen_href_begin(srcdir, fid, HTML, ptable.part[PART_LNO].start));
			strbuf_puts(sb, ptable.part[PART_TAG].start);
			strbuf_puts(sb, gen_href_end());
			strbuf_sprintf(sb, "</td><td nowrap='nowrap' align='right'>%s</td>"
				       "<td nowrap='nowrap' align='left'>%s</td><td nowrap='nowrap'>",
				ptable.part[PART_LNO].start, path);
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
		strbuf_puts(sb, "</td>");
		strbuf_puts(sb, current_row_end);
		recover(&ptable);
	} else {
		/* print tag name with anchor */
		strbuf_puts(sb, current_line_begin);
		strbuf_puts(sb, gen_href_begin(srcdir, fid, HTML, ptable.part[PART_LNO].start));
		strbuf_puts(sb, ptable.part[PART_TAG].start);
		strbuf_puts(sb, gen_href_end());
		recover(&ptable);

		/* print line number */
		for (p = ptable.part[PART_TAG].end; p < ptable.part[PART_PATH].start; p++)
			strbuf_putc(sb, *p);
		/* print file name */
		strbuf_puts(sb, path);
		/* print the rest */
		for (p = ptable.part[PART_PATH].end; *p; p++) {
			unsigned char c = *p;

			if (c == '&')
				strbuf_puts(sb, quote_amp);
			else if (c == '<')
				strbuf_puts(sb, quote_little);
			else if (c == '>')
				strbuf_puts(sb, quote_great);
			else
				strbuf_putc(sb, c);
		}
		strbuf_puts(sb, current_line_end);
	}
	return strbuf_value(sb);
}
/**
 * Generate list end tag.
 */
const char *
gen_list_end(void)
{
	return table_list ? table_end : verbatim_end;
}
/**
 * Generate beginning of form (@CODE{\<form\>})
 *
 *	@param[in]	target	target attribute or @VAR{NULL} for no target.
 */
const char *
gen_form_begin(const char *target)
{
	STATIC_STRBUF(sb);

	strbuf_clear(sb);
	strbuf_sprintf(sb, "<form method='get' action='%s'", fix_attr_value(action));
	if (Fflag && target)
		strbuf_sprintf(sb, " target='%s'", fix_attr_value(target));
	strbuf_puts(sb, ">");
	return strbuf_value(sb);
}
/**
 * Generate end of form (@CODE{\</form\>})
 */
const char *
gen_form_end(void)
{
	return "</form>";
}
/**
 * Generate input tag (@CODE{\<input\>})
 * @par Uses:
 *		gen_input_with_title_checked()
 */
const char *
gen_input(const char *name, const char *value, const char *type)
{
	return gen_input_with_title_checked(name, value, type, 0, NULL);
}
/**
 * Generate input radiobox tag (@CODE{\<input type='radio'\>})
 * @par Uses:
 *		gen_input_with_title_checked()
 */
const char *
gen_input_radio(const char *name, const char *value, int checked, const char *title)
{
	return gen_input_with_title_checked(name, value, "radio", checked, title);
}
/**
 * Generate input checkbox tag (@CODE{\<input type='checkbox'\>})
 * @par Uses:
 *		gen_input_with_title_checked()
 */
const char *
gen_input_checkbox(const char *name, const char *value, const char *title)
{
	return gen_input_with_title_checked(name, value, "checkbox", 0, title);
}
/**
 * Generate input radio tag (@CODE{\<input\>})
 *
 *	@note @a name, @a value, @a type and @a title may be @VAR{NULL}, thus only those
 *		with a non-@VAR{NULL} value will have there attribute added. <br>
 *		The argument names are the same as the corresponding HTML attribute names.
 *	@note Single quote (@CODE{'}) characters are used with the attribute values.
 */
const char *
gen_input_with_title_checked(const char *name, const char *value, const char *type, int checked, const char *title)
{
	STATIC_STRBUF(sb);

	strbuf_clear(sb);
	strbuf_puts(sb, "<input");
	if (type)
		strbuf_sprintf(sb, " type='%s'", type);
	if (name)
		strbuf_sprintf(sb, " name='%s' id='%s'", name, name);
	if (value)
		strbuf_sprintf(sb, " value='%s'", fix_attr_value(value));
	if (checked) {
		if (enable_xhtml)
			strbuf_puts(sb, " checked='checked'");
		else
			strbuf_puts(sb, " checked");
	}
	if (title)
		strbuf_sprintf(sb, " title='%s'", fix_attr_value(title));
	strbuf_sprintf(sb, "%s>", empty_element);
	return strbuf_value(sb);
}
/**
 * Generate beginning of frameset (@CODE{\<frameset\>})
 *
 *	@param[in]	contents	target
 */
const char *
gen_frameset_begin(const char *contents)
{
	STATIC_STRBUF(sb);

	strbuf_clear(sb);
	strbuf_sprintf(sb, "<frameset %s>", contents);
	return strbuf_value(sb);
}
/**
 * Generate end of frameset (@CODE{\</frameset\>})
 */
const char *
gen_frameset_end(void)
{
	return "</frameset>";
}
/**
 * Generate beginning of frame (@CODE{\<frame\>})
 *
 *	@param[in]	name	target (value for @CODE{name} and @CODE{id} attributes)
 *	@param[in]	src	value for @CODE{src} attribute
 */
const char *
gen_frame(const char *name, const char *src)
{
	STATIC_STRBUF(sb);

	strbuf_clear(sb);
	strbuf_sprintf(sb, "<frame name='%s' id='%s' src='%s'%s>", name, name, src, empty_element);
	return strbuf_value(sb);
}


/** HTML attribute delimiter character ( ' or &quot; only) */
#define ATTR_DELIM '\''

/**
 * Check and fix an attribute's value; convert all @c ' (single quote) characters
 * into @CODE{\&\#39;} within it.
 */
static const char *
fix_attr_value(const char *value)
{
	STATIC_STRBUF(sb);
	char c;
	const char *cptr;

	strbuf_clear(sb);
	cptr = value;

	while((c = *cptr) != '\0') {
		if(c == ATTR_DELIM)
			strbuf_puts(sb, "&#39;");
		else
			strbuf_putc(sb, c);
		++cptr;
	}
	return strbuf_value(sb);
}
