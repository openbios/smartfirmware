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

/* (c) Copyright 2001 by CodeGen, Inc.  All Rights Reserved. */

/* Console virtual device */

/*#define DEBUG	*/
#include "defs.h"

#define MAX_DEVS	4

struct self
{
	int numdevs;
	Instance *odevs[MAX_DEVS];
	Instance *idevs[MAX_DEVS];
};

typedef struct self Self;

Retcode
console_open_dev(Environ *e, char *name, Instance **instp)
{
	Retcode ret;
	Int len = strlen(name);

	IFCKSP(e, 0, 2);
	PUSHP(e, name);
	PUSH(e, len);
	ret = execute_word(e, "open-dev");

	if (ret != NO_ERROR)
	{
		*instp = NULL;
		return ret;
	}

	IFCKSP(e, 1, 0);
	POPT(e, *instp, Instance *);

	if (*instp == NULL)
		return E_NO_DEVICE;

	return NO_ERROR;
}

C(f_console_open)			/* open (-- okay?) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	char *devs;
	char *dev;
	char *idev;
	int i;
	Retcode ret;
	Retcode rret = NO_ERROR;

	IFCKSP(e, 0, 1);

	if (inst == NULL)
		return E_NULL_INSTANCE;

	s = (Self *)malloc(sizeof *s);
	
	if (s == NULL)
		return E_OUT_OF_MEMORY;

	memset(s, 0, sizeof *s);
	inst->self = s;

	devs = get_config(e, "console-devices", CSTR);

	if (devs == NULL)
		devs = "screen+keyboard";

	for (i = 0; i < MAX_DEVS && devs; i++)
	{
		dev = devs;
		devs = strchr(dev, ';');

		if (devs)
			*devs = '\0';

		idev = strchr(dev, '+');

		if (idev)
			*idev = '\0';

		ret = console_open_dev(e, dev, &s->odevs[i]);

		if (ret != NO_ERROR && ret != E_NO_DEVICE)
		{
			if (rret == NO_ERROR)
				rret = ret;

			s->odevs[i] = NULL;
		}
		else if (ret == E_NO_DEVICE)
			s->odevs[i] = NULL;

		if (idev)
		{
			ret = console_open_dev(e, idev + 1, &s->idevs[i]);

			if (ret != NO_ERROR && ret != E_NO_DEVICE)
			{
				if (rret == NO_ERROR)
					rret = ret;

				s->idevs[i] = NULL;
			}
			else if (ret == E_NO_DEVICE)
				s->idevs[i] = NULL;

			*idev = '+';
		}
		else
			s->idevs[i] = s->odevs[i];

		if (devs)
			*devs++ = ';';
	}

	if (i >= MAX_DEVS && devs)
		cprintf(e, "Extra devices in console-device ignored, max %d\n",
				MAX_DEVS);

	s->numdevs = i;

	if (rret != NO_ERROR)
		PUSH(e, FFALSE);
	else
		PUSH(e, FTRUE);

	return NO_ERROR;
}

Retcode
console_close_dev(Environ *e, Instance *inst)
{
	if (inst)
	{
		PUSHP(e, inst);
		return execute_word(e, "close-dev");
	}

	return NO_ERROR;
}

C(f_console_close)			/* close (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	int i;

	if (inst == NULL)
		return E_NULL_INSTANCE;

	s = inst->self;
	
	if (s == NULL)
		return E_BAD_INSTANCE;

	for (i = 0; i < s->numdevs; i++)
	{
		console_close_dev(e, s->odevs[i]);

		if (s->odevs[i] != s->idevs[i])
				console_close_dev(e, s->idevs[i]);
	}

	free(s);
	return NO_ERROR;
}

Retcode
console_op(Environ *e, Instance *inst, char *op, Byte *addr, Int len, Int *actual)
{
	Retcode ret;

	if (inst == NULL)
	{
		*actual = -2;
		return NO_ERROR;
	}

	IFCKSP(e, 0, 2);
	PUSHP(e, addr);
	PUSH(e, len);

	/* call op method on inst */
	ret = execute_method_name(e, inst, op, CSTR);

	IFCKSP(e, 1, 0);
	POP(e, *actual);
	return ret;
}

