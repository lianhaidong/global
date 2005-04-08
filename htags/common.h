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
extern const char *html_begin;
extern const char *html_end;
extern const char *html_head_begin;
extern const char *html_head_end;
extern const char *html_title_begin;
extern const char *html_title_end;
extern const char *body_begin;
extern const char *body_end;
extern const char *title_begin;
extern const char *title_end;
extern const char *header_begin;
extern const char *header_end;
extern const char *cvslink_begin;
extern const char *cvslink_end;
extern const char *caution_begin;
extern const char *caution_end;
extern const char *list_begin;
extern const char *item_begin;
extern const char *item_end;
extern const char *list_end;
extern const char *define_list_begin;
extern const char *define_list_end;
extern const char *define_term_begin;
extern const char *define_term_end;
extern const char *define_desc_begin;
extern const char *define_desc_end;
extern const char *table_begin;
extern const char *table_end;
extern const char *verbatim_begin;
extern const char *verbatim_end;
extern const char *comment_begin;
extern const char *comment_end;
extern const char *sharp_begin;
extern const char *sharp_end;
extern const char *brace_begin;
extern const char *brace_end;
extern const char *reserved_begin;
extern const char *reserved_end;
extern const char *position_begin;
extern const char *position_end;
extern const char *warned_line_begin;
extern const char *warned_line_end;
extern const char *error_begin;
extern const char *error_end;
extern const char *message_begin;
extern const char *message_end;
extern const char *string_begin;
extern const char *string_end;
extern const char *quote_great;
extern const char *quote_little;
extern const char *quote_amp;
extern const char *quote_space;
extern const char *hr;
extern const char *br;
extern const char *empty_element;
extern const char *noframes_begin;
extern const char *noframes_end;

/*
 * Parameter for gen_image();
 */
#define CURRENT	0
#define PARENT 1

int fputs_nl(const char *, FILE *);
void setup_html();
void setup_xhtml();
void setup_tex();
const char *meta_record();
const char *upperdir(const char *);
const char *gen_page_begin(const char *, int);
const char *gen_page_end();
const char *gen_image(int, const char *, const char *);
const char *gen_name_number(int);
const char *gen_name_string(const char *);
const char *gen_href_begin_with_title_target(const char *, const char *, const char *, const char *, const char *, const char *);
const char *gen_href_begin_with_title(const char *, const char *, const char *, const char *, const char *);
const char *gen_href_begin(const char *, const char *, const char *, const char *);
const char *gen_href_begin_simple(const char *);
const char *gen_href_end();
const char *gen_list_begin();
const char *gen_list_body(const char *, const char *);
const char *gen_list_end();
const char *gen_div_begin(const char *);
const char *gen_div_end();
const char *gen_form_begin(const char *);
const char *gen_form_end();
const char *gen_input(const char *, const char *, const char *);
const char *gen_input_radio(const char *, const char *, int, const char *);
const char *gen_input_checkbox(const char *, const char *, const char *);
const char *gen_input_with_title_checked(const char *, const char *, const char *, int, const char *);
const char *gen_frameset_begin(const char *);
const char *gen_frameset_end();
const char *gen_frame(const char *, const char *);

#endif /* ! _COMMON_H_ */
