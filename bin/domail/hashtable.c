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


#define DEBUG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <db.h>

#include "spamlib.h"

struct hashtableentry
{
    struct hashtableentry *next;
    unsigned int hash;
    char data[1];
};

typedef struct hashtableentry Hashtableentry;

struct hashtable
{
    Hashtableentry **table;
    int len;
    int size;
    int entsz;
    Hashtablecmpfunc cmp;
    Hashtabledestfunc dest;
};

Hashtable *
hashtablecreate(int entrysize, Hashtablecmpfunc cmp, Hashtabledestfunc dest)
{
    Hashtable *ht = (Hashtable *)malloc(sizeof (Hashtable));

    if (ht == NULL)
	return NULL;

    ht->table = NULL;
    ht->len = 0;
    ht->size = 0;
    ht->entsz = entrysize;
    ht->cmp = cmp;
    ht->dest = dest;
//    dprintf("Hash Table: table %p, len %d, size %d, entsz %d, cmp %p, dest %p\n",
//	    ht->table, ht->len, ht->size, ht->entsz, ht->cmp, ht->dest);
    return ht;
}

static void
hashtableexpand(Hashtable *ht)
{
    int newlen = (ht->len + 7) * 3;
    Hashtableentry **newtable =
	    (Hashtableentry **)malloc(newlen * sizeof (Hashtableentry *));
    int i, j;
    Hashtableentry *list;
    Hashtableentry *entry;

    if (newtable == NULL)
	return;

    memset(newtable, '\0', newlen * sizeof (Hashtableentry *));

    for (i = 0; i < ht->len; i++)
	for (list = ht->table[i]; list != NULL; )
	{
	    entry = list;
	    list = list->next;
	    j = entry->hash % newlen;
	    entry->next = newtable[j];
	    newtable[j] = entry;
	}

    if (ht->table)
	free(ht->table);

    ht->table = newtable;
    ht->len = newlen;
//    fprintf(stderr, "new table size %d\n", newlen);
}

void *
hashtablelookup(Hashtable *ht, void *key, unsigned int hash)
{
    Hashtableentry *prev;
    Hashtableentry *list;
    int hi;
    int i;

    if (ht == NULL || ht->table == NULL)
	return NULL;

    hi = hash % ht->len;
    list = ht->table[hi];
    prev = NULL;

    for (i = 0; list != NULL; prev = list, list = list->next, i++)
	if (list->hash == hash && (*ht->cmp)(list->data, key) == 0)
	{
	    if (prev)
	    {
		/* move to top of the list */
		prev->next = list->next;
		list->next = ht->table[hi];
		ht->table[hi] = list;
	    }

	    return list->data;
	}

    return NULL;
}

void *
hashtableadd(Hashtable *ht, void *key, unsigned int hash)
{
    Hashtableentry *entry;

    if (ht == NULL)
	return NULL;

    if (ht->table == NULL || ht->size > (ht->len * 2))
	hashtableexpand(ht);

    if (ht->table == NULL)
	return NULL;

    entry = (Hashtableentry *)malloc(sizeof *entry + ht->entsz - 1);

    if (entry == NULL)
	return NULL;

    entry->hash = hash;
    memcpy(entry->data, key, ht->entsz);

    entry->next = ht->table[hash % ht->len];
    ht->table[hash % ht->len] = entry;
    ht->size++;
    return entry->data;
}

void
hashtableclear(Hashtable *ht)
{
    int i;
    Hashtableentry *entry;
    Hashtableentry *next;

    if (ht == NULL || ht->table == NULL)
	return;

    for (i = 0; i < ht->len; i++)
    {
	for (entry = ht->table[i]; entry; entry = next)
	{
	    next = entry->next;
	    (*ht->dest)(entry->data);
	    free(entry);
	}

	ht->table[i] = NULL;
    }

    ht->size = 0;
}

void
hashtablefree(Hashtable *ht)
{
    hashtableclear(ht);

    if (ht != NULL)
	free(ht);
}
