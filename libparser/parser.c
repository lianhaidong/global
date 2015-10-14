/*
 * Copyright (c) 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2010, 2011,
 *	2015
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
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <ltdl.h>
#if defined(_WIN32) && !defined(__CYGWIN__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef SLIST_ENTRY
#endif

#include "parser.h"
#include "internal.h"
#include "checkalloc.h"
#include "die.h"
#include "langmap.h"
#include "locatestring.h"
#include "queue.h"
#include "strbuf.h"
#include "test.h"

#define NOTFUNCTION	".notfunction"
#ifdef __DJGPP__
#define DOS_NOTFUNCTION	"_notfunction"
#endif

struct words {
	const char *name;
};
static struct words *words;
static int tablesize;

static int
cmp(const void *s1, const void *s2)
{
	return strcmp(((struct words *)s1)->name, ((struct words *)s2)->name);
}

static void
load_notfunction(const char *filename)
{
	FILE *ip;
	STRBUF *sb = strbuf_open(0);
	STRBUF *ib = strbuf_open(0);
	char *p;
	int i;

	if ((ip = fopen(filename, "r")) == NULL)
		die("'%s' cannot read.", filename);
	for (tablesize = 0; (p = strbuf_fgets(ib, ip, STRBUF_NOCRLF)) != NULL; tablesize++)
		strbuf_puts0(sb, p);
	fclose(ip);
	words = (struct words *)check_malloc(sizeof(struct words) * tablesize);
	/*
	 * Don't free *p.
	 */
	p = (char *)check_malloc(strbuf_getlen(sb) + 1);
	memcpy(p, strbuf_value(sb), strbuf_getlen(sb) + 1);
	for (i = 0; i < tablesize; i++) {
		words[i].name = p;
		p += strlen(p) + 1;
	}
	qsort(words, tablesize, sizeof(struct words), cmp);
	strbuf_close(sb);
	strbuf_close(ib);
}

static int
isnotfunction(const char *name)
{
	struct words tmp;
	struct words *result;

	if (words == NULL)
		return 0;
	tmp.name = name;
	result = (struct words *)bsearch(&tmp, words, tablesize, sizeof(struct words), cmp);
	return (result != NULL) ? 1 : 0;
}

/*----------------------------------------------------------------------*/
/* Parser switch                                                        */
/*----------------------------------------------------------------------*/
/**
 * This is the linkage section of each parsers.
 * If you want to support new language, you must define parser procedure
 * which requires file name as an argument.
 */
struct lang_entry {
	const char *lang_name;
	void (*parser)(const struct parser_param *);	/**< parser procedure */
	const char *parser_name;
	const char *lt_dl_name;
};

struct plugin_entry {
	STAILQ_ENTRY(plugin_entry) next;
	lt_dlhandle handle;
	struct lang_entry entry;
};

#ifndef IS__DOXYGEN_
static STAILQ_HEAD(plugin_list, plugin_entry)
	plugin_list = STAILQ_HEAD_INITIALIZER(plugin_list);

#else
static struct plugin_list {
	struct plugin_entry *stqh_first; /**< first element */
	struct plugin_entry **stqh_last; /**< addr of last next element */
} plugin_list = { NULL, &(plugin_list).stqh_first };
#endif
static char *langmap_saved, *pluginspec_saved;

/**
 * load_plugin_parser: Load plug-in parsers.
 *
 *	@param[in]	pluginspec	described below
 *
 * Syntax:
 *   <pluginspec> ::= <map> | <map>","<pluginspec>
 *   <map>        ::= <language name>":"<shared object path>
 *                  | <language name>":"<shared object path>":"<function name>
 */
