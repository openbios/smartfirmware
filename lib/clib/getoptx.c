/*  Copyright (c) 1992-2005 CodeGen, Inc.  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Redistributions in any form must be accompanied by information on
 *     how to obtain complete source code for the CodeGen software and any
 *     accompanying software that uses the CodeGen software.  The source code
 *     must either be included in the distribution or be available for no
 *     more than the cost of distribution plus a nominal fee, and must be
 *     freely redistributable under reasonable conditions.  For an
 *     executable file, complete source code means the source code for all
 *     modules it contains.  It does not include source code for modules or
 *     files that typically accompany the major components of the operating
 *     system on which the executable file runs.  It does not include
 *     source code generated as output by a CodeGen compiler.
 *
 *  THIS SOFTWARE IS PROVIDED BY CODEGEN AS IS AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  (Commercial source/binary licensing and support is also available.
 *   Please contact CodeGen for details. http://www.codegen.com/)
 */

/* Copyright (c) 1993,1994,1997-1998 by Parag Patel and Thomas J. Merritt.
   All Rights Reserved. */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include "getoptx.h"

typedef int boolean;
#define TRUE 1
#define FALSE 0

#define strfree(str)    ((str) == NULL ? 0 : (free((void*)(str)), 0))
#define strnew(len)     ((char*)malloc((len) + 1))
#define strdup(str)     (((str) == NULL) ? NULL : \
				strcpy((char*)malloc(strlen(str) + 1), (str)))
#define streq(s1, s2)   (strcmp(s1, s2) == 0)

/* used to resize a dynamic array - grow it in BUMP chunks */
#define BUMP 32

#if !defined(_DJGPP_SOURCE) && !defined(unix)
int optind = 0;
char *optarg = NULL;
#endif
const char *optname = NULL;
int optsign;

/* not all compilers have enums */
#define NONE	0
#define PARAM	1
#define NUMERIC	2
typedef int Params;

typedef struct Optentry
{
	const char *name;		/* option name */
	int len;				/* minimum length of name to match or zero */
	Params params;			/* type of parameter for option */
	int code;				/* id value to return if this option is found */
} Optentry;

typedef struct Opttable
{
	Optentry *optlist;		/* simply a sorted array of options */
	int optlen;				/* allocated length of array */
	int optnum;				/* number of options < optlen */
	boolean sorted;			/* has array been sorted or not? */
} Opttable;

/* global vars to maintain state between calls to getoptx() */
static Opttable *minustab = NULL;
static Opttable *plustab = NULL;

static int lastargc = 0;
static const char **lastargv = NULL;
static const char *lastminus = NULL;
static const char *lastplus = NULL;

static Opttable *lasttab = NULL;
static const char *singlechars = NULL;

static boolean endflag = TRUE;

static Opttable *
new_opttable(void)
{
	Opttable *tab = (Opttable *)malloc(sizeof (Opttable));

	if (tab == NULL)
		return NULL;

	tab->optlist = NULL;
	tab->optlen = 0;
	tab->optnum = 0;
	tab->sorted = FALSE;
	return tab;
}

static void
clear_opttable(Opttable *tab)
{
	int i;

	if (tab == NULL || tab->optlist == NULL)
		return;

	for (i = 0; i < tab->optnum; i++)
		strfree(tab->optlist[i].name);

	free(tab->optlist);
	tab->optlist = NULL;
	tab->optlen = 0;
	tab->optnum = 0;
	tab->sorted = FALSE;
}

static void
insert_opttable(Opttable *tab, Optentry *key)
{
	if (tab == NULL)
		return;

	if (tab->optnum >= tab->optlen)
	{
		int nlen;
		Optentry *nlist;

		nlen = tab->optlen + BUMP - tab->optlen % BUMP;
		nlist = (Optentry *)malloc(nlen * sizeof (Optentry));

		if (nlist == NULL)
			return;

		if (tab->optlist != NULL)
		{
			memcpy((void *)nlist, (void *)tab->optlist,
					tab->optlen * sizeof (Optentry));
			free(tab->optlist);
		}

		tab->optlist = nlist;
		tab->optlen = nlen;
	}

	tab->optlist[tab->optnum].name = strdup(key->name);
	tab->optlist[tab->optnum].len = key->len;
	tab->optlist[tab->optnum].params = key->params;
	tab->optlist[tab->optnum].code = key->code;
	tab->optnum += 1;
	tab->sorted = FALSE;
}

