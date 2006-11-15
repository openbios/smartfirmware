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

/* Copyright (c) 1993 by Parag Patel.  All Rights Reserved. */
/* $Header: /u/cgt/cvs/lib/clib/strpool.c,v 1.1 1998/01/26 00:47:01 parag Exp $ */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "stdlibx.h"
#include "stringx.h"
#include "hash.h"

typedef struct poolelem
{
	char const *str;
	int ref;
} poolelem;

static long
hash_poolelem(poolelem *self)
{
	return strhash(self->str);
}

static poolelem *
init_poolelem(poolelem *self)
{
	self->str = NULL;
	self->ref = 0;
	return self;
}

static poolelem *
copy_poolelem(poolelem *self, poolelem *e)
{
	strfree(self->str);
	self->str = strdup(e->str);
	self->ref = e->ref;
	return self;
}

static poolelem *
destroy_poolelem(poolelem *self)
{
	strfree(self->str);
	return self;
}

static boolean
compare_poolelem(poolelem *self, poolelem *e)
{
	return streq(self->str, e->str);
}

static struct hashinfo poolinfo =
{
	(hash_func*)hash_poolelem,
	(init_func*)init_poolelem,
	(copy_func*)copy_poolelem,
	(destroy_func*)destroy_poolelem,
	(compare_func*)compare_poolelem,
	sizeof (poolelem)
};

static struct hashtable *strpool = NULL;

char const *
strget(char const *s)
{
	poolelem e, *ep;

	if (strpool == NULL)
		strpool = new_hashtable(&poolinfo);

	e.str = s;

	if (!find_hashtable(strpool, (hashelt *)&e))
		insert_hashtable(strpool, (hashelt *)&e);

	ep = (poolelem *)cur_hashtable(strpool);
	ep->ref++;
	return ep->str;
}

void
strdrop(char const *s)
{
	poolelem e, *ep;

	if (strpool == NULL)
		return;

	e.str = s;

	if (!find_hashtable(strpool, &e))
		return;

	ep = (poolelem *)cur_hashtable(strpool);

	if (--ep->ref <= 0)
		remove_hashtable(strpool, ep);
}

char const *
strgeti(char const *s)
{
	char *str = strnew(strlen(s));
	char *p = str;
	const char *ret;

	for (; *s != '\0'; s++)
		*p++ = (isupper(*s)) ? tolower(*s) : *s;

	*p = '\0';
	ret = strget(str);
	strfree(str);
	return ret;
}

void
strdropi(char const *s)
{
	char *str = strnew(strlen(s));
	char *p = str;

	for (; *s != '\0'; s++)
		*p++ = (isupper(*s)) ? tolower(*s) : *s;

	*p = '\0';
	strdrop(str);
	strfree(str);
}

void
clear_string_pool(void)
{
	if (strpool == NULL)
		return;

	clear_hashtable(strpool);
}
