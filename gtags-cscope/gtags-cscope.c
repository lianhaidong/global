/*===========================================================================
 Copyright (c) 1998-2000, The Santa Cruz Operation 
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 *Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 *Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 *Neither name of The Santa Cruz Operation nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE. 
 =========================================================================*/


/**	@file
 *	@NAME{gtags-cscope} - interactive C symbol cross-reference (@NAME{cscope})
 *
 *	main functions
 */

#include "global-cscope.h"

#include "build.h"
#include "version-cscope.h"	/* FILEVERSION and FIXVERSION */

/* for libutil */
#include "env.h"
#include "gparam.h"
#include "path.h"
#include "test.h"
#include "version.h"
/* usage */
#include "const.h"

#include <stdlib.h>	/* atoi */
#include <unistd.h>
#if defined(USE_NCURSES) && !defined(RENAMED_NCURSES)
#include <ncurses.h>
#else
#include <curses.h>
#endif
#include <sys/types.h>	/* needed by stat.h */
#include <sys/stat.h>	/* stat */
#include <signal.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#define S_IRWXG 0070
#define S_IRWXO 0007
#define mkdir(path,mode) mkdir(path)
#endif

/** @name defaults for unset environment variables */
/** @{ */
/** text editor */
#if defined(__DJGPP__) || (defined(_WIN32) && !defined(__CYGWIN__))
#define EDITOR	"tde"
#else
#define EDITOR	"vi"
#endif

/** no @NAME{\$HOME} --\> use root directory */
#define HOME	"/"
/** shell executable */
#define	SHELL	"sh"

/** default: used by @NAME{vi} and @NAME{emacs} */
#define LINEFLAG "+%s"
/** temp dir */
#define TMPDIR	"/tmp"
/** @} */

static char const rcsid[] = "$Id$";

char	*editor, *shell, *lineflag;	/**< environment variables */
char	*global_command;	/**< @FILE{global} by default */
char	*gtags_command;		/**< @FILE{gtags} by default */
char	*home;			/**< Home directory */
BOOL	lineflagafterfile;
char	*argv0;			/**< command name */
int	dispcomponents = 1;	/**< file path components to display */
#if CCS
BOOL	displayversion;		/**< display the C Compilation System version */
#endif
BOOL	editallprompt = YES;	/**< prompt between editing files */
BOOL	incurses = NO;		/**< in @NAME{curses} */
BOOL	isuptodate;		/**< consider the crossref up-to-date */
BOOL	linemode = NO;		/**< use line oriented user interface */
BOOL	verbosemode = NO;	/**< print extra information on line mode */
BOOL	absolutepath = NO;	/**< print absolute path name */
BOOL	ignoresigint = NO;	/**< ignore @NAME{SIGINT} signal */
BOOL	ogs;			/**< display @NAME{OGS} book and subsystem names */
char	*prependpath;		/**< prepend path to file names */
FILE	*refsfound;		/**< references found file */
char	temp1[PATHLEN + 1];	/**< temporary file name */
char	temp2[PATHLEN + 1];	/**< temporary file name */
char	tempdirpv[PATHLEN + 1];	/**< private temp directory */
char	tempstring[TEMPSTRING_LEN + 1]; /**< use this as a buffer, instead of @CODE{yytext}, 
				 * which had better be left alone */
char	*tmpdir;		/**< temporary directory */

static	BOOL	onesearch;		/**< one search only in line mode */
static	char	*reflines;		/**< symbol reference lines file */

/* Internal prototypes: */
static	void	longusage(void);
static	void	usage(void);
int	qflag;

#ifdef HAVE_FIXKEYPAD
void	fixkeypad();
#endif

#if defined(KEY_RESIZE) && defined(SIGWINCH)
void 
sigwinch_handler(int sig, siginfo_t *info, void *unused)
{
    (void) sig;
    (void) info;
    (void) unused;
    if(incurses == YES)
        ungetch(KEY_RESIZE);
}
#endif