Retcode
console_read(Environ *e, Instance *inst, Byte *addr, Int len, Int *actual)
{
	return console_op(e, inst, "read", addr, len, actual);
}

Retcode
console_write(Environ *e, Instance *inst, Byte *addr, Int len, Int *actual)
{
	return console_op(e, inst, "write", addr, len, actual);
}

C(f_console_read)			/* read (addr len -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int len;
	Int actual = 0;
	Byte *addr;
	Self *s;
	Retcode ret = NO_ERROR;
	int i;

	DPRINTF(("enter console_read\n"));

	if (inst == NULL)
		return E_NULL_INSTANCE;

	s = inst->self;
	
	if (s == NULL)
		return E_BAD_INSTANCE;

	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);

	for (i = 0; i < s->numdevs; i++)
	{
		ret = console_read(e, s->idevs[i], addr, len, &actual);
		DPRINTF(("console_read: dev=%d, ret=%d, actual=%d\n", i, ret, actual));

		if (ret != NO_ERROR)
			break;

		if (actual != -2 && actual != 0)
			break;
	}

	PUSH(e, actual == 0 ? -2 : actual);
	return ret;
}

C(f_console_write)			/* write (addr len -- actual) */
{
	Retcode ret;
	Retcode rret = NO_ERROR;
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int len;
	Int actual = 0;
	Int ractual = 0;
	Byte *addr;
	Self *s;
	int i;

	DPRINTF(("enter console_write\n"));

	if (inst == NULL)
		return E_NULL_INSTANCE;

	DPRINTF(("console_write: good instance\n"));

	s = inst->self;
	
	if (s == NULL)
		return E_BAD_INSTANCE;

	DPRINTF(("console_write: good self\n"));

	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	DPRINTF(("console_write: addr=%#x len=%d\n", addr, len));

	for (i = 0; i < s->numdevs; i++)
	{
		actual = 0;
		ret = console_write(e, s->odevs[i], addr, len, &actual);
		DPRINTF(("console_read: dev=%d, ret=%d, actual=%d\n", i, ret, actual));

		if (ret != NO_ERROR && rret == NO_ERROR)
			rret = ret;

		if (i == 0 || (actual >= 0 && actual < ractual))
			ractual = actual;
	}

	PUSH(e, ractual);
	DPRINTF(("console_write: actual=%d\n", ractual));
	return rret;
}

C(f_console_draw_logo)		/* draw-logo ( line# addr width height -- ) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	Retcode ret;
	Retcode rret = NO_ERROR;
	int i;
	Cell height;
	Cell width;
	Cell addr;
	Cell linenum;

	DPRINTF(("enter console_draw_logo\n"));

	if (inst == NULL)
		return E_NULL_INSTANCE;

	s = inst->self;
	
	if (s == NULL)
		return E_BAD_INSTANCE;

	IFCKSP(e, 4, 0);
	POP(e, height);
	POP(e, width);
	POP(e, addr);
	POP(e, linenum);

	for (i = 0; i < s->numdevs; i++)
	{
		PUSH(e, linenum);
		PUSH(e, addr);
		PUSH(e, width);
		PUSH(e, height);
		ret = execute_method_name(e, s->odevs[i], "draw-logo", CSTR);

		if (ret == E_NO_METHOD)
			DROPN(e, 4);
		else if (ret != NO_ERROR && rret == NO_ERROR)
			rret = ret;
	}

	return rret;
}

static const Initentry console_methods[] =
{
	{ "open",          f_console_open,       INVALID_FCODE },
	{ "close",         f_console_close,      INVALID_FCODE },
	{ "read",          f_console_read,       INVALID_FCODE },
	{ "write",         f_console_write,      INVALID_FCODE },
	{ "draw-logo",     f_console_draw_logo,  INVALID_FCODE },
	{ NULL,            NULL },
};

CC(console_install)
{
	Package *pkg;

	pkg = new_pkg_name(e->currpkg, "console");

	if (pkg == NULL)
		return E_OUT_OF_MEMORY;

	prop_set_str(pkg->props, "device_type", CSTR, "display", CSTR);
	return init_entries(e, pkg->dict, console_methods);
}
