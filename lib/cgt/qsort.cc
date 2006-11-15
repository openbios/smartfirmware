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
#include <memory.h>
#include <stdio.h>

#include <expand.h>

typedef int (*__fcmp)(void const *, void const *);

extern "C" void qsort(void *p, size_t items, size_t size,
	int (*cmp)(void const *, void const *));

static char *temp = NULL;
static int templen = 0;
static __fcmp compfunc;
static char *ptr;
static int elsize;

static void
swap(int i, int j)
{
//    fprintf(stderr, "swap(%d, %d)\n", i, j);
    memcpy(temp, &ptr[i * elsize], elsize);
    memcpy(&ptr[i * elsize], &ptr[j * elsize], elsize);
    memcpy(&ptr[j * elsize], temp, elsize);
}

static int
compare(int i, int j)
{
//    fprintf(stderr, "compare(%d, %d) = ", i, j);
    int ret = (*compfunc)(&ptr[i * elsize], &ptr[j * elsize]);
//    fprintf(stderr, "%d\n", ret);
    return ret;
}

struct range
{
    size_t l;
    size_t h;
};

static struct range *stack = NULL;
static int stacklen = 0;
static int stacksize = 0;

declare_expand(struct range)
implement_expand(struct range)

static void
push(size_t l, size_t h)
{
    expand(stack, stacklen, stacksize + 1);
    stack[stacksize].l = l;
    stack[stacksize].h = h;
    stacksize++;
}

static boolean
pop(size_t &l, size_t &h)
{
    if (stacksize == 0)
	return FALSE;

    stacksize--;
    l = stack[stacksize].l;
    h = stack[stacksize].h;
    return TRUE;
}

static void
qsort(size_t l, size_t h)
{
//    fprintf(stderr, "qsort(%d, %d)\n", l, h);

    if (l >= h)
    	return;

    if (l + 1 == h)
    {
	if (compare(l, h) > 0)
	    swap(l, h);

	return;
    }

    if (l + 2 == h)
    {
	if (compare(l, l + 1) > 0)
	    if (compare(l + 1, h) > 0)
	    	swap(l, h);
	    else
	    {
	    	swap(l, l + 1);

		if (compare(l + 1, h) > 0)
		    swap(l + 1, h);
	    }
	else if (compare(l + 1, h) > 0)
	{
	    swap(l + 1, h);

	    if (compare(l, l + 1) > 0)
	    	swap(l, l + 1);
	}

    	return;
    }

    size_t m = (l + h) >> 1;
//    fprintf(stderr, "m = %d\n", m);
    swap(l, m);

    size_t i = l + 1;
    size_t j = h;

    while (i < j)
    {
//	fprintf(stderr, "i = %d, j = %d\n", i, j);

    	while (i < j)
	{
	    if (compare(l, i) < 0)
	    	break;

	    i++;
	}

	while (i < j)
	{
	    if (compare(l, j) > 0)
	    	break;

	    j--;
	}

	if (i < j)
	    swap(i++, j--);
    }

    while (i > l)
    {
	if (compare(l, i) >= 0)
	    break;

    	i--;
    }

    if (i > l)
    {
    	swap(l, i);
	push(l, i - 1);
    }

    push(i + 1, h);
}

void
qsort(void *p, size_t items, size_t size,
	int (*cmp)(void const *, void const *))
{
//    fprintf(stderr, "qsort(0x%08lX, %d, %d, 0x%08lX)\n", p, items, size, cmp);

    if (templen < size)
    {
    	if (temp != NULL)
	    delete[/*templen*/] temp;

	temp = new char[size];
	templen = size;
    }

    compfunc = cmp;
    ptr = (char *)p;
    elsize = size;

    push(0, items - 1);

    size_t l;
    size_t h;

    while (pop(l, h))
    	qsort(l, h);
}