int
main(int argc, char **argv)
{
    char *s;
    int c;
    pid_t pid;
    struct stat	stat_buf;
    mode_t orig_umask;
#if defined(KEY_RESIZE) && defined(SIGWINCH)
    struct sigaction winch_action;
#endif
	
    /* save the command name for messages */
    argv0 = argv[0];

    /* set the options */
    while (--argc > 0 && (*++argv)[0] == '-') {
	/* HBB 20030814: add GNU-style --help and --version options */
	if (strequal(argv[0], "--help")
	    || strequal(argv[0], "-h")) {
	    longusage();
	    myexit(0);
	}
	if (strequal(argv[0], "--version")
	    || strequal(argv[0], "-V")) {
#if CCS
	    displayversion = YES;
#else
	    fprintf(stderr, "%s: %s (based on cscope version %d%s)\n", argv0, get_version(),
		    FILEVERSION, FIXVERSION);
	    myexit(0);
#endif
	}

	for (s = argv[0] + 1; *s != '\0'; s++) {
			
	    /* look for an input field number */
	    if (isdigit((unsigned char) *s)) {
		field = *s - '0';
		if (field > 8) {
		    field = 8;
		}
		if (*++s == '\0' && --argc > 0) {
		    s = *++argv;
		}
		if (strlen(s) > PATLEN) {
		    postfatal("\
gtags-cscope: pattern too long, cannot be > %d characters\n", PATLEN);
		    /* NOTREACHED */
		}
		strcpy(Pattern, s);
		goto nextarg;
	    }
	    switch (*s) {
	    case '-':	/* end of options */
		--argc;
		++argv;
		goto lastarg;
	    case 'a':	/* absolute path name */
		absolutepath = YES;
		break;
	    case 'b':	/* only build the cross-reference */
		buildonly = YES;
		linemode  = YES;
		break;
	    case 'c':	/* ASCII characters only in crossref */
		/* N/A */
		break;
	    case 'C':	/* turn on caseless mode for symbol searches */
		caseless = YES;
		break;
	    case 'd':	/* consider crossref up-to-date */
		isuptodate = YES;
		break;
	    case 'e':	/* suppress ^E prompt between files */
		editallprompt = NO;
		break;
	    case 'i':	/* ignore SIGINT signal */
		ignoresigint = YES;
		break;
	    case 'k':	/* ignore DFLT_INCDIR */
		/* N/A */
		break;
	    case 'L':
		onesearch = YES;
		/* FALLTHROUGH */
	    case 'l':
		linemode = YES;
		break;
	    case 'v':
		verbosemode = YES;
		break;
	    case 'o':	/* display OGS book and subsystem names */
		ogs = YES;
		break;
	    case 'q':	/* quick search */
		/* N/A */
		break;
	    case 'T':	/* truncate symbols to 8 characters */
		/* N/A */
		break;
	    case 'u':	/* unconditionally build the cross-reference */
		/* N/A */
		break;
	    case 'U':	/* assume some files have changed */
		/* N/A */
		break;
	    case 'R':
		usage();
		break;
	    case 'f':	/* alternate cross-reference file */
	    case 'F':	/* symbol reference lines file */
/*	    case 'i':	/* file containing file names */
	    case 'I':	/* #include file directory */
	    case 'p':	/* file path components to display */
	    case 'P':	/* prepend path to file names */
	    case 's':	/* additional source file directory */
	    case 'S':
		c = *s;
		if (*++s == '\0' && --argc > 0) {
		    s = *++argv;
		}
		if (*s == '\0') {
		    fprintf(stderr, "%s: -%c option: missing or empty value\n", 
			    argv0, c);
		    goto usage;
		}
		switch (c) {
		case 'f':	/* alternate cross-reference file (default: cscope.out) */
		    /* N/A */
		    break;
		case 'F':	/* symbol reference lines file */
		    reflines = s;
		    break;
		case 'i':	/* file containing file names (default: cscope.files) */
		    /* N/A */
		    break;
		case 'I':	/* #include file directory */
		    /* N/A */
		    break;
		case 'p':	/* file path components to display */
		    if (*s < '0' || *s > '9' ) {
			fprintf(stderr, "\
%s: -p option: missing or invalid numeric value\n", 
				argv0);
			goto usage;
		    }
		    dispcomponents = atoi(s);
		    break;
		case 'P':	/* prepend path to file names */
		    /* N/A */
		    break;
		case 's':	/* additional source directory */
		case 'S':
		    /* N/A */
		    break;
		}
		goto nextarg;
	    default:
		fprintf(stderr, "%s: unknown option: -%c\n", argv0, 
			*s);
	    usage:
		usage();
		fprintf(stderr, "Try the -h option for more information.\n");
		myexit(1);
	    } /* switch(option letter) */
	} /* for(option) */
    nextarg:	
	;
    } /* while(argv) */

 lastarg:
    /* read the environment */
    editor = mygetenv("EDITOR", EDITOR);
    editor = mygetenv("VIEWER", editor); /* use viewer if set */
    editor = mygetenv("CSCOPE_EDITOR", editor);	/* has last word */
    home = mygetenv("HOME", HOME);
    global_command = mygetenv("GTAGSGLOBAL", "global");
    gtags_command = mygetenv("GTAGSGTAGS", "gtags");
#if defined(_WIN32) || defined(__DJGPP__)
    shell = mygetenv("COMSPEC", SHELL);
    shell = mygetenv("SHELL", shell);
    tmpdir = mygetenv("TMP", TMPDIR);
    tmpdir = mygetenv("TMPDIR", tmpdir);
#else
    shell = mygetenv("SHELL", SHELL);
    tmpdir = mygetenv("TMPDIR", TMPDIR);
#endif
    lineflag = mygetenv("CSCOPE_LINEFLAG", LINEFLAG);
    lineflagafterfile = getenv("CSCOPE_LINEFLAG_AFTER_FILE") ? 1 : 0;

    /* make sure that tmpdir exists */
    if (lstat (tmpdir, &stat_buf)) {
	fprintf (stderr, "\
cscope: Temporary directory %s does not exist or cannot be accessed\n", 
		 tmpdir);
	fprintf (stderr, "\
cscope: Please create the directory or set the environment variable\n\
cscope: TMPDIR to a valid directory\n");
	myexit(1);
    }

    /* create the temporary file names */
    orig_umask = umask(S_IRWXG|S_IRWXO);
    pid = getpid();
    snprintf(tempdirpv, sizeof(tempdirpv), "%s/cscope.%d", tmpdir, pid);
    if(mkdir(tempdirpv,S_IRWXU)) {
	fprintf(stderr, "\
cscope: Could not create private temp dir %s\n",
		tempdirpv);
	myexit(1);
    }
    umask(orig_umask);

    snprintf(temp1, sizeof(temp1), "%s/cscope.1", tempdirpv);
    snprintf(temp2, sizeof(temp2), "%s/cscope.2", tempdirpv);

    /* if running in the foreground */
    if (signal(SIGINT, SIG_IGN) != SIG_IGN && ignoresigint == NO) {
	/* cleanup on the interrupt and quit signals */
	signal(SIGINT, myexit);
#ifdef SIGQUIT
	signal(SIGQUIT, myexit);
#endif
    }
    /* cleanup on the hangup signal */
#ifdef SIGHUP
    signal(SIGHUP, myexit);
#endif

    /* ditto the TERM signal */
    signal(SIGTERM, myexit);

    if (linemode == NO) {
	signal(SIGINT, SIG_IGN);	/* ignore interrupts */
#ifdef SIGPIPE
	signal(SIGPIPE, SIG_IGN);/* | command can cause pipe signal */
#endif

#if defined(KEY_RESIZE) && defined(SIGWINCH)
	winch_action.sa_sigaction = sigwinch_handler;
	sigemptyset(&winch_action.sa_mask);
	winch_action.sa_flags = SA_SIGINFO;
	sigaction(SIGWINCH,&winch_action,NULL);
#endif

	/* initialize the curses display package */
	initscr();	/* initialize the screen */
	entercurses();
#if TERMINFO
	keypad(stdscr, TRUE);	/* enable the keypad */
# ifdef HAVE_FIXKEYPAD
	fixkeypad();	/* fix for getch() intermittently returning garbage */
# endif
#endif /* TERMINFO */
#if UNIXPC
	standend();	/* turn off reverse video */
#endif
	dispinit();	/* initialize display parameters */
	setfield();	/* set the initial cursor position */
	clearmsg();	/* clear any build progress message */
	display();	/* display the version number and input fields */
    }

    /* if the cross-reference is to be considered up-to-date */
    if (isuptodate == YES) {
	char com[80];
	snprintf(com, sizeof(com), "%s -p >" NULL_DEVICE, global_command);
	if (system(com) != 0) {
	    postfatal("gtags-cscope: GTAGS not found. Please invoke again without -d option.\n");
            /* NOTREACHED */
	}
    } else {
	char buf[MAXPATHLEN];

	if (linemode == NO || verbosemode == YES)    /* display if verbose as well */
	    postmsg("Building cross-reference...");                 
	rebuild();
	if (linemode == NO )
            clearmsg(); /* clear any build progress message */
	if (buildonly == YES) {
	    myexit(0);
	}
	set_env("GTAGSROOT", getcwd(buf, sizeof(buf)));
	set_env("GTAGSDBPATH", getcwd(buf, sizeof(buf)));
    }

    /* opendatabase(); */

    /* if using the line oriented user interface so cscope can be a 
       subprocess to emacs or samuel */
    if (linemode == YES) {
	if (*Pattern != '\0') {		/* do any optional search */
	    if (search() == YES) {
		/* print the total number of lines in
		 * verbose mode */
		if (verbosemode == YES)
		    printf("cscope: %d lines\n",
			   totallines);

		while ((c = getc(refsfound)) != EOF)
		    putchar(c);
	    }
	}
	if (onesearch == YES)
	    myexit(0);
		
	for (;;) {
	    char buf[PATLEN + 2];
			
	    printf(">> ");
	    fflush(stdout);
	    if (fgets(buf, sizeof(buf), stdin) == NULL) {
		myexit(0);
	    }
	    /* remove any trailing newline character */
	    if (*(s = buf + strlen(buf) - 1) == '\n') {
		*s = '\0';
	    }
	    switch (*buf) {
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
		field = *buf - '0';
		strcpy(Pattern, buf + 1);
		search();
		printf("cscope: %d lines\n", totallines);
		while ((c = getc(refsfound)) != EOF) {
		    putchar(c);
		}
		break;

	    case 'c':	/* toggle caseless mode */
	    case ctrl('C'):
		if (caseless == NO) {
		    caseless = YES;
		} else {
		    caseless = NO;
		}
		break;

	    case 'r':	/* rebuild database cscope style */
	    case ctrl('R'):
		rebuild();
		putchar('\n');
		break;

	    case 'C':	/* clear file names */
		/* N/A */
		putchar('\n');
		break;

	    case 'F':	/* add a file name */
		/* N/A */
		putchar('\n');
		break;

	    case 'q':	/* quit */
	    case ctrl('D'):
	    case ctrl('Z'):
		myexit(0);

	    default:
		fprintf(stderr, "gtags-cscope: unknown command '%s'\n", buf);
		break;
	    }
	}
	/* NOTREACHED */
    }
    /* do any optional search */
    if (*Pattern != '\0') {
	atfield();		/* move to the input field */
	command(ctrl('Y'));	/* search */
    } else if (reflines != NULL) {
	/* read any symbol reference lines file */
	readrefs(reflines);
    }
    display();		/* update the display */

    for (;;) {
	if (!selecting)
	    atfield();	/* move to the input field */

	/* exit if the quit command is entered */
	if ((c = mygetch()) == EOF || c == ctrl('D') || c == ctrl('Z')) {
	    break;
	}
	/* execute the commmand, updating the display if necessary */
	if (command(c) == YES) {
	    display();
	}

	if (selecting) {
	    move(displine[curdispline], 0);
	    refresh();
	}
    }
    /* cleanup and exit */
    myexit(0);
    /* NOTREACHED */
    return 0;		/* avoid warning... */
}

