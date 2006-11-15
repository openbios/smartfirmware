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

/* (c) Copyright 1998-2001 by CodeGen, Inc.  All Rights Reserved. */

/* Generic NVRAM support for embedded systems.
   This is intended as an outline for how to handle NVRAM parameters
   and requires customization.
*/

/*#define DEBUG*/

#include "defs.h"


/* these magic values must not be legal 7-bit ASCII characters nor zero */
#define NVRAM_MAGIC_1	0xBE
#define NVRAM_MAGIC_2	0xEF

#define NVRAM_HEADER		6
#define NVRAM_MAX		(g_nvram_size - 8)

/* NVRAM layout is very simple:

	0xBE 0xEF len.hi len.lo check.hi check.lo param1...\0 value1...\0 p2...\0 v2...\0 ... \0
	
	len is the length of the actual parameters not including the optional \0
	denoting the end of parameters nor the NVRAM_HEADER.

	check is the sum of all of the bytes including the header.

	The magic-value at the header is used to detect if NVRAM been corrupted.
	When NVRAM is written out, these are first written as zero.  After
	the last bytes of NVRAM are written, then the magic bytes are filled in.
	If magic values aren't found when reading, NVRAM is corrupt/uninited.
 */


/* This is used to initialize non-volatile RAM from the machdep.c file.
   It may need to be customized or pointed to some header file if the
   machdep.c version has been modified.
 */
extern struct nvram_data
{
	char *name;
	char *val;
} g_nvram[];


/* used to manipulate in-memory copy before updating real NVRAM, so
   we can more easily delete entries by simply rewriting all of NVRAM
 */
static Byte *g_nvram_image = (Byte *)0;
static Byte *g_nvram_base = (Byte *)0;
static Byte g_nvram_mini[256];
static Int g_nvram_size = -1;
static Int g_nvram_len = -1;


/* store entire g_nvram_image array to real NVRAM
 */
static void
store_nvram(void)
{
	int i;
	uInt checksum = 0;

	g_nvram_base[0] = NVRAM_MAGIC_1;	/* set the magic numbers */
	g_nvram_base[1] = NVRAM_MAGIC_2;
	g_nvram_base[2] = g_nvram_len >> 8;	/* set the length */
	g_nvram_base[3] = g_nvram_len & 0xFF;
	g_nvram_base[4] = 0;
	g_nvram_base[5] = 0;

	for (i = 0; i < g_nvram_len + NVRAM_HEADER; i++)
		checksum += g_nvram_base[i] & 0xFF;

	g_nvram_base[4] = -checksum >> 8;
	g_nvram_base[5] = -checksum & 0xFF;
	g_nvram_base[NVRAM_HEADER + g_nvram_len] = 0;

	machine_nvram_write(g_e, g_nvram_base, g_nvram_len + NVRAM_HEADER + 1);
}

/* load real NVRAM into g_nvram_image array
 */
static void
load_nvram(void)
{
	if (g_nvram_len < 0)
	{
		g_nvram_size = machine_nvram_size(g_e);
		g_nvram_base = malloc(g_nvram_size);

		if (g_nvram_base == (Byte *)0)
		{
			g_nvram_base = g_nvram_mini;
			g_nvram_size = 256;
		}

		g_nvram_image = g_nvram_base + NVRAM_HEADER;
	}

	if (machine_nvram_read(g_e, g_nvram_base, &g_nvram_len) != NO_ERROR)
	{
		/* corrupt or not initialized - clear it */
		cprintf(g_e, "NVRAM is corrupt, reseting to default values\n");
		g_nvram_len = 0;
		g_nvram_image[0] = '\0';
	}
	else
		g_nvram_len -= NVRAM_HEADER + 1;
}

/* Set the specified parameter in nvram to the specified value.
 */
Retcode
set_nvram(Environ *e, Byte *param, Int plen, Byte *val, Int vlen)
{
	Int i;
	Int j = -1;
	Byte *v;
	Int add = 0;
	Int rem = 0; 
	Byte *s;

	setstrlen(&param, &plen);
	setstrlen(&val, &vlen);

	if (g_nvram_len < 0)
		load_nvram();

	/* find location and size of parameter in g_nvram_image */
	for (i = 0; i < g_nvram_len; )
	{
		/* check if param in g_nvram_image currently */
		if (compare_strs(param, plen, g_nvram_image + i, CSTR))
		{
			j = i;		/* record position */
			i += strlen(g_nvram_image + i) + 1;
			rem = plen + strlen(g_nvram_image + i) + 2;
			break;
		}

		/* skip the parameter and its value */
		i += strlen(g_nvram_image + i) + 1;
		i += strlen(g_nvram_image + i) + 1;
	}

	/* do not save the value if it is the default value */
	v = get_nvram_default(e, param, plen);
	DPRINTF(("set_nvram: param=%S(%d) v=%s(%d) val=%S(%d)\n",
			param, plen, plen, v, strlen(v), val, vlen, vlen));

	if ((v == NULL || strlen(v) == 0) ? (vlen != 0) :
			(strlen(v) != vlen || strncmp(v, val, vlen) != 0))
		add = plen + vlen + 2;

	DPRINTF(("set_nvram: prevlen=%d, add=%d, rem=%d\n", g_nvram_len, add, rem));

	if (g_nvram_len + (add - rem) >= NVRAM_MAX)
		return E_OUT_OF_PROM;

	/* if old value, move everything down */
	if (rem)
	{
		memmove(g_nvram_image + j, g_nvram_image + j + rem,
				g_nvram_len - (j + rem));
		g_nvram_len -= rem;
	}

	if (add)
	{
		/* append it to end of nvram */
		s = g_nvram_image + g_nvram_len;
		memcpy(s, param, plen);
		s += plen;
		*s++ = '\0';
		memcpy(s, val, vlen);
		s += vlen;
		*s++ = '\0';
		*s = '\0';
		g_nvram_len += add;
	}

	/* finally store it */
	DPRINTF(("set_nvram: newlen=%d\n", g_nvram_len));
	store_nvram();
	return NO_ERROR;
}

