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
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "queue.h"
#include "global.h"
#include "cache.h"
#include "htags.h"
#include "path2url.h"
#include "common.h"

/*
 * makedefineindex: make definition index (including alphabetic index)
 *
 *	i)	file		definition index file
 *	i)	total		definitions total
 *	o)	@defines
 *	gi)	tag cache
 */
int
makedefineindex(file, total, defines)
	char *file;
	int total;
	STRBUF *defines;
{
	int count = 0;
	int alpha_count = 0;
	char indexlink[1024];
	char *index_string = "Index Page";
	char *target = (Fflag) ? "mains" : "_top";
	FILE *MAP = NULL;
	FILE *DEFINES, *old, *STDOUT, *TAGS, *ALPHA = NULL;
	STRBUF *sb = strbuf_open(0);
	STRBUF *url = strbuf_open(0);
	char *_;
	char command[1024], buf[1024], alpha[32], alpha_f[32];

	if (Fflag)
		snprintf(indexlink, sizeof(indexlink), "../defines.%s", normal_suffix);
	else
		snprintf(indexlink, sizeof(indexlink), "../mains.%s", normal_suffix);
	if (map_file) {
		if (!(MAP = fopen(makepath(distpath, "MAP", NULL), "w")))
			die("cannot open '%s'.", makepath(distpath, "MAP", NULL));
	}
	if (!(DEFINES = fopen(makepath(distpath, file, NULL), "w")))
		die("cannot make function index '%s'.", file);
	fprintf(DEFINES, "%s\n", html_begin);
	fprintf(DEFINES, "%s", set_header(title_define_index));
	fprintf(DEFINES, "%s\n", body_begin);
	if (Fflag)
		fprintf(DEFINES, "<A HREF=defines.%s><H2>%s</H2></A>\n", normal_suffix, title_define_index);
	else
		fprintf(DEFINES, "<H2>%s</H2>\n", title_define_index);
	if (!aflag && !Fflag) {
		snprintf(indexlink, sizeof(indexlink), "mains.%s", normal_suffix);
		fprintf(DEFINES, "<A HREF=%s TITLE='%s'>", indexlink, index_string);
		if (icon_list)
			fprintf(DEFINES, "<IMG SRC=icons/%s.%s ALT='[..]' %s>", back_icon, icon_suffix, icon_spec);
		else
			fprintf(DEFINES, "[..]");
		fprintf(DEFINES, "</A>\n");
	}
	if (!aflag) {
		if (!no_order_list)
			fprintf(DEFINES, "%s\n", list_begin);
		/*
		else
			fprintf(DEFINES, "%s\n", br);
		*/
	}
	/*
	 * map DEFINES to STDOUT.
	 */
	old = STDOUT;
	STDOUT = DEFINES;
	snprintf(command, sizeof(command), "global -c");
	if ((TAGS = popen(command, "r")) == NULL)
		die("cannot fork.");
	alpha[0] = '\0';
	while ((_ = strbuf_fgets(sb, TAGS, STRBUF_NOCRLF)) != NULL) {
		char *line, *tag;
		char guide[1024], url_for_map[1024];

		count++;
		tag = _;
		message(" [%d/%d] adding %s", count, total, tag);
		if (aflag && (alpha[0] == '\0' || strncmp(tag, alpha, strlen(alpha)))) {
			char *msg = (alpha_count == 1) ? "definition is containded." : "definitions are containded.";

			if (alpha[0]) {
				strbuf_sprintf(defines, "<A HREF=defines/%s.%s TITLE='%d %s'>[%s]</A>\n",
					alpha_f, HTML, alpha_count, msg, alpha);
				alpha_count = 0;
				if (!no_order_list)
					fprintf(ALPHA, "%s\n", list_end);
				else
					fprintf(ALPHA, "%s\n", br);
				fprintf(ALPHA, "<A HREF=%s TITLE='%s'>", indexlink, index_string);
				if (icon_list)
					fprintf(ALPHA, "<IMG SRC=../icons/%s.%s ALT='[..]' %s>", back_icon, icon_suffix, icon_spec);
				else
					fprintf(ALPHA, "[..]");
				fprintf(ALPHA, "</A>\n");
				fprintf(ALPHA, "%s\n", body_end);
				fprintf(ALPHA, "%s\n", html_end);
				if (cflag) {
					if (pclose(ALPHA) < 0)
						die("terminated abnormally.");
				} else
					fclose(ALPHA);
				file_count++;
			}
			/*
			 * setup index char (for example, 'a' of '[a]').
			 * alpha is used for display.
			 * alpha_f is used for part of path.
			 */
			if (*(unsigned char *)tag > 127) {
				int i1 = *tag & 0xff;
				int i2 = *(tag + 1) & 0xff;
				/*
				 * for multi-byte(EUC) code.
				 */
				alpha[0] = *tag;
				alpha[1] = *(tag + 1);
				alpha[2] = '\0';
				snprintf(alpha_f, sizeof(alpha_f), "%03d%03d", i1, i2);
			} else {
				alpha[0] = *tag;
				alpha[1] = '\0';
				/*
				 * for CD9660 or FAT file system
				 * 97 == 'a', 122 == 'z'
				 */
				if (*tag >= 'a' && *tag <= 'z') {
					alpha_f[0] = 'l';
					alpha_f[1] = *tag;
					alpha_f[2] = '\0';
				} else {
					alpha_f[0] = *tag;
					alpha_f[1] = '\0';
				}
			}
			if (cflag) {
				snprintf(buf, sizeof(buf), "gzip -c >%s/defines/%s.%s", distpath, alpha_f, HTML);
				ALPHA = popen(buf, "w");
			} else {
				snprintf(buf, sizeof(buf), "%s/defines/%s.%s", distpath, alpha_f, HTML);
				ALPHA = fopen(buf, "w");
			}
			if (!ALPHA)
				die("cannot make alphabetical function index.");
			fprintf(ALPHA, "%s\n", html_begin);
			snprintf(buf, sizeof(buf), "[%s]", alpha);
			fprintf(ALPHA, "%s", set_header(buf));
			fprintf(ALPHA, "%s\n", body_begin);
			fprintf(ALPHA, "<H2>[%s]</H2>\n", alpha);
			fprintf(ALPHA, "<A HREF=%s TITLE='%s'>", indexlink, index_string);
			if (icon_list)
				fprintf(ALPHA, "<IMG SRC=../icons/%s.%s ALT='[..]' %s>", back_icon, icon_suffix, icon_spec);
			else
				fprintf(ALPHA, "[..]");
			fprintf(ALPHA, "</A>\n");
			if (!no_order_list)
				fprintf(ALPHA, "%s\n", list_begin);
			else
				fprintf(ALPHA, "%s%s\n", br, br);
			STDOUT = ALPHA;
		}
		alpha_count++;
		/*
		 * generating url for function definition.
	 	 */
		line = cache_get(GTAGS, tag);
		strbuf_reset(url);

		if (line == NULL)
			die("internal error in makedefineindex()."); 
		if (*line == ' ') {
			SPLIT ptable;
			char *fid, *enumber;

			if (split(line + 1, 2, &ptable) < 2) {
				recover(&ptable);
				die("too small number of parts in makedefineindex().\n'%s'", line);
			}
			fid     = ptable.part[0].start;
			enumber = ptable.part[1].start;
			snprintf(url_for_map, sizeof(url_for_map), "%s/%s.%s",
				DEFS, fid, HTML);
			/*
			 * cache record: " <file id> <entry number>"
			 */
			if (dynamic) {
				if (*action != '/' && aflag)
					strbuf_puts(url, "../");
				strbuf_puts(url, action);
				strbuf_sprintf(url, "?pattern=%s&type=definitions", tag);
			} else {
				if (aflag)
					strbuf_puts(url, "../");
				strbuf_sprintf(url, "%s/%s.%s", DEFS, fid, HTML);
			}
			snprintf(guide, sizeof(guide), "Multiple defined in %s places.", enumber);
			recover(&ptable);
		} else {
			SPLIT ptable;
			char *lno, *filename, *path;

			if (split(line, 3, &ptable) < 3) {
				recover(&ptable);
				die("too small number of parts in makedefineindex().\n'%s'", line);
			}
			lno = ptable.part[0].start;
			path = ptable.part[1].start;
			path += 2;		/* remove './' */

			filename = path2url(path);
			snprintf(url_for_map, sizeof(url_for_map), "%s/%s#%s",
				SRCS, filename, lno);
			if (aflag)
				strbuf_puts(url, "../");
			strbuf_sprintf(url, "%s/%s#%s", SRCS, filename, lno);
			snprintf(guide, sizeof(guide), "Defined at %s in %s.", lno, path);
			recover(&ptable);
		}
		if (!no_order_list)
			fprintf(STDOUT, list_item);
		fprintf(STDOUT, "<A HREF=%s TARGET=%s TITLE='%s'>%s</A>\n", strbuf_value(url), target, guide, tag);
		if (no_order_list)
			fprintf(STDOUT, br);
		if (map_file)
			fprintf(MAP, "%s\t%s\n", tag, url_for_map);
	}
	if (pclose(TAGS) < 0)
		die("'%s' failed.", command);
	STDOUT = old;
	if (aflag && alpha[0]) {
		char *msg = (alpha_count == 1) ? "definition is containded." : "definitions are containded.";

		strbuf_sprintf(defines, "<A HREF=defines/%s.%s TITLE='%d %s'>[%s]</A>\n", alpha_f, HTML, alpha_count, msg, alpha);
		if (!no_order_list)
			fprintf(ALPHA, "%s\n", list_end);
		else
			fprintf(ALPHA, "%s\n", br);
		fprintf(ALPHA, "<A HREF=%s TITLE='%s'>", indexlink, index_string);
		if (icon_list)
			fprintf(ALPHA, "<IMG SRC=../icons/%s.%s ALT='[..]' %s>", back_icon, icon_suffix, icon_spec);
		else
			fprintf(ALPHA, "[..]");
		fprintf(ALPHA, "</A>\n");
		fprintf(ALPHA, "%s\n", body_end);
		fprintf(ALPHA, "%s\n", html_end);
		fclose(ALPHA);
		file_count++;

		fprintf(DEFINES, strbuf_value(defines));
	}
	if (!no_order_list && !aflag)
		fprintf(DEFINES, "%s\n", list_end);
	if (!aflag && !Fflag) {
		fprintf(DEFINES, "<A HREF=mains.%s TITLE='Index Page'>", normal_suffix);
		if (icon_list)
			fprintf(DEFINES, "<IMG SRC=icons/%s.%s ALT='[..]' %s>", back_icon, icon_suffix, icon_spec);
		else
			fprintf(DEFINES, "[..]");
		fprintf(DEFINES, "</A>\n");
	}
	fprintf(DEFINES, "%s\n", body_end);
	fprintf(DEFINES, "%s\n", html_end);
	fclose(DEFINES);
	file_count++;
	if (map_file)
		fclose(MAP);
	strbuf_close(sb);
	strbuf_close(url);
	return count;
}
