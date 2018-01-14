/*
 * Copyright (c) 2016
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "die.h"
#include "secure_popen.h"

/*
 * Secure substitution of popen(3)
 */
static pid_t pid;
/**
 * secure_popen
 *
 *	@param[in]	command	command (full path)
 *	@param[in]	type "r": read, "w", write
 *	@return		file descripter
 *
 * The process which can be invoked at the same time is restricted to only one.
 */
FILE *
secure_popen(const char *command, const char *type, char *const argv[])
{
	FILE *ip;
	int fd[2];

	if (pipe(fd) < 0)
		return (NULL);
	pid = fork();
	if (pid == 0) {
		/* child process */
		if (*type == 'r') {
			close(fd[0]);
			if (dup2(fd[1], STDOUT_FILENO) < 0)
				die("dup2 failed.");
		} else {
			close(fd[1]);
			if (dup2(fd[0], STDIN_FILENO) < 0)
				die("dup2 failed.");
		}
		execvp(command, argv);
		die("cannot execute '%s'. (execvp failed)", command);
	}
	/* parent process */
	if (pid < 0)
		die("fork failed.");
	if (*type == 'r') {
		close(fd[1]);
		ip = fdopen(fd[0], "r");
	} else {
		close(fd[0]);
		ip = fdopen(fd[1], "w");
	}
	if (ip == NULL)
		die("fdopen failed.");
	return ip;
}
int
secure_pclose(FILE *ip)
{
	int status, ppid;
	(void)fclose(ip);

	do {
		ppid = waitpid(pid, &status, 0);
	} while (ppid == -1 && errno == EINTR);

	return (ppid == -1 ? -1 : status);
}
