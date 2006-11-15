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

/* Routines for managing partitions and filesystems under disk devices.
   This code is intended for use by built-in C disk device drivers.
 */

#include "defs.h"
#include "fs.h"


static Bool g_loading = FALSE;

Retcode
filesys_read_bytes(Environ *e, Instance *disk, uLong loc, uLong size,
		uByte *buf)
{
	Retcode ret;
	uLong val;

	if (e == NULL || disk == NULL || buf == NULL)
		return E_ABORT;

	DPRINTF((
		"filesys_read_bytes: e=%p disk=%p loc=%#lx size=%#lx buf=%p\n",
			e, disk, loc, size, buf));
	IFCKSP(e, 0, 2);

#ifdef __LONGLONG
	if (sizeof loc > sizeof (uInt))
	{
		PUSH(e, loc & QUADLET_MASK);					/* pos.lo */
		PUSH(e, (loc >> QUADLET_SIZE) & QUADLET_MASK);	/* pos.hi */
	}
	else
#endif
	{
		PUSH(e, loc);		/* pos.lo */
		PUSH(e, 0);			/* pos.hi */
	}

	/* seek (pos.lo pos.hi -- status) */
	DPRINTF(("filesys_read_bytes: package->dict->list=%p name=%p/%P\n",
			disk->package->dict->list,
			disk->package->dict->list->name,
			disk->package->dict->list->name));

	ret = execute_method_name(e, disk, "seek", CSTR);
	DPRINTF(("filesys_read_bytes: seek: %s (%d)\n", err2str(ret), ret));

	if (ret != NO_ERROR)
		return ret;

	POP(e, val);			/* status */

	if (val != 0 && val != 1)
		return E_SEEK_ERROR;

	/* read (addr len -- actual) */
	PUSHP(e, buf);
	PUSH(e, size);
	ret = execute_method_name(e, disk, "read", CSTR);
	DPRINTF(("filesys_read_bytes: read: %s (%d)\n", err2str(ret), ret));

	if (ret != NO_ERROR)
		return ret;

	POP(e, val);			/* actual */

	if (val != size)
		ret = E_READ_ERROR;

	if (g_loading)
	{
		if (((loc >> 10) & 0x1F) == 0)
		{
		    if (diagnostic_mode(e))
			cprintf(e, "#");
		    else
			spin_cursor(e);
		}
	}

	DPRINTF(("filesys_read_bytes: return %s (%d)\n", err2str(ret), ret));
	return ret;
}


Retcode
file_system(Environ *e, Filesys_action what, Instance *disk,
		Byte *path, uLong loc, uLong start, uByte *buf, uInt size,
		uByte *retbuf, uLong *val)
{
	Retcode ret;
	int i;
	Retcode (*action)(Environ *e, Filesys_action what, Instance *disk,
			Byte *path, uLong loc, uLong start, uByte *buf, uInt size,
			uByte *retbuf, uLong *val);

	DPRINTF(("file_system: e=%p disk=%p loc=%#lx start=%#lx path=%p buf=%p\n",
			e, disk, loc, start, path, buf));

	ret = filesys_read_bytes(e, disk, loc, size, buf);

	if (ret != NO_ERROR)
		return ret;

	for (i = 0; g_filesys[i] != NULL; i++)
	{
		Filesys *fs = g_filesys[i];

		DPRINTF(("file_system: probing filesys %s\n", fs->name));

		if (fs->action == NULL)
			continue;

		/* recursive filesys actions are not allowed here */
		action = fs->action;
		fs->action = NULL;

		if (what == FS_LOAD)
			g_loading = TRUE;

		ret = action(e, what, disk, path, loc, start, buf, size, retbuf, val);
		g_loading = FALSE;
		fs->action = action;

		/* R_END or NO_ERROR will stop probing for any more filesystems */
		if (ret != E_NO_FILESYS)
			break;
	}

	DPRINTF(("file_system: return %s (%d)\n", err2str(ret), ret));
	return ret;
}


C(f_dlist_files)	/* $list-files ("directory<eol>" dev-str dev-len --) */
{
	Int alen = PSTR;
	Byte *args;
	Instance *disk;
	Retcode ret = NO_ERROR;

	IFCKSP(e, 2, 2);

	args = parse_line(e);		/* line is optional file-sys dir + args */
	setstrlen(&args, &alen);
	DPRINTF(("list-files: args=\"%S\" alen=%d\n", args, alen, alen));

	/* open the device - device path is already on stack */
	ret = execute_word(e, "open-dev");
	
	if (ret != NO_ERROR)
		return ret;

	POPT(e, disk, Instance*);
	
	if (disk == NULL)
		return E_NULL_INSTANCE;
	
	/* call the list-files method with dir-name/args on the stack */
	PUSHP(e, args);
	PUSH(e, alen);
	ret = execute_method_name(e, disk, "list-files", CSTR);

	if (ret == E_NO_METHOD)
	{
		cprintf(e, "no support for listing files\n");
		ret = NO_ERROR;
	}

	/* and close the device */
	PUSHP(e, disk);
	(void)execute_word(e, "close-dev");

	DPRINTF(("list-files: return %s (%d)\n", err2str(ret), ret));
	return ret;
}

C(f_list_files)		/* list-files ("device:args< >directory<eol>" --) */
{
	Int dlen = PSTR;
	Byte *dev = parse_word(e);

	IFCKSP(e, 0, 2);

	/* make sure we have a device path */
	if (dev == NULL || !*dev)
		return E_EMPTY_STRING;

	setstrlen(&dev, &dlen);
	PUSHP(e, dev);
	PUSH(e, dlen);
	return f_dlist_files(e);
}

const Initentry init_filesystem[] =
{
	{ "$list-files",   f_dlist_files,  INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"directory<eol>\" dev-str dev-len --)\n" \
				"\tlist files/directories on block device")},
	{ "list-files",    f_list_files,   INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"device:args< >directory<eol>\" --)\n" \
				"\tlist files/directories on block device")},
	{ NULL,            NULL },
};
