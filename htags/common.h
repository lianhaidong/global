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
#ifndef _COMMON_H_
#define _COMMON_H_

/*
 * tag
 */
extern char *html_begin;
extern char *html_end;
extern char *html_head_begin;
extern char *html_head_end;
extern char *html_title_begin;
extern char *html_title_end;
extern char *body_begin;
extern char *body_end;
extern char *title_begin;
extern char *title_end;
extern char *header_begin;
extern char *header_end;
extern char *cvslink_begin;
extern char *cvslink_end;
extern char *caution_begin;
extern char *caution_end;
extern char *list_begin;
extern char *item_begin;
extern char *item_end;
extern char *list_end;
extern char *define_list_begin;
extern char *define_list_end;
extern char *define_term_begin;
extern char *define_term_end;
extern char *define_desc_begin;
extern char *define_desc_end;
extern char *table_begin;
extern char *table_end;
extern char *verbatim_begin;
extern char *verbatim_end;
extern char *comment_begin;
extern char *comment_end;
extern char *sharp_begin;
extern char *sharp_end;
extern char *brace_begin;
extern char *brace_end;
extern char *reserved_begin;
extern char *reserved_end;
extern char *position_begin;
extern char *position_end;
extern char *warned_line_begin;
extern char *warned_line_end;
extern char *error_begin;
extern char *error_end;
extern char *message_begin;
extern char *message_end;
extern char *string_begin;
extern char *string_end;
extern char *quote_great;
extern char *quote_little;
extern char *quote_amp;
extern char *quote_space;
extern char *hr;
extern char *br;
extern char *empty_element;

/*
 * Parameter for gen_image();
 */
#define CURRENT	0
#define PARENT 1

void setup_html();
void setup_xhtml();
void setup_tex();
char *meta_record();
char *upperdir(const char *);
char *gen_page_begin(char *, int);
char *gen_page_end();
char *gen_image(int, const char *, const char *);
char *gen_name_number(int);
char *gen_name_string(const char *);
char *gen_href_begin_with_title_target(const char *, const char *, const char *, const char *, const char *, const char *);
char *gen_href_begin_with_title(const char *, const char *, const char *, const char *, const char *);
char *gen_href_begin(const char *, const char *, const char *, const char *);
char *gen_href_begin_simple(const char *);
char *gen_href_end();
char *gen_list_begin();
char *gen_list_body(char *, char *);
char *gen_list_end();
char *gen_div_begin(const char *);
char *gen_div_end();
int is_binary(char *);

#endif /* ! _COMMON_H_ */
