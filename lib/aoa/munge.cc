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
#include <stdlibx.h>
#include <stdio.h>
#include <stdiox.h>
#include <string.h>
#include <stringx.h>
#include <hashtable.h>
#include <dynarray.h>
#include <bitset.h>

class Elem
{
public:
    void *_p;
    int _i;
    Elem(void *p) { _p = p; }
    Elem(Elem const &e) { _p = e._p; _i = e._i; }
    ~Elem() { }
    operator void *() { return _p; }
    boolean operator==(void *p) { return _p == p; }
    boolean operator==(Elem const &e) { return _p == e._p; }
};

unsigned long
ptrhash(void *p)
{
    return (unsigned long)p >> 3;
}

declare_hashtable(ElemTable, ElemIter, Elem, void *)
implement_hashtable(ElemTable, ElemIter, Elem, void *, ptrhash)

declare_array(IntVec, int, 4096)
implement_array(IntVec, int, 4096)

void *
atop(char const *s)
{
    if (s[1] == 'x' || s[1] == 'X')
	s += 2;

    char *es;
    unsigned long l = strtol(s, &es, 16);
    return (void *)l;
}

main()
{
    char *str;
    ElemTable idtab;
    Bitset freeids;
    IntVec sizes;
    int ids = 0;
    long memallocated = 0;
    long memfreed = 0;
    long meminuse = 0;
    long maxmeminuse = 0;

    while ((str = xgets(stdin)) != NULL)
    {
    	int argc;
	const char **argv = strsplit(str, " \t", TRUE, &argc);

	if (argc == 0)
	    continue;

	switch (argv[0][0])
	{
	case 'm':
	    {
		if (argc != 3)
		{
		    fprintf(stderr, "expected m size 0xaddr\n");
		    break;
		}

		long sz = atol(argv[1]);
		void *addr = atop(argv[2]);

		if (idtab.find(addr))
		    fprintf(stderr, "malloc returned an allocated pointer, p=%p\n", addr);

		idtab.insert(addr);
		int id = freeids.min();
		freeids -= id;

		if (id == -1)
		    id = ids++;

		idtab()._i = id;
		sizes[id] = sz;
		memallocated += sz;
		meminuse += sz;

		if (meminuse > maxmeminuse)
		    maxmeminuse = meminuse;

		printf("m %d %ld\n", id, sz);
	    }

	    break;
	case 'f':
	    {
		if (argc != 2)
		{
		    fprintf(stderr, "expected f 0xaddr\n");
		    break;
		}

		void *addr = atop(argv[1]);

		if (!idtab.find(addr))
		{
		    fprintf(stderr, "free passed an unallocated pointer, p=%p\n", addr);
		    break;
		}

		int id = idtab()._i;
		idtab.remove(addr);
		freeids += id;
		int sz = sizes[id];
		memfreed += sz;
		meminuse -= sz;
		printf("f %d\n", id);
	    }

	    break;
	case 'r':
	    {
		if (argc != 4)
		{
		    fprintf(stderr, "expected r 0xoaddr nsz 0xnaddr\n");
		    break;
		}

		void *oaddr = atop(argv[1]);
		long nsz = atol(argv[2]);
		void *naddr = atop(argv[3]);

		if (oaddr != NULL)
		{
		    if (!idtab.find(oaddr))
		    {
			fprintf(stderr, "realloc passed an unallocated pointer, p=%p\n", oaddr);
			break;
		    }

		    int id = idtab()._i;
		    idtab.remove(oaddr);
		    int sz = sizes[id];
		    memfreed += sz;
		    meminuse -= sz;

		    if (nsz != 0)
		    {
			if (idtab.find(naddr))
			    fprintf(stderr, "realloc returned an allocated pointer, p=%p\n", naddr);

			idtab.insert(naddr);
			idtab()._i = id;
			sizes[id] = nsz;
			memallocated += nsz;
			meminuse += nsz;

			if (meminuse > maxmeminuse)
			    maxmeminuse = meminuse;

			printf("r %d %ld\n", id, nsz);
		    }
		    else
		    {
		    	freeids += id;
			printf("f %d\n", id);
		    }
		}
		else
		{
		    if (idtab.find(naddr))
			fprintf(stderr, "realloc returned an allocated pointer, p=%p\n", naddr);

		    idtab.insert(naddr);

		    int id = freeids.min();
		    freeids -= id;

		    if (id == -1)
			id = ids++;

		    idtab()._i = id;
		    sizes[id] = nsz;
		    memallocated += nsz;
		    meminuse += nsz;

		    if (meminuse > maxmeminuse)
			maxmeminuse = meminuse;

		    printf("m %d %ld\n", id, nsz);
		}
	    }

	    break;
	case 'a':
	    {
		if (argc != 4)
		{
		    fprintf(stderr, "expected a align size 0xaddr\n");
		    break;
		}

		long align = atol(argv[1]);
		long sz = atol(argv[2]);
		void *addr = atop(argv[3]);

		if (idtab.find(addr))
		    fprintf(stderr, "memalign returned an allocated pointer, p=%p\n", addr);

		idtab.insert(addr);
		int id = freeids.min();
		freeids -= id;

		if (id == -1)
		    id = ids++;

		idtab()._i = id;
		sizes[id] = sz;
		memallocated += sz;
		meminuse += sz;

		if (meminuse > maxmeminuse)
		    maxmeminuse = meminuse;

		printf("a %d %ld %ld\n", id, align, sz);
	    }

	    break;
	default:
	    fprintf(stderr, "unrecognized option \"%s\"\n", argv[0]);
	    break;
	}
    }

    printf("#\n# allocated=%ld, freed=%ld, maxinuse=%ld\n", memallocated,
	    memfreed, maxmeminuse);

    return 0;
}