static int
qsort_cmp_optentry(const void *v1, const void *v2)
{
	Optentry *e1 = (Optentry *)v1;
	Optentry *e2 = (Optentry *)v2;

	return strcmp(e1->name, e2->name);
}

static int
bsearch_cmp_optentry(const void *v1, const void *v2)
{
	Optentry *o1 = (Optentry *)v1;
	Optentry *o2 = (Optentry *)v2;

#ifdef DEBUG
	fprintf(stderr, "bsearch_cmp: o1=%s o1-len=%d o2=%s o2-len=%d\n",
			o1->name, o1->len, o2->name, o2->len);
#endif

	if (o2->len > 0 && o2->len <= o1->len)
	{
		int cmp = strncmp(o1->name, o2->name, o2->len);

		if (cmp != 0)
			return cmp;

		if (o1->len > strlen(o2->name))
			return 1;

		return strncmp(o1->name, o2->name, o1->len);
	}
	else if (o1->len > 0 && o1->len <= o2->len)
	{
		int cmp = strncmp(o1->name, o2->name, o1->len);

		if (cmp != 0)
			return cmp;

		if (o2->len > strlen(o1->name))
			return -1;

		return strncmp(o1->name, o2->name, o2->len);
	}

	return strcmp(o1->name, o2->name);
}

static Optentry *
find_opttable(Opttable *tab, const char *name)
{
	Optentry key;

	if (tab == NULL || tab->optlist == NULL)
		return NULL;

	if (!tab->sorted)
	{
		qsort((void *)tab->optlist, tab->optnum, sizeof (Optentry),
				qsort_cmp_optentry);
		tab->sorted = TRUE;
	}

	key.name = name;
	key.len = strlen(name);
	return (Optentry *)bsearch((void *)&key, (void *)tab->optlist, tab->optnum,
			sizeof (Optentry), bsearch_cmp_optentry);
}

static void
parseopts(Opttable *tab, const char *str)
{
	static char *buf = NULL;
	static int buflen = 0;
	char namebuf[2];
	char *name;
	int code;

	clear_opttable(tab);

	if (str == NULL)
		return;

	while (str[0] != '\0')
	{
		Params param = NONE;
		const char *close;
		Optentry key;

		key.len = 0;

		if (str[0] == '(' && (close = strchr(&str[1], ')')) != NULL)
		{
			/* long multi-character option */
			const char *strend;
			int len;
			const char *colon = strchr(&str[1], ':');
			const char *slash = strchr(&str[1], '/');

			if (colon > close)
				colon = NULL;

			if (colon != NULL)
				strend = &colon[-1];
			else
				strend = &close[-1];

			len = strend - str + 1;

			if (buflen < len)
			{
				strfree(buf);
				buflen = len;
				buf = strnew(len);

				if (buf == NULL)
					return;
			}

			if (slash != NULL && slash <= strend)
			{
				/* match a substring, but save the complete option name */
				len--;
				key.len = slash - str - 1;
				strncpy(buf, &str[1], key.len);
				strncpy(buf + key.len, &str[key.len + 2],
						len - key.len - 1);
			}
			else
				strncpy(buf, &str[1], len - 1);

			buf[len - 1] = '\0';

			if (colon == NULL || &colon[1] == close)
				code = (unsigned char)buf[0];
			else if (isdigit(colon[1]) || colon[1] == '-')
				code = atoi(&colon[1]);
			else
				code = (unsigned char)colon[1];

			str = &close[1];
			name = buf;

			if (buf[0] == '#' && buf[1] == '\0')
			{
				param = NUMERIC;
				name = "(#)";
			}
		}
		else					/* standard one-character option */
		{
			name = namebuf;
			name[0] = *str++;
			name[1] = '\0';
			code = (unsigned char)name[0];
		}

		if (str[0] == ':')
		{
			if (param == NONE)
				param = PARAM;

			str++;
		}

		key.name = name;
		key.params = param;
		key.code = code;
		insert_opttable(tab, &key);
	}
}

