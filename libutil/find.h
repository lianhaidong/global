/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2005, 2012, 2015
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

#ifndef _FIND_H_
#define _FIND_H_

void set_accept_dotfiles(void);
int skipthisfile(const char *);
int issourcefile(const char *);
void find_open(const char *, int);
void find_open_filelist(const char *, const char *, int);
char *find_read(void);
void find_close(void);

#endif /* ! _FIND_H_ */
