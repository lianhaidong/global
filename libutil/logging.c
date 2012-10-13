/*
 * Copyright (c) 2011 Tama Communications Corporation
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
#include <config.h>
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#include "logging.h"

/** @file

Logging utility:

@code
+------------------------------------
|main(int argc, char **argv)
|{
|	logging_printf("Start\n");
|	logging_arguments(argc, argv);
|	...
@endcode

@code{.sh}
% setenv GTAGSLOGGING /tmp/log
% global -x main
% cat /tmp/log
Start
0: |global|
1: |-x|
2: |main|
% _
@endcode

See logging_printf() for more details.
*/
static FILE *lp;
static int ignore;

static int
logging_open(void)
{
	char *logfile = getenv("GTAGSLOGGING");

	if (logfile == NULL || (lp = fopen(logfile, "a")) == NULL)
		return -1;
	return 0;
}
/**
 * logging_printf: print a message into the logging file.
 *
 *	@param[in]	s	@NAME{printf} style format (fmt) string
 *
 *	@remark
 *		Log messages are appended to the logging file; which is opened using 
 *		@CODE{'fopen(xx, \"a\")'} on the first call to logging_printf() or
 *		logging_arguments(). <br>
 *		The logging file's filename should be in the OS environment variable
 *		@FILE{GTAGSLOGGING}. <br>
 *		If @FILE{GTAGSLOGGING} is not setup or the logging file cannot be
 *		opened, logging is disabled; logging_printf() and logging_arguments()
 *		then do nothing.
 *
 *	@note The logging file stays @EMPH{open} for the life of the progam.
 */
void
logging_printf(const char *s, ...)
{
	va_list ap;

	if (ignore)
		return;
	if (lp == NULL && logging_open() < 0) {
		ignore = 1;
		return;
	}
	va_start(ap, s);
	vfprintf(lp, s, ap);
	va_end(ap);
}

/**
 * logging_arguments: print arguments into the logging file.
 *
 *	@param[in]	argc
 *	@param[in]	argv
 *
 *	@par Uses:
 *		logging_printf()
 */
void
logging_arguments(int argc, char **argv)
{
	int i;

	for (i = 0; i < argc; i++) {
		if (ignore)
			break;
		logging_printf("%d: |%s|\n", i, argv[i]);
	}
}
