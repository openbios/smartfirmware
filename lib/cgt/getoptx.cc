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

// Copyright (c) 1992,1993,1998 by TJ Merritt and Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/lib/cgt/getoptx.cc,v 1.20 2002/06/30 06:35:35 tjm Exp $

#include <stdlib.h>
#include <stdlibx.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stringx.h>
#include <hashtable.h>
#include "getoptx.h"

#if !defined(_DJGPP_SOURCE) && !defined(unix) && !defined(OSX_SOURCE)
int optind = 0;
char *optarg = NULL;
#endif
const char *optname = NULL;
int opttype;

enum Params
{
    NONE,
    PARAM,
    NUMERIC
};

class optentry
{
public:
    char *name;
    Params params;
    int code;

    optentry() { name = NULL; params = NONE; code = 0; }
    optentry(const char *s) { name = strdup(s); params = NONE; code = 0; }
    ~optentry() { strfree(name); }
    boolean operator==(const char *s) { return streq(s, name); }
    boolean operator==(const optentry &e) { return streq(e.name, name); }
    operator const char *() { return name; }
};

declare_hashtable(opttable, optiter, optentry, const char *);
implement_hashtable(opttable, optiter, optentry, const char *, strhash);
implement_hashtableiter(opttable, optiter, optentry, const char *, strhash);

static opttable minustab;
static opttable plustab;

static int lastargc = 0;
static const char **lastargv = NULL;
static const char *lastminus = NULL;
static const char *lastplus = NULL;

static opttable *lasttab = NULL;
static const char *singlechars = NULL;

static boolean endflag = TRUE;

static void
parseopts(opttable &tab, const char *str)
{
    tab.clear();

    if (str == NULL)
	return;

    static char *buf = NULL;
    static int buflen = 0;
    char namebuf[2];
    char *name;
    int code;

    while (str[0] != '\0')
    {
	Params param = NONE;
	const char *close;

	if (str[0] == '(' && (close = strchr(&str[1], ')')) != NULL)
	{
	    // long multi-character option
	    const char *colon = strchr(&str[1], ':');

	    if (colon > close)
		colon = NULL;

	    const char *strend;

	    if (colon != NULL)
		strend = &colon[-1];
	    else
		strend = &close[-1];

	    int len = strend - str + 1;

	    if (buflen < len)
	    {
		if (buf)
		    delete[/*buflen*/] buf;

		buf = new char[len];
		buflen = len;
	    }

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
	else	// standard one-character option
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

	optentry &oe = tab[name];
	oe.params = param;
	oe.code = code;
    }
}

int
getoptx(int argc, const char **argv, const char *minus, const char *plus)
{
    int loc = 1;

    if (optind < 1 || lastargc != argc || lastargv != argv ||
	    lastminus == NULL || !streq(lastminus, minus) || lastplus == NULL ||
	    !streq(lastplus, plus))
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

    opttable *tab;
    optentry *ent = NULL;
    boolean  single = FALSE;

    if (singlechars != NULL && singlechars[0] != '\0')
    {
	char name[2];
	name[0] = *singlechars++;
	name[1] = '\0';
	tab = lasttab;

	if (tab->find((char*)name))
	    ent = &(*tab)();

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

	if (argv[optind][0] == '+' ||
		(argv[optind][0] == '-' && argv[optind][1] == '-' &&
			argv[optind][2]))
	{
	    if (argv[optind][0] != '+')
		loc++;

	    tab = &plustab;
	    opttype = '+';
	}
	else if (argv[optind][0] == '-')
	{
	    if (argv[optind][1] == '-' && argv[optind][2] == '\0')
	    {
		optind++;
		endflag = TRUE;
		return EOF;
	    }

	    tab = &minustab;
	    opttype = '-';
	}
	else
	{
	    endflag = TRUE;
	    return EOF;
	}

	if (tab->find(&argv[optind][loc]))	// multi-char name
	    ent = &(*tab)();
	else
	{
	    char name[2];
	    name[0] = argv[optind][loc];
	    name[1] = '\0';

	    if (tab->find((char*)name))		// single-char name
	    {
		ent = &(*tab)();
		single = TRUE;
		singlechars = &argv[optind][loc + 1];
		lasttab = tab;
	    }
	    else if (isdigit(name[0]) || name[0] == '-')    // numeric
	    {
		if (tab->find((char*)"(#)"))
		    ent = &(*tab)();
	    }
	}
    }

    int code = '?';
    Params param = NONE;

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