static void
load_plugin_parser(const char *pluginspec)
{
	char *p, *q;
	const char *lt_dl_name, *parser_name;
	struct plugin_entry *pent;

	pluginspec_saved = check_strdup(pluginspec);
	if (lt_dlinit() != 0)
		die("cannot initialize libltdl.");
	p = pluginspec_saved;
	while (*p != '\0') {
		pent = check_malloc(sizeof(*pent));
		pent->entry.lang_name = p;
		p = strchr(p, ':');
		if (p == NULL)
			die_with_code(2, "syntax error in pluginspec '%s'.", pluginspec);
		*p++ = '\0';
		if (*p == '\0')
			die_with_code(2, "syntax error in pluginspec '%s'.", pluginspec);
		lt_dl_name = p;
		p = strchr(p, ',');
		if (p != NULL)
			*p++ = '\0';
		q = strchr(lt_dl_name, ':');
#ifdef _WIN32
		/* Assume a single-character name is a drive letter. */
		if (q == lt_dl_name + 1)
			q = strchr(q + 1, ':');
#endif
		if (q == NULL) {
			parser_name = "parser";
		} else {
			*q++ = '\0';
			if (*q == '\0')
				die_with_code(2, "syntax error in pluginspec '%s'.", pluginspec);
			parser_name = q;
		}
#if defined(_WIN32) && !defined(__CYGWIN__)
		/* Bypass libtool and load the DLL directly, relative to us. */
		pent->handle = (lt_dlhandle)LoadLibrary(lt_dl_name);
		if (pent->handle == NULL) {
			q = strrchr(lt_dl_name, '/');
			if (q == NULL)
				q = strrchr(lt_dl_name, '\\');
			if (q != NULL)
				lt_dl_name = q + 1;
			q = strrchr(lt_dl_name, '.');
			if (q != NULL && strcmp(q, ".la") == 0)
				*q = '\0';
			pent->handle = (lt_dlhandle)LoadLibrary(lt_dl_name);
			if (pent->handle == NULL) {
				char dll[MAX_PATH*2];
				GetModuleFileName(NULL, dll, MAX_PATH);
				q = strrchr(dll, '\\');
				sprintf(q+1, "..\\lib\\gtags\\%s", lt_dl_name);
				pent->handle = (lt_dlhandle)LoadLibrary(dll);
			}
		}
		if (pent->handle == NULL)
			die_with_code(2, "cannot open shared object '%s'.", lt_dl_name);
		pent->entry.lt_dl_name = lt_dl_name;
		pent->entry.parser = (PVOID)GetProcAddress((HINSTANCE)pent->handle, parser_name);
#else
		pent->handle = lt_dlopen(lt_dl_name);
		if (pent->handle == NULL) {
			/*
			 * Retry after removing the extension, because some packages
			 * don't have '.la' files.
			 */
			q = strrchr(lt_dl_name, '.');
			if (q)
				*q = '\0';
			pent->handle = lt_dlopenext(lt_dl_name);
			if (pent->handle == NULL)
				die_with_code(2, "cannot open shared object '%s'.", lt_dl_name);
		}
		pent->entry.lt_dl_name = lt_dl_name;
		pent->entry.parser = lt_dlsym(pent->handle, parser_name);
#endif
		if (pent->entry.parser == NULL)
			die_with_code(2, "cannot find symbol '%s' in '%s'.", parser_name, lt_dl_name);
		pent->entry.parser_name = parser_name;
		STAILQ_INSERT_TAIL(&plugin_list, pent, next);
		if (p == NULL)
			break;
	}
}

/**
 * unload_plugin_parser: Unload plug-in parsers.
 */
static void
unload_plugin_parser(void)
{
	struct plugin_entry *pent;

	if (pluginspec_saved == NULL)
		return;
	while (!STAILQ_EMPTY(&plugin_list)) {
		pent = STAILQ_FIRST(&plugin_list);
#if defined(_WIN32) && !defined(__CYGWIN__)
		FreeLibrary((HMODULE)pent->handle);
#else
		lt_dlclose(pent->handle);
#endif
		STAILQ_REMOVE_HEAD(&plugin_list, next);
		free(pent);
	}
	lt_dlexit();
	free(pluginspec_saved);
}

/**
 * The first entry is default language.
 */
static const struct lang_entry lang_switch[] = {
	/* lang_name    parser_proc	parser_name	lt_dl_name,	*/
	/*				(for debug)	(for debug)	*/
	{"c",		C,		"C",		"built-in"},	/* DEFAULT */
	{"yacc",	yacc,		"yacc",		"built-in"},
	{"cpp",		Cpp,		"Cpp",		"built-in"},
	{"java",	java,		"java",		"built-in"},
	{"php",		php,		"php",		"built-in"},
	{"asm",		assembly,	"assembly",	"built-in"}
};
#define DEFAULT_ENTRY &lang_switch[0]
/**
 * get language entry.
 *
 *      @param[in]      lang    language name (NULL means 'not specified'.)
 *      @return              language entry
 */
