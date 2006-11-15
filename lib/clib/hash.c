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
#include "stdlibx.h"
#include "hash.h"

typedef struct hashinfo hashinfo;

typedef struct hashtable
{
	hashinfo *info;
	hashelt **chains;
	long numchains;
	long size;
	struct hashiter *iters;
	hashelt *cur;
} hashtable;

#define link(e) ((hashelt**)(e))[-1]
#define linki(e) ((hashelt**)(e))[-1]

typedef struct hashiter
{
	hashtable *t;
	struct hashiter *n;
	struct hashiter *p;
	int hv;
	hashelt *cur;
} hashiter;


hashelt *
lookup_hashtable(hashtable *self, hashelt *key)
{
	unsigned long hv = self->info->hash(key) % self->numchains;
	hashelt *p;

	for (p = self->chains[hv]; p != NULL; p = link(p))
		if (self->info->compare(p, key))
			return p;

	return NULL;
}

long
size_hashtable(hashtable *self)
{
	return self->size;
}

hashelt *
get_hashtable(hashtable *self, hashelt *key)
{
	return self->cur = lookup_hashtable(self, key);
}

boolean
find_hashtable(hashtable *self, hashelt *key)
{
	self->cur = lookup_hashtable(self, key);
	return self->cur != NULL;
}

hashelt *
cur_hashtable(hashtable *self)
{
	return self->cur;
}

static void
bump_hashtable(hashtable *self, long chains)
{
	long i;
	hashelt **nchains;
	hashelt *list = NULL;
	hashelt *last = NULL;

	if (self->iters != NULL)
		return;

	if (chains == 0)
	{
		chains = hashtable_bumpfunc(self->size);

		if (chains == 0)
			chains = 1;
	}

	if (self->numchains >= chains)
		return;

	nchains = (hashelt **)malloc(self->info->size * chains);

	for (i = 0; i < chains; i++)
		nchains[i] = NULL;

	for (i = 0; i < self->numchains; i++)
		if (self->chains[i] != NULL)
		{
			if (last != NULL)
				link(last) = self->chains[i];
			else
				list = self->chains[i];

			for (last = self->chains[i]; link(last) != NULL;
					last = link(last))
				;
		}

	for (last = list; last != NULL;)
	{
		unsigned long hv;

		list = last;
		last = link(last);
		hv = self->info->hash(list) % chains;
		link(list) = nchains[hv];
		nchains[hv] = list;
	}

	if (self->chains != NULL)
		free(self->chains);

	self->chains = nchains;
	self->numchains = chains;
}

hashtable *
init_hashtable(hashtable *self, hashinfo *info)
{
	self->chains = NULL;
	self->numchains = 0;
	self->size = 0;
	self->cur = NULL;
	self->iters = NULL;
	self->info = info;
	bump_hashtable(self, 0);
	return self;
}

hashtable *
new_hashtable(hashinfo *info)
{
	return init_hashtable((hashtable *)malloc(sizeof (hashtable)), info);
}

hashtable *
dup_hashtable(hashtable *self)
{
	hashtable *dup = new_hashtable(self->info);
	return copy_hashtable(dup, self);
}

hashtable *
destroy_hashtable(hashtable *self)
{
	if (self == NULL)
		return NULL;

	clear_hashtable(self);

	if (self->chains != NULL)
		free(self->chains);

	return self;
}

void
delete_hashtable(hashtable *self)
{
	if (self == NULL)
		return;

	free(destroy_hashtable(self));
}

boolean
insert_hashtable(hashtable *self, hashelt *key)
{
	unsigned long hv = self->info->hash(key) % self->numchains;
	hashelt *p;

	for (p = self->chains[hv]; p != NULL; p = link(p))
		if (self->info->compare(p, key))
		{
			self->cur = NULL;
			return FALSE;
		}

	p = (hashelt *)malloc(self->info->size + sizeof (hashelt *));
	p = (hashelt *)((hashelt **)p + 1);
	self->info->init(p);
	self->info->copy(p, key);
	link(p) = self->chains[hv];
	self->chains[hv] = p;
	self->cur = p;
	self->size++;

	if ((self->size << 1) > (self->numchains << 1) + self->numchains)
		bump_hashtable(self, 0);

	return TRUE;
}

boolean
remove_hashtable(hashtable *self, hashelt *key)
{
	unsigned long hv = self->info->hash(key) % self->numchains;
	hashelt *p, *q;

	for (p = self->chains[hv], q = NULL; p != NULL; p = link(p))
		if (self->info->compare(p, key))
		{
			hashiter *iter;

			if (self->cur == p)
				self->cur = NULL;

			for (iter = self->iters; iter != NULL; iter = iter->n)
				if (iter->cur == p)
					advance_hashiter(iter);

			if (q != NULL)
				link(q) = link(p);
			else
				self->chains[hv] = link(p);

			self->info->destroy(p);
			free((hashelt **)p - 1);
			self->size--;
			return TRUE;
		}
		else
			q = p;

	return FALSE;
}