int
getoptx(int argc, const char **argv, const char *minus, const char *plus)
{
	Opttable *tab;
	Optentry *ent = NULL;
	boolean single = FALSE;
	int code = '?';
	Params param = NONE;
	int loc = 1;

	if (minustab == NULL)
		minustab = new_opttable();

	if (plustab == NULL)
		plustab = new_opttable();

	if (minustab == NULL || plustab == NULL)
		return EOF;

	if (minus == NULL)
		minus = "";

	if (plus == NULL)
		plus = "";

	if (optind < 1 || lastargc != argc || lastargv != argv ||
			lastminus == NULL || !streq(lastminus, minus) ||
			lastplus == NULL || !streq(lastplus, plus))
	{
		lastargc = argc;
		lastargv = argv;
		lastminus = minus;
		lastplus = plus;
		optind = 0;
		endflag = FALSE;
		singlechars = NULL;
		lasttab = NULL;
		parseopts(minustab, minus);
		parseopts(plustab, plus);
	}

	if (endflag)
		return EOF;

	if (singlechars != NULL && singlechars[0] != '\0')
	{
		char name[2];

		name[0] = *singlechars++;
		name[1] = '\0';
		tab = lasttab;
		ent = find_opttable(tab, name);
		single = TRUE;
	}
	else
	{
		optind++;

		if (optind >= argc)
		{
			endflag = TRUE;
			return EOF;
		}

		/* --opt is same as +opt */
		if (argv[optind][0] == '+' ||
				(argv[optind][0] == '-' && argv[optind][1] == '-' &&
						argv[optind][2]))
		{
			if (argv[optind][0] != '+')
				loc++;

			tab = plustab;
			optsign = '+';
		}
		else if (argv[optind][0] == '-')
		{
			if (argv[optind][1] == '\0')
			{
				endflag = TRUE;
				return EOF;
			}
			else if (argv[optind][1] == '-' && argv[optind][2] == '\0')
			{
				optind++;
				endflag = TRUE;
				return EOF;
			}

			tab = minustab;
			optsign = '-';
		}
		else
		{
			endflag = TRUE;
			return EOF;
		}

		ent = find_opttable(tab, &argv[optind][loc]);	/* multi-char name */

		if (ent == NULL)		/* did not find it */
		{
			char name[2];
			name[0] = argv[optind][loc];
			name[1] = '\0';
			ent = find_opttable(tab, name);	/* single-char name */

			if (ent == NULL && (isdigit(name[0]) || name[0] == '-'))
				ent = find_opttable(tab, "(#)");	/* numeric */
			else				/* do not care if found or not found */
			{
				single = TRUE;
				singlechars = &argv[optind][loc + 1];
				lasttab = tab;
			}
		}
	}

	if (ent != NULL)
	{
		code = ent->code;
		param = ent->params;
		optname = ent->name;
	}
	else
		optname = NULL;

	switch (param)
	{
	case PARAM:
		if (single && singlechars[0] != '\0')
		{
			optarg = (char *)singlechars;
			singlechars = NULL;
		}
		else
		{
			optind++;

			if (optind >= argc)
			{
				optarg = NULL;
				return '?';
			}

			optarg = (char *)argv[optind];
		}

		break;

	case NUMERIC:
		optarg = (char *)&argv[optind][loc];

		if (streq(optarg, "(#)"))
			optarg = NULL;

		break;

	default:
		optarg = NULL;
		break;
	}

	return code;
}
