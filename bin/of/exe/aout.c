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

/* (c) Copyright 1999-2000 by CodeGen, Inc.  All Rights Reserved. */

/* BSD AOUT program loader.
 */

/*#define DEBUG*/
#include "defs.h"
#include "exe.h"

#define __LDPGSZ	4096
#define ntohl(n)	BE4(n)

/* The following is copied from FreeBSD 3.4's <sys/imgact_aout.h> */

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)exec.h	8.1 (Berkeley) 6/11/93
 * $FreeBSD: src/sys/sys/imgact_aout.h,v 1.9.2.2 1999/09/01 06:12:10 sef Exp $
 */

#define N_GETMAGIC(ex) \
	( (ex).a_midmag & 0xffff )
#define N_GETMID(ex) \
	( (N_GETMAGIC_NET(ex) == ZMAGIC) ? N_GETMID_NET(ex) : \
	((ex).a_midmag >> 16) & 0x03ff )
#define N_GETFLAG(ex) \
	( (N_GETMAGIC_NET(ex) == ZMAGIC) ? N_GETFLAG_NET(ex) : \
	((ex).a_midmag >> 26) & 0x3f )
#define N_SETMAGIC(ex,mag,mid,flag) \
	( (ex).a_midmag = (((flag) & 0x3f) <<26) | (((mid) & 0x03ff) << 16) | \
	((mag) & 0xffff) )

#define N_GETMAGIC_NET(ex) \
	(ntohl((ex).a_midmag) & 0xffff)
#define N_GETMID_NET(ex) \
	((ntohl((ex).a_midmag) >> 16) & 0x03ff)
#define N_GETFLAG_NET(ex) \
	((ntohl((ex).a_midmag) >> 26) & 0x3f)
#define N_SETMAGIC_NET(ex,mag,mid,flag) \
	( (ex).a_midmag = htonl( (((flag)&0x3f)<<26) | (((mid)&0x03ff)<<16) | \
	(((mag)&0xffff)) ) )

#define N_ALIGN(ex,x) \
	(N_GETMAGIC(ex) == ZMAGIC || N_GETMAGIC(ex) == QMAGIC || \
	 N_GETMAGIC_NET(ex) == ZMAGIC || N_GETMAGIC_NET(ex) == QMAGIC ? \
	 ((x) + __LDPGSZ - 1) & ~(unsigned long)(__LDPGSZ - 1) : (x))

/* Valid magic number check. */
#define	N_BADMAG(ex) \
	(N_GETMAGIC(ex) != OMAGIC && N_GETMAGIC(ex) != NMAGIC && \
	 N_GETMAGIC(ex) != ZMAGIC && N_GETMAGIC(ex) != QMAGIC && \
	 N_GETMAGIC_NET(ex) != OMAGIC && N_GETMAGIC_NET(ex) != NMAGIC && \
	 N_GETMAGIC_NET(ex) != ZMAGIC && N_GETMAGIC_NET(ex) != QMAGIC)


/* Address of the bottom of the text segment. */
/*
 * This can not be done right.  Abuse a_entry in some cases to handle kernels.
 */
#define N_TXTADDR(ex) \
	((N_GETMAGIC(ex) == OMAGIC || N_GETMAGIC(ex) == NMAGIC || \
	N_GETMAGIC(ex) == ZMAGIC) ? \
	((ex).a_entry < (ex).a_text ? 0 : (ex).a_entry & ~__LDPGSZ) : __LDPGSZ)
//#define	N_TXTADDR(ex)	(N_GETMAGIC2(ex) == (ZMAGIC|0x10000) ? 0 : __LDPGSZ)

/* Address of the bottom of the data segment. */
#define N_DATADDR(ex) \
	N_ALIGN(ex, N_TXTADDR(ex) + (ex).a_text)

/* Text segment offset. */
#define	N_TXTOFF(ex) \
	(N_GETMAGIC(ex) == ZMAGIC ? __LDPGSZ : (N_GETMAGIC(ex) == QMAGIC || \
	N_GETMAGIC_NET(ex) == ZMAGIC) ? 0 : sizeof(struct exec))

/* Data segment offset. */
#define	N_DATOFF(ex) \
	N_ALIGN(ex, N_TXTOFF(ex) + (ex).a_text)

/* Relocation table offset. */
#define N_RELOFF(ex) \
	N_ALIGN(ex, N_DATOFF(ex) + (ex).a_data)

/* Symbol table offset. */
#define N_SYMOFF(ex) \
	(N_RELOFF(ex) + (ex).a_trsize + (ex).a_drsize)

/* String table offset. */
#define	N_STROFF(ex) 	(N_SYMOFF(ex) + (ex).a_syms)

/*
 * Header prepended to each a.out file.
 * only manipulate the a_midmag field via the
 * N_SETMAGIC/N_GET{MAGIC,MID,FLAG} macros in a.out.h
 */

struct exec {
     unsigned long	a_midmag;	/* flags<<26 | mid<<16 | magic */
     unsigned long	a_text;		/* text segment size */
     unsigned long	a_data;		/* initialized data size */
     unsigned long	a_bss;		/* uninitialized data size */
     unsigned long	a_syms;		/* symbol table size */
     unsigned long	a_entry;	/* entry point */
     unsigned long	a_trsize;	/* text relocation size */
     unsigned long	a_drsize;	/* data relocation size */
};

