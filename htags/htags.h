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
#ifndef _HTAGS_H_
#define _HTAGS_H_

#include "gparam.h"

#if defined(_WIN32) || defined(__DJGPP__)
#define W32	1
#define NULL_DEVICE	"NUL"
#else
#define W32	0
#define NULL_DEVICE	"/dev/null"
#endif

/*
 * definition_header
 */
#define NO_HEADER	0
#define BEFORE_HEADER	1
#define RIGHT_HEADER	2
#define AFTER_HEADER	3

/*
 * Directory names.
 */
#define SRCS	"S"
#define DEFS	"D"
#define REFS	"R"
#define INCS	"I"
#define INCREFS "J"
#define SYMS	"Y"

extern int w32;
extern char *www;
extern char *include_header;
extern int file_count;
extern int sep;
extern int exitflag;
extern char *save_config;
extern char *save_argv;

extern char cwdpath[MAXPATHLEN];
extern char dbpath[MAXPATHLEN];
extern char distpath[MAXPATHLEN];
extern char gtagsconf[MAXPATHLEN];

extern char sort_path[MAXFILLEN];
extern char gtags_path[MAXFILLEN];
extern char global_path[MAXFILLEN];
extern char findcom[MAXFILLEN];
extern char *null_device;
extern char *tmpdir;

extern int aflag;
extern int cflag;
extern int fflag;
extern int Fflag;
extern int nflag;
extern int gflag;
extern int Sflag;
extern int qflag;
extern int vflag;
extern int wflag;
extern int debug;

extern int show_help;
extern int show_version;
extern int caution;
extern int dynamic;
extern int symbol;
extern int statistics;

extern int no_map_file;
extern int no_order_list;
extern int other_files;
extern int enable_grep;
extern int enable_idutils;

extern char *action_value;
extern char *id_value;
extern char *cgidir;
extern char *main_func;
extern char *style_sheet;
extern char *cvsweb_url;
extern char *cvsweb_cvsroot;
extern char *gtagslabel;
extern char *title;

extern char *title_define_index;
extern char *title_file_index;
extern char *title_included_from;

extern char *anchor_label[];
extern char *anchor_icons[];
extern char *anchor_comment[];
extern char *anchor_msg[];
extern char *back_icon;
extern char *dir_icon;
extern char *c_icon;
extern char *file_icon;

extern int ncol;
extern int tabs;
extern char stabs[];
extern int full_path;
extern int map_file;
extern char *icon_list;
extern char *icon_suffix;
extern char *icon_spec;
extern char *prolog_script;
extern char *epilog_script;
extern int show_position;
extern int table_list;
extern int colorize_warned_line;
extern char *script_alias;
extern char *gzipped_suffix;
extern char *normal_suffix;
extern char *HTML;
extern char *action;
extern char *saction;
extern char *id;
extern int cgi;
extern int definition_header;
extern char *htags_options;
extern char *include_file_suffixes;

#endif /* _HTAGS_H_ */