static const struct lang_entry *
get_lang_entry(const char *lang)
{
	int i, size = sizeof(lang_switch) / sizeof(struct lang_entry);
	struct plugin_entry *pent;

	if (lang == NULL)
		die("get_lang_entry: something is wrong.");
	/*
	 * Priority 1: locates in the plugin parser list.
	 */
	STAILQ_FOREACH(pent, &plugin_list, next) {
		if (strcmp(lang, pent->entry.lang_name) == 0)
			return &pent->entry;
	}
	/*
	 * Priority 2: locates in the built-in parser list.
	 */
	for (i = 0; i < size; i++)
		if (!strcmp(lang, lang_switch[i].lang_name))
			return &lang_switch[i];
	/*
	 * if specified language not found, it assumes default language, that is C.
	 */
	return DEFAULT_ENTRY;
}

/**
 * Usage:
 * [gtags.conf]
 * +----------------------------
 * |...
 * |gtags_parser=<pluginspec>
 * |langmap=<langmap>
 *
 * 1. Load langmap and pluginspec, and initialize parsers.
 *
 *	parser_init(langmap, plugin_parser);
 *
 * 2. Execute parsers
 *
 *	parse_file(...);
 *
 * 3. Unload parsers.
 *
 *	parser_exit();
 */
/*
 * parser_init: load langmap and shared libraries.
 *
 *	@param[in]	langmap		the value of langmap=<langmap>
 *	@param[in]	pluginspec	the value of gtags_parser=<pluginspec>
 */
void
parser_init(const char *langmap, const char *pluginspec)
{
	/* setup language mapping. */
	if (langmap == NULL)
		langmap = DEFAULTLANGMAP;
	langmap = trim_langmap(langmap);
	setup_langmap(langmap);
	langmap_saved = check_strdup(langmap);

	/* load shared objects. */
	if (pluginspec != NULL)
		load_plugin_parser(pluginspec);

	/*
	 * This is a hack for FreeBSD.
	 * In the near future, it will be removed.
	 */
	if (test("r", NOTFUNCTION))
		load_notfunction(NOTFUNCTION);
#ifdef __DJGPP__
	else if (test("r", DOS_NOTFUNCTION))
		load_notfunction(DOS_NOTFUNCTION);
#endif
}

/**
 * parser_exit: unload shared libraries.
 */
void
parser_exit(void)
{
	unload_plugin_parser();
	free(langmap_saved);
}

/**
 * parse_file: select and execute a parser.
 *
 *	@param[in]	path	path name
 *	@param[in]	flags	PARSER_WARNING: print warning messages
 *	@param[in]	put	callback routine,
 *			each parser use this routine for output
 *	@param[in]	arg	argument for callback routine
 */
void
parse_file(const char *path, int flags, PARSER_CALLBACK put, void *arg)
{
	const char *lang, *suffix;
	const struct lang_entry *ent;
	struct parser_param param;

	/* get suffix of the path. */
	suffix = locatestring(path, ".", MATCH_LAST);
	if (suffix == NULL)
		return;
	lang = decide_lang(suffix);
	if (lang == NULL)
		return;
	/*
	 * Select parser.
	 * If lang == NULL then default parser is selected.
	 */
	ent = get_lang_entry(lang);
	if (flags & PARSER_EXPLAIN) {
		fprintf(stderr, " File '%s' is handled as follows:\n", path);
		fprintf(stderr, "\tsuffix:   |%s|\n", suffix);
		fprintf(stderr, "\tlanguage: |%s|\n", lang);
		fprintf(stderr, "\tparser:   |%s|\n", ent->parser_name);
		fprintf(stderr, "\tlibrary:  |%s|\n", ent->lt_dl_name ? ent->lt_dl_name : "builtin library");
	}
	/*
	 * call language specific parser.
	 */
	param.size = sizeof(param);
	param.flags = flags;
	param.file = path;
	param.put = put;
	param.arg = arg;
	param.isnotfunction = isnotfunction;
	param.langmap = langmap_saved;
	param.die = die;
	param.warning = warning;
	param.message = message;
	ent->parser(&param);
}

void
dbg_print(int level, const char *s)
{
	fprintf(stderr, "[%04d]", lineno);
	for (; level > 0; level--)
		fprintf(stderr, "    ");
	fprintf(stderr, "%s\n", s);
}
