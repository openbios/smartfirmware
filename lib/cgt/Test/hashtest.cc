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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdlibx.h>
#include <stringx.h>

struct Elem
{
    char *name;
    Elem(Elem &e) { name = strdup(e.name); }
    Elem(char *n) { name = strdup(n); }
    ~Elem() { strfree(name); }
    void operator=(Elem &e) { strfree(name); name = strdup(e.name); }
    boolean operator==(Elem &e) { return boolean(strcmp(name, e.name) == 0); }
    boolean operator==(char *k) { return boolean(strcmp(name, k) == 0); }
    operator char *() { return name; }
};

//#define TABLE Table
//#define ITER Iter
//#define ELEM Elem
//#define KEY char *
//#define HASHFUNC strhash

//#include <../hashtable.g>

#include <hashtable.h>

declare_hashtable(Table, Iter, Elem, char *);
implement_hashtable(Table, Iter, Elem, char *, strhash);
implement_hashtableiter(Table, Iter, Elem, char *, strhash);

char *dictionary[] =
{
    "word1",
    "word2",
    "word3",
    "word4",
    "word5",
    "word6",
    "word7",
    "word8",
    "word9",
    "word10",
    "word11",
    NULL
};

main()
{
    printf("Testing...\n");

    Table t;
    printf("size = %ld\n", t.size());

    int i;

    for (i = 0; dictionary[i] != NULL; i++)
	printf("insert(%s) = %d\n", dictionary[i], t.insert(dictionary[i]));

    printf("size = %ld\n", t.size());

    for (i = 0; dictionary[i] != NULL; i++)
    {
	boolean ret = t.find(dictionary[i]);
	printf("find(%s) = %d, name = %s\n", dictionary[i], ret, t().name);
    }

    printf("size = %ld\n", t.size());

    for (i = 0; dictionary[i] != NULL; i++)
	printf("remove(%s) = %d\n", dictionary[i], t.remove(dictionary[i]));

    printf("size = %ld\n", t.size());
    return 0;
}

static unsigned long usage = 0;
static char *startaddr = NULL;

void *
operator new(size_t sz)
{
    char *p = (char*)malloc(sz);
    if (startaddr == NULL)
	startaddr = p;
    printf("new char[%d] = 0x%X\n", sz, p - startaddr);

    if (p != NULL)
    {
	usage += sz;
	printf("Usage = %ld\n", usage);
    }

    return p;
}

void
operator delete(void *p)
{
    free(p);
    printf("delete 0x%X\n", (char*)p - startaddr);
}
