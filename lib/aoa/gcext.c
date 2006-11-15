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
#include <string.h>
#include "aoa.h"
#include "gcext.h"
#include "gcbase.h"

extern struct gc_interface *gc_interface_list[];

static struct gc_interface *g_curr_interface = NULL;

struct gc_interface *
gc_get_curr_interface(void)
{
    return g_curr_interface;
}

void
gc_set_curr_interface(struct gc_interface *g)
{
    g_curr_interface = g;
}

struct gc_interface *
gc_find_interface(const char *str)
{
    struct gc_interface **g;

    for (g = gc_interface_list; *g; g++)
	if ((*g)->name && strcmp(str, (*g)->name) == 0)
	    return *g;

    return NULL;
}

void *
gc_alloc(struct gc_type_info *arg, size_t size)
{
    void *obj;

    if (g_curr_interface == NULL)
	g_curr_interface = gc_interface_list[0];

    obj = gc_base_alloc(g_curr_interface, size, arg);

    if (arg->fill == ZERO_FILL)
	memset(obj, 0, size);
    else if (arg->fill == ILLEGAL_FILL)
	memset(obj, 0xFC, size);

    return obj;
}

void
gc_free(void *obj)
{
    if (g_curr_interface == NULL)
	g_curr_interface = gc_interface_list[0];

    gc_base_free(g_curr_interface, obj);
}

struct gc_type_info *
gc_object_type_info(void *obj)
{
    struct gc_pageinfo *pi = (struct gc_pageinfo *)GETPAGEINFO(obj);

    if (!gc_is_gc_page(pi))
	return NULL;

    return pi->type;
}

void
gc_collect(void)
{
    if (g_curr_interface == NULL)
	g_curr_interface = gc_interface_list[0];

    if (g_curr_interface->collect)
    {
    #ifdef MTSAFE
	ACQUIRE_LOCK(g_curr_interface->lock);
    #endif
	g_curr_interface->collect(g_curr_interface);

    #ifdef MTSAFE
	RELEASE_LOCK(g_curr_interface->lock);
    #endif
    }
}

void
gc_collect_all(void)
{
    struct gc_interface **g;

    if (g_curr_interface == NULL)
	g_curr_interface = gc_interface_list[0];

    for (g = gc_interface_list; *g; g++)
    {
    #ifdef MTSAFE
	ACQUIRE_LOCK((*g)->lock);
    #endif

	if ((*g)->collect)
	    (*g)->collect(*g);

    #ifdef MTSAFE
	RELEASE_LOCK((*g)->lock);
    #endif
    }
}

void
gc_del_root(char *ptr)
{
    if (g_curr_interface == NULL)
	g_curr_interface = gc_interface_list[0];

    if (g_curr_interface->del_root)
    {
    #ifdef MTSAFE
	ACQUIRE_LOCK(g_curr_interface->lock);
    #endif

	g_curr_interface->del_root(g_curr_interface, ptr);

    #ifdef MTSAFE
	RELEASE_LOCK(g_curr_interface->lock);
    #endif
    }
}

void
gc_add_root(char *start, char *end)
{
    if (g_curr_interface == NULL)
	g_curr_interface = gc_interface_list[0];

    if (g_curr_interface->add_root)
    {
    #ifdef MTSAFE
	ACQUIRE_LOCK(g_curr_interface->lock);
    #endif

	g_curr_interface->add_root(g_curr_interface, start, end);

    #ifdef MTSAFE
	RELEASE_LOCK(g_curr_interface->lock);
    #endif
    }
}