void
clear_hashtable(hashtable *self)
{
	hashiter *iter;
	long i;

	for (iter = self->iters; iter != NULL; iter = iter->n)
	{
		iter->cur = NULL;
		iter->hv = -1;
	}

	for (i = 0; i < self->numchains; i++)
	{
		hashelt *p, *q;

		for (p = self->chains[i], q = NULL; p != NULL; p = q)
		{
			q = link(p);
			self->info->destroy(p);
			free((hashelt **)p - 1);
		}

		self->chains[i] = NULL;
	}

	self->size = 0;
	self->cur = NULL;
}

hashtable *
copy_hashtable(hashtable *self, hashtable *t)
{
	clear_hashtable(self);
	join_hashtable(self, t);
	return self;
}

boolean
compare_hashtable(hashtable *self, hashtable *t)
{
	long i;
	hashelt *p;

	if (self->size != t->size)
		return FALSE;

	for (i = 0; i < self->numchains; i++)
		for (p = self->chains[i]; p != NULL; p = link(p))
		{
			hashelt *e = lookup_hashtable(self, p);

			if (e == NULL || !self->info->compare(e, p))
				return FALSE;
		}

	return TRUE;
}

void
join_hashtable(hashtable *self, hashtable *t)
{
	long i;
	hashelt *p;

	for (i = 0; i < t->numchains; i++)
		for (p = t->chains[i]; p != NULL; p = link(p))
			if (!insert_hashtable(self, p))
				self->info->copy(self->cur, p);
}

void
separate_hashtable(hashtable *self, hashtable *t)
{
	long i;
	hashelt *p;

	for (i = 0; i < t->numchains; i++)
		for (p = t->chains[i]; p != NULL; p = link(p))
			remove_hashtable(self, p);
}

void
intersect_hashtable(hashtable *self, hashtable *t)
{
	long i;
	hashelt *p;

	for (i = 0; i < self->numchains; i++)
		for (p = self->chains[i]; p != NULL; p = link(p))
			if (lookup_hashtable(t, p) == NULL)
				remove_hashtable(self, p);
}

hashelt *
cur_hashiter(hashiter *self)
{
	return self->cur;
}

boolean
finished_hashiter(hashiter *self)
{
	return self->cur == NULL;
}

void
attach_hashiter(hashiter *self, hashtable *t)
{
	self->t = t;
	self->n = self->t->iters;
	self->p = NULL;

	if (self->n != NULL)
		self->n->p = self;

	self->t->iters = self;

	for (self->hv = 0; self->hv < self->t->numchains; self->hv++)
		if (self->t->chains[self->hv] != NULL)
			break;

	if (self->hv < self->t->numchains)
		self->cur = self->t->chains[self->hv];
	else
		self->cur = NULL;
}

void
detach_hashiter(hashiter *self)
{
	if (self->t != NULL)
	{
		if (self->p == NULL)
		{
			self->t->iters = self->n;

			if (self->n)
				self->n->p = NULL;
		}
		else
		{
			self->p->n = self->n;

			if (self->n)
				self->n->p = NULL;
		}
	}
}

hashiter *
init_hashiter(hashiter *self)
{
	self->t = NULL;
	self->n = NULL;
	self->p = NULL;
	self->hv = 0;
	self->cur = NULL;
	return self;
}

hashiter *
new_hashiter(void)
{
	return init_hashiter((hashiter *)malloc(sizeof (hashiter)));
}

hashiter *
destroy_hashiter(hashiter *self)
{
	if (self == NULL)
		return NULL;

	detach_hashiter(self);
	return self;
}

void
delete_hashiter(hashiter *self)
{
	if (self == NULL)
		return;

	free(destroy_hashiter(self));
}

hashiter *
copy_hashiter(hashiter *self, hashiter *i)
{
	detach_hashiter(self);
	attach_hashiter(self, i->t);
	self->hv = i->hv;
	self->cur = i->cur;
	return self;
}

void
table_hashiter(hashiter *self, hashtable *t)
{
	detach_hashiter(self);
	attach_hashiter(self, t);
}

boolean
compare_hashiter(hashiter *self, hashiter *i)
{
	if (self->t == i->t && self->hv == i->hv && self->cur == i->cur)
		return TRUE;

	return FALSE;
}

hashelt *
advance_hashiter(hashiter *self)
{
	hashelt *cur = self->cur;

	if (self->cur != NULL && linki(self->cur) != NULL)
		self->cur = linki(self->cur);
	else
	{
		if (self->cur == NULL)
			self->hv = -1;

		for (self->hv++; self->hv < self->t->numchains; self->hv++)
			if (self->t->chains[self->hv] != NULL)
				break;

		if (self->hv < self->t->numchains)
			self->cur = self->t->chains[self->hv];
		else
			self->cur = NULL;
	}

	return cur;
}

boolean
remove_hashiter(hashiter *self)
{
	hashelt *prev = self->t->chains[self->hv];
	hashelt *cur = self->cur;
	hashiter *iter;

	if (cur == NULL)
		return FALSE;

	if (prev == cur)
		self->t->chains[self->hv] = linki(self->cur);
	else
	{
		while (prev != NULL && linki(prev) != NULL)
			if (linki(prev) == cur)
				break;

		if (prev == NULL)
			return FALSE;

		linki(prev) = linki(cur);
	}

	for (iter = self->t->iters; iter != NULL; iter = iter->n)
		if (iter->cur == cur)
			advance_hashiter(iter);

	self->t->info->destroy(cur);
	free(cur);
	return TRUE;
}
