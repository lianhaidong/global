/*
 * Copyright (c) 1996, 1997, 1998, 1999
 *             Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999, 2000, 2001, 2002
 *             Tama Communications Corporation. All rights reserved.
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

#ifndef _CONF_H_
#define _CONF_H_

#include "strbuf.h"
/*
 * Access library for gtags.conf (.globalrc).
 * File format is a subset of XXXcap (termcap, printcap) file.
 */
#define GTAGSCONF       "/etc/gtags.conf"
#define OLD_GTAGSCONF   "/etc/global.conf"	/* for compatibility */
#define DEBIANCONF      "/etc/gtags/gtags.conf"
#define OLD_DEBIANCONF  "/etc/gtags/global.conf"/* for compatibility */
#define GTAGSRC         ".globalrc"
#define DEFAULTLABEL    "default"
#define DEFAULTSUFFIXES "c,h,y,c++,cc,cpp,cxx,hxx,C,H,s,S,java"
#define DEFAULTSKIP     "GPATH,GTAGS,GRTAGS,GSYMS,HTML/,tags,TAGS,y.tab.c,y.tab.h,HTML/,SCCS/,RCS/,CVS/,.deps/"

void	openconf(void);
int	getconfn(const char *, int *);
int	getconfs(const char *, STRBUF *);
int	getconfb(const char *);
char	*getconfline(void);
void	closeconf(void);

#endif /* ! _CONF_H_ */
