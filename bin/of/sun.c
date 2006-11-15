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

/* (c) Copyright 1997-1998 by CodeGen, Inc.  All Rights Reserved. */

/* Words available only on Sun/OpenBoot but not part of OpenFirmware.
 */

#include "defs.h"


EC(f_dl);		/* defined in debug.c */

C(f_dcall_self)	/* $call-self (... method-str method-len -- ?) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Byte *str;
	Int len;
	
	if (inst == NULL)
		return E_NULL_INSTANCE;

	IFCKSP(e, 2, 0);
	POP(e, len);
	POPT(e, str, Byte*);
	return execute_method_name(e, inst, str, len);
}

/* 5.3.5.1  Property array encoding */
C(f_sbus2cpu)		/* sbus-intr>cpu (sbus-intr# -- cpu-inter#) 0x131 */
{
	return NO_ERROR;
}

/* return default load address */
C(f_load_base)		/* load-base (-- addr) */
{
	extern Retcode push_config_int(Environ *e, char *param);
	return push_config_int(e, "load-base");
}

#ifndef MAX_INST_STACK_SIZE
#define MAX_INST_STACK_SIZE	32
#endif
Int g_insttop = 0;
Instance *g_inststk[MAX_INST_STACK_SIZE];

static Retcode 
do_select_dev(Environ *e, Byte *dev, Int len)
{
	Retcode ret;
	Instance *inst;

	IFCKSP(e, 0, 2);

	if (g_insttop >= MAX_INST_STACK_SIZE)
	{
		cprintf(e, "instance stack overflow\n");
		return E_OUT_OF_MEMORY;
	}

	PUSHP(e, dev);
	PUSH(e, len);
	ret = execute_word(e, "open-dev");

	if (ret != NO_ERROR)
		return ret;

	POPT(e, inst, Instance*);

	if (inst == NULL)
		return E_NO_DEVICE;

	g_inststk[g_insttop++] = (Instance*)(uPtr)e->currinst;
	e->currinst = (uPtr)inst;		/* set current instance */
	e->currpkg = inst->package;		/* cd to opened device */
	return NO_ERROR;
}

C(f_select_dev)				/* select-dev (dev-path path-len --) */
{
	Byte *dev;
	Int len;

	IFCKSP(e, 2, 0);
	POP(e, len);
	POPT(e, dev, Byte*);
	return do_select_dev(e, dev, len);
}

C(f_begin_select_dev)		/* begin-select-dev (dev-path path-len --) */
{
	Byte *dev;
	Int devlen, len;
	Retcode ret;

	IFCKSP(e, 2, 0);
	POP(e, devlen);
	POPT(e, dev, Byte*);

	/* open path up to but not including the last device */
	for (len = devlen; len > 0 && dev[len] != '/'; len--)
		;

	if (len == 0 && dev[len] == '/')	/* in case we have "/" */
		len++;

	ret = do_select_dev(e, dev, len);
	
	if (ret != NO_ERROR)
		return ret;
	
	/* finally select the complete path for the current device */
	IFCKSP(e, 0, 2);
	PUSHP(e, dev);
	PUSH(e, devlen);
	return execute_word(e, "find-device");
}

C(f_unselect_dev)		/* unselect-dev (--) */
{
	Instance *inst;
	Retcode ret;

	IFCKSP(e, 0, 1);
	PUSH(e, e->currinst);
	ret = execute_word(e, "close-dev");

	if (g_insttop <= 0)
	{
		cprintf(e, "instance stack underflow\n");
		return E_OUT_OF_MEMORY;
	}

	inst = g_inststk[--g_insttop];
	e->currinst = (uPtr)inst;
	e->currpkg = (inst && inst->package) ? inst->package : NULL;
	return NO_ERROR;
}

C(f_dload)				/* dload ("filename< >" addr --) */
{
	Byte *file = parse_word(e);
	Byte *addr;
	Instance *inst;
	Int len;
	Retcode ret;
	Byte buf[STR_SIZE];

	IFCKSP(e, 1, 2);
	POPT(e, addr, Byte*);
	bprintf(buf, "net:,%P", file);

#ifdef SUN_COMPATIBILITY
	/* change all vertical-bars to slashes - OBP-compatible */
	for (file = buf; (file = strchr(file, '/')) != NULL; )
		*file++ = '|';
#endif

	/* try to open this device */
	PUSHP(e, buf);
	PUSH(e, strlen(buf));
	ret = execute_word(e, "open-dev");
	
	if (ret != NO_ERROR)
		return ret;

	POPT(e, inst, Instance*);
	
	if (inst == NULL)
		return E_ABORT;

	/* try to load at the specified address */
	PUSHP(e, addr);
	ret = execute_method_name(e, inst, "load", CSTR);

	if (ret == NO_ERROR)
		POP(e, len);		/* length of code loaded into memory */

	/* and close the device */
	PUSHP(e, inst);
	(void)execute_word(e, "close-dev");

	return ret;
}

