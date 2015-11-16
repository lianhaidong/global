/*
 * Copyright (c) 2014, 2015
 *      Tama Communications Corporation
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
#ifndef _NEARSORT_H_
#define _NEARSORT_H_

#define COMPARE_NEARNESS(p1, p2, c) (get_nearness(p2, c) - get_nearness(p1, c))

const char *set_nearbase_path(const char *);
const char *get_nearbase_path(void);
int get_nearness(const char *, const char *);
int compare_nearpath(const void *, const void *);

#endif /* ! _NEARSORT_H_ */