void
cannotopen(char *file)
{
    posterr("Cannot open file %s", file);
}

/* FIXME MTE - should use postfatal here */
void
cannotwrite(char *file)
{
    char	msg[MSGLEN + 1];

    snprintf(msg, sizeof(msg), "Removed file %s because write failed", file);
    myperror(msg);	/* display the reason */

    unlink(file);
    myexit(1);	/* calls exit(2), which closes files */
}

/** enter curses mode */
void
entercurses(void)
{
    incurses = YES;
#ifndef __MSDOS__ /* HBB 20010313 */
    nonl();		    /* don't translate an output \n to \n\r */
#endif
    raw();			/* single character input */
    noecho();			/* don't echo input characters */
    clear();			/* clear the screen */
    mouseinit();		/* initialize any mouse interface */
    drawscrollbar(topline, nextline);
}


/** exit curses mode */
void
exitcurses(void)
{
	/* clear the bottom line */
	move(LINES - 1, 0);
	clrtoeol();
	refresh();

	/* exit curses and restore the terminal modes */
	endwin();
	incurses = NO;

	/* restore the mouse */
	mousecleanup();
	fflush(stdout);
}


/** normal usage message */
static void
usage(void)
{
	fputs(usage_const, stderr);
}


/** long usage message */
static void
longusage(void)
{
	fputs(usage_const, stdout);
        fputs(help_const, stdout);
}

/** cleanup and exit */

void
myexit(int sig)
{
	/* HBB 20010313; close file before unlinking it. Unix may not care
	 * about that, but DOS absolutely needs it */
	if (refsfound != NULL)
		fclose(refsfound);
	
	/* remove any temporary files */
	if (temp1[0] != '\0') {
		unlink(temp1);
		unlink(temp2);
		rmdir(tempdirpv);
	}
	/* restore the terminal to its original mode */
	if (incurses == YES) {
		exitcurses();
	}
	/* dump core for debugging on the quit signal */
#ifdef SIGQUIT
	if (sig == SIGQUIT) {
		abort();
	}
#endif
	/* HBB 20000421: be nice: free allocated data */
	exit(sig);
}
