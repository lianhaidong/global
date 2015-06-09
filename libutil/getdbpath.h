/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2008, 2011, 2014, 2015
 *	Tama Communications Corporation
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

#ifndef _GETDBPATH_H_
#define _GETDBPATH_H_

extern char const *gtags_dbpath_error;

char *getobjdir(const char *, int);
int gtagsexist(const char *, char *, int, int);
int setupdbpath(int);
int in_the_project(const char *);
const char *get_dbpath(void);
const char *get_root(void);
const char *get_root_with_slash(void);
const char *get_cwd(void);
const char *get_relative_cwd_with_slash(void);
const char *set_nearbase_path(const char *);
const char *get_nearbase_path(void);
const char *get_normalized_path(const char *, char *, int);
void dump_dbpath(void);

#endif /* ! _GETDBPATH_H_ */