/* This should get the default value of the specified parameter in nvram.
   The typical implementation simply reads the static array above.
 */
Byte *
get_nvram_default(Environ *e, Byte *param, Int plen)
{
	struct nvram_data *nv;
	
	for (nv = g_nvram; nv->name; nv++)
		if (compare_strs(param, plen, nv->name, CSTR))
			return nv->val;
	
	return NULL;
}

/* This should get the value of the specified parameter in nvram.
   The static array above should be read as a fall-back if nvram
   is not readable or has become corrupt.
 */
Byte *
get_nvram(Environ *e, Byte *param, Int plen)
{
	Int i;

	setstrlen(&param, &plen);

	if (g_nvram_len < 0)
		load_nvram();

	for (i = 0; i < g_nvram_len; )
	{
		if (compare_strs(param, plen, g_nvram_image + i, CSTR))
		{
			i += strlen(g_nvram_image + i) + 1;
			return g_nvram_image + i;
		}

		/* skip the parameter and its value */
		i += strlen(g_nvram_image + i) + 1;
		i += strlen(g_nvram_image + i) + 1;
	}

	return get_nvram_default(e, param, plen);
}

/* This should create a new word in nvram.  Space should be allocated
   for maxlen bytes if necessary.  For this implementation, it does
   not need to do anything.
 */
Byte *
new_nvram(Environ *e, Byte *param, Int plen, Int maxlen)
{
	return NULL;
}

/* This is to make sure that any garbage gets flushed out of nvram.
   It is only called from the user-interface "set-defaults".
 */
Retcode
reset_nvram(Environ *e)
{
	struct nvram_data *nv;
	Retcode ret;

	for (nv = g_nvram; nv->name; nv++)
	{
		ret = prop_set_str(e->options->props, nv->name, CSTR, nv->val, CSTR);

		if (ret != NO_ERROR)
			return ret;
	}

	g_nvram_image[0] = '\0';
	g_nvram_len = 0;
	store_nvram();
	return NO_ERROR;
}


/* This is needed to initialize the /options node from nvram at bootup.
 */
CC(init_options_from_nvram)
{
	struct nvram_data *nv;
	Int i, l;
	Retcode ret;

	/* first create the aliases under /options for our static defaults */
	for (nv = g_nvram; nv->name; nv++)
	{
		ret = prop_set_str(e->options->props, nv->name, CSTR, nv->val, CSTR);

		if (ret == NO_ERROR)
			ret = create_prop_alias(e, nv->name, CSTR);

		if (ret != NO_ERROR)
			return ret;
	}

	if (g_nvram_len < 0)
		load_nvram();

	/* then override /options with all vars in NVRAM */
	for (i = 0; i < g_nvram_len; )
	{
		l = strlen(g_nvram_image + i) + 1;
		DPRINTF(("init_nvopts: g_nvram_len=%d i=%d l=%d param=%s val=%s\n",
				g_nvram_len, i, l, g_nvram_image + i, g_nvram_image + i + l));

		/* do not save password under options */
		if (compare_strs(g_nvram_image + i, CSTR, "security-password", CSTR))
			ret = prop_set_str(e->options->props, g_nvram_image + i, CSTR, "", CSTR);
		else
			ret = prop_set_str(e->options->props, g_nvram_image + i, CSTR,
					g_nvram_image + i + l, CSTR);

		if (ret == NO_ERROR)
			ret = create_prop_alias(e, g_nvram_image + i, CSTR);

		if (ret != NO_ERROR)
			break;

		i += l + strlen(g_nvram_image + i + l) + 1;
		DPRINTF(("init_nvopts: i=%d l=%d\n", i, l));
	}

	return NO_ERROR;
}

/* This is used to override nvram parameters arguments by those passed in
   to main().  The args must contain strings of "-parameter value" pairs.
 */
Retcode
machine_init_args(Environ *e, int argc, char *argv[])
{
	Retcode ret = NO_ERROR;
	Retcode ret2;
	int i;

	if (argc != 0)
	    dprintf("machine_init_args: argc %d, argv %p\n", argc, argv);

	if (e->options->props == NULL)
		return E_INIT_ERROR;

	for (i = 1; i < argc; i++)
	{
		int j;

		for (j = 0; argv[i][j]; j++)
			if (argv[i][j] == '=')
				break;

		if (argv[i][0] == '-' && argv[i][j] == '=')
		{
			argv[i][j] = '\0';
			ret2 = set_nvram(e, &argv[i][1], CSTR, &argv[i][j + 1], CSTR);
			argv[i][j] = '=';

			if (ret == NO_ERROR)
				ret = ret2;
		}
	}

	return ret;
}
