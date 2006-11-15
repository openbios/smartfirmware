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


#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gcext.h"
#ifdef __MWERKS__
#include <console.h>
#define macintosh
#endif

#ifdef macintosh
extern void *sbrk(size_t len);
#endif

struct Object
{
    char *name;
    int value;
    struct Object *left, *right;
};

void
destroy_obj(void *o)
{
    struct Object *obj = (struct Object*)o;

    printf("destroy_obj(%s) = %p\n", obj->name, obj);

    if (obj && obj->name)
	free(obj->name);
}

static ptrdiff_t obj_offsets[] =
	{ offsetof(struct Object, left), offsetof(struct Object, right) };

struct gc_type_info obj_info =
{
    "Object",
    NOT_ATOMIC,
    ZERO_FILL,
    destroy_obj,
    sizeof obj_offsets / sizeof obj_offsets[0],
    obj_offsets,
    NULL	/* no userdef */
};

struct Object *
new_obj(char *name, int val)
{
    struct Object *obj = gc_alloc(&obj_info, sizeof *obj);

    if (!obj)
    {
	fprintf(stderr, "new_obj(): cannot allocate Object\n");
	exit(10);
    }

    obj->name = malloc(strlen(name) + 1);
    strcpy(obj->name, name);
    obj->value = val;
    printf("new_obj(%s) = %p\n", obj->name, obj);
    return obj;
}

int
main(int argc, char *argv[])
{
    struct Object *o, *oo;
    int i, j;
    int max = 100;
    int max2 = 10;
    extern gc_dumpheap(void);
    extern void *sbrk(int);

#ifdef __MWERKS__
	argc = ccommand(&argv);
#endif

    printf("starting sbrk(0) is %p\n", sbrk(0));

    if (argc > 1)
	max = atoi(argv[1]);

    if (argc > 2)
	max2 = atoi(argv[2]);

    for (i = 0; i < max; i++)
	for ((oo = NULL), j = 0; j < max2; (o->right = oo), (oo = o), j++)
	    o = new_obj("nuts", i);

    printf("sbrk(0) is %p\n", sbrk(0));
    gc_dumpheap();
    return 0;
}