/* a_magic */
#define	OMAGIC		0407	/* old impure format */
#define	NMAGIC		0410	/* read-only text */
#define	ZMAGIC		0413	/* demand load format */
#define QMAGIC          0314    /* "compact" demand load format */

/* a_mid */
#define	MID_ZERO	0	/* unknown - implementation dependent */
#define	MID_SUN010	1	/* sun 68010/68020 binary */
#define	MID_SUN020	2	/* sun 68020-only binary */
#define MID_I386	134	/* i386 BSD binary */
#define MID_SPARC	138	/* sparc */
#define	MID_HP200	200	/* hp200 (68010) BSD binary */
#define	MID_HP300	300	/* hp300 (68020+68881) BSD binary */
#define	MID_HPUX	0x20C	/* hp200/300 HP-UX binary */
#define	MID_HPUX800     0x20B   /* hp800 HP-UX binary */

/*
 * a_flags
 */
#define EX_PIC		0x10	/* contains position independent code */
#define EX_DYNAMIC	0x20	/* contains run-time link-edit info */
#define EX_DPMASK	0x30	/* mask for the above */

/* END of code copied from FreeBSD header file */


#ifdef AOUT_MAGIC_0
Bool
aout_is_exec(Environ *e, uByte *load, uInt loadlen)
{
	struct exec *ex = (struct exec*)load;
	int m;

	DPRINTF(("aout_is_exec: midmag=%#x badmagic=%d mid=%#x\n",
			ex->a_midmag, N_BADMAG(*ex), N_GETMID(*ex)));

	if (N_BADMAG(*ex))
		return FALSE;

	m = N_GETMID(*ex);

	if (m == AOUT_MAGIC_0)
		return TRUE;

	#ifdef AOUT_MAGIC_1
		if (m == AOUT_MAGIC_1)
			return TRUE;
	#endif

	#ifdef AOUT_MAGIC_2
		if (m == AOUT_MAGIC_2)
			return TRUE;
	#endif

	#ifdef AOUT_MAGIC_3
		if (m == AOUT_MAGIC_3)
			return TRUE;
	#endif

	return FALSE;
}

/* prepare to run the aout image already verified to be aout
 */
Retcode
aout_load(Environ *e, uByte *load, uInt loadlen, uLong *entrypoint)
{
	struct exec exbuf = *(struct exec*)load;
	struct exec *ex = &exbuf;	/* copy header to support overlapping loads */
	char *os = get_config(e, "operating-system", CSTR);
	uInt loadoff = 0;

	if (!aout_is_exec(e, load, loadlen))
		return E_BAD_IMAGE;

	*entrypoint = N_TXTADDR(*ex);

	if (*entrypoint < ex->a_entry)
		*entrypoint = ex->a_entry;

	if (AOUT_MAGIC_0 == MID_I386)
	{
		/* hacks for booting various x86 OSes */
		if (compare_strs(os, CSTR, "openbsd", CSTR))
		{
			DPRINTF(("aout_load: openbsd: entrypoint=%#lx\n", *entrypoint));
			*entrypoint &= 0x00FFFFFF;
			loadoff = *entrypoint - sizeof *ex - N_TXTADDR(*ex);
		}
	}

	/* copy the text segment */
	DPRINTF(("aout_load: textaddr=%#x load=%#x offset=%#x\n",
			loadoff + N_TXTADDR(*ex), load, N_TXTOFF(*ex)));
	memmove((char*)(loadoff + N_TXTADDR(*ex)),
			load + N_TXTOFF(*ex), ex->a_text);

#ifdef MACHINE_CLAIM_MEMORY
	/* claim/map this area if requested */
	MACHINE_CLAIM_MEMORY(e, (char*)(loadoff + N_TXTADDR(*ex)), ex->a_text);
#endif /* MACHINE_CLAIM_MEMORY */

	/* copy the data segment */
	DPRINTF(("aout_load: dataaddr=%#x load=%#x offset=%#x\n",
			loadoff + N_DATADDR(*ex), load, N_DATOFF(*ex)));
	memmove((char*)(loadoff + N_DATADDR(*ex)),
			load + N_DATOFF(*ex), ex->a_data);

	/* zero bss segment */
	DPRINTF(("aout_load: bssaddr=%#x size=%#x\n",
			loadoff + N_DATADDR(*ex) + ex->a_data, ex->a_bss));
	memset((char*)(loadoff + N_DATADDR(*ex)) + ex->a_data, 0, ex->a_bss);

#ifdef MACHINE_CLAIM_MEMORY
	/* claim/map this area if requested */
	MACHINE_CLAIM_MEMORY(e, (char*)(loadoff + N_DATADDR(*ex)),
			ex->a_data + ex->a_bss);
#endif /* MACHINE_CLAIM_MEMORY */

	return NO_ERROR;
}

uInt
aout_length(Environ *e, uByte *load, uInt loadlen)
{
	struct exec *ex = (struct exec*)load;

	if (!aout_is_exec(e, load, loadlen))
		return 0;

	return ex->a_text + ex->a_data + ex->a_bss;
}

Sym_table *
aout_symbols(Environ *e, uByte *load, uInt loadlen)
{
	Sym_table *tab = NULL;

	return tab;
}
#endif /* AOUT_MAGIC_0 */

const Exec_entry exec_aout =
{
	"aout",
#ifdef AOUT_MAGIC_0
	aout_is_exec,
	aout_load,
	aout_length,
	aout_symbols
#else
	NULL,
#endif /* AOUT_MAGIC_0 */
};