static Retcode
do_method(Environ *e, Byte *device, Byte *method)
{
	Package *savepkg = e->currpkg;
	Cell savelpp = e->linesperpage;
	Retcode ret;
	int key;

	IFCKSP(e, 0, 2);

	/* shut off pagination */
	e->linesperpage = e->lines + 1;

	/* some program is still running */
	if (e->go_running)
	{
		cprintf(e,
			"%s may crash the system since a program is currently running.\n"
				"Do you wish to continue? (y/n): ",
				method);
		key = get_key(e);
		cprintf(e, "\n");

		if (key != 'y' && key != 'Y')
		{
			e->linesperpage = savelpp;
			return E_ABORT;
		}
	}

	/* try to open the device */
	PUSHP(e, device);
	PUSH(e, strlen(device));
	PUSHP(e, method);
	PUSH(e, strlen(method));
	ret = execute_word(e, "execute-device-method");
	e->linesperpage = savelpp;
	
	if (ret != NO_ERROR)
		return ret;

	DROP(e);
	e->currpkg = savepkg;
	return ret;
}

static Retcode
recurse_method_all(Environ *e, Package *node, Byte *type, Byte *method)
{
	Package *pkg;
	Retcode ret;
	Byte *str;
	Int slen;
	Byte buf[STR_SIZE];

	if (node->props &&
			prop_get_str(node->props, "device_type", CSTR, &str, &slen) ==
					NO_ERROR &&
			compare_strs(str, slen, type, CSTR))
	{
		get_device_name(e, node, buf);
		cprintf(e, "%s\n", buf);
		ret = do_method(e, buf, method);
		return ret;
	}

	/* otherwise search all children for devices of this type */
	for (pkg = node->children; pkg; pkg = pkg->link)
	{
		ret = recurse_method_all(e, pkg, type, method);

		if (ret != NO_ERROR)
			return ret;
	}

	return NO_ERROR;
}

static Retcode
do_method_all(Environ *e, Byte *type, Byte *method)
{
	Byte *name = parse_line(e);
	Cell savelpp = e->linesperpage;
	Package *pkg;
	Retcode ret;

	/* shut off pagination */
	e->linesperpage = e->lines + 1;

	if (name && *name)
		pkg = find_device(e, name, PSTR);
	else
		pkg = e->root;

	if (pkg == NULL)
		ret = E_NO_DEVICE;
	else
		ret = recurse_method_all(e, pkg, type, method);

	e->linesperpage = savelpp;
	return ret;
}

C(f_probe_scsi)			/* probe-scsi (--) */
{
	return do_method(e, "scsi", "show-children");
}

C(f_probe_scsi_all)		/* probe-scsi-all ("device-name<eol>"--) */
{
	return do_method_all(e, "scsi", "show-children");
}

C(f_watch_net)			/* watch-net (--) */
{
	return do_method(e, "net", "watch-net");
}

C(f_watch_net_all)		/* watch-net-all ("device-name<eol>"--) */
{
	return do_method_all(e, "network", "watch-net");
}


const Initentry init_sun[] =
{
	{ "$call-self", f_dcall_self, INVALID_FCODE },
	{ "sbus-intr>cpu", f_sbus2cpu, 0x131 },
	{ "load-base", f_load_base, INVALID_FCODE },

	{ "select-dev", f_select_dev, INVALID_FCODE, },
	{ "unselect-dev", f_unselect_dev, INVALID_FCODE },

	{ "begin-select-dev", f_begin_select_dev, INVALID_FCODE },
	{ "end-select-dev", f_unselect_dev, INVALID_FCODE },

	{ "dload", f_dload, INVALID_FCODE },

	{ "probe-scsi", f_probe_scsi, INVALID_FCODE },
	{ "probe-scsi-all", f_probe_scsi_all, INVALID_FCODE },
	{ "watch-net", f_watch_net, INVALID_FCODE },
	{ "watch-net-all", f_watch_net_all, INVALID_FCODE },

	{ NULL, NULL }
};
