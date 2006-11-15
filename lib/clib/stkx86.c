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

/* Copyright (c) 1994-2003 by Thomas J. Merritt and Parag Patel.
 * All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#include "stdlibx.h"
#include "stdiox.h"
#include "stringx.h"
#include "stkback.h"

#ifdef __FreeBSD__
extern int _init;
static unsigned long start_addr = (unsigned long)&_init;
#else
static unsigned long start_addr = 1024 * 1024;
#endif

static char const *progfile = NULL;

void
programfilename(char const *name)
{
    progfile = name;
}

char const *
addrlabel(unsigned long addr, boolean syms)
{
    static char label[BUFSIZ];
    Dl_info dlinfo;

    if (syms && dladdr((void*)addr, &dlinfo) && dlinfo.dli_saddr)
    {
	if (dlinfo.dli_saddr == (void*)addr)
	    sprintf(label, "%s", dlinfo.dli_sname);
	else
	    sprintf(label, "%s+0x%lX", dlinfo.dli_sname,
		    addr - (unsigned long)dlinfo.dli_saddr);
    }
    else
	sprintf(label, "0x%08lX", addr);

    return label;
}

void
backtrace(boolean syms)
{
    long *bp = &((long *)&syms)[-2];
    static boolean inbacktrace = FALSE;

    if (inbacktrace)
	return;

    inbacktrace = TRUE;
    fprintf(stderr, "\r***STACK BACKTRACE***\r\n");

    /* while (bp[1] > 1) */
    while (bp != NULL && (unsigned long)bp[1] >= start_addr)
    {
	/* print address of caller */
	unsigned char *pc = (unsigned char *)bp[1];
	char const *label = addrlabel((long)pc, syms);
	int labellen = strlen(label);

	if (labellen < 8)
	    fprintf(stderr, "%s\t\t", label);
	else if (labellen < 16)
	    fprintf(stderr, "%s\t", label);
	else
	    fprintf(stderr, "%s\n\t\t", label);

	/* print address of callee if known */
	/* 11101000 == CALL disp32 */
	if (pc[-5] == 0xE8)
	{
	    fprintf(stderr, "%s", addrlabel((long)pc + (((long)pc[-1] << 24) |
		    ((long)pc[-2] << 16) | ((long)pc[-3] << 8) | pc[-4]),
		    syms));
	}
	else
	    fprintf(stderr, "???");

	/* print paramter of call.  max of five. */
	/* 10000011 11000100 == ADDL $n,%esp */
	if (pc[0] == 0x83 && pc[1] == 0xC4)
	    switch (pc[2])
	    {
	    case 0:		/* No int's */
		fprintf(stderr, "()\r\n");
		break;
	    case 4:		/* One int */
		fprintf(stderr, "(0x%lX)\r\n",
			bp[2]);
		break;
	    case 8:		/* Two int's */
		fprintf(stderr, "(0x%lX, 0x%lX)\r\n",
			bp[2], bp[3]);
		break;
	    case 12:		/* Three int's */
		fprintf(stderr, "(0x%lX, 0x%lX, 0x%lX)\r\n",
			bp[2], bp[3], bp[4]);
		break;
	    case 16:		/* Four int's */
		fprintf(stderr, "(0x%lX, 0x%lX, 0x%lX, 0x%lX)\r\n",
			bp[2], bp[3], bp[4], bp[5]);
		break;
	    default:		/* Don't know */
		fprintf(stderr, "(0x%lX, 0x%lX, 0x%lX, 0x%lX, 0x%lX)\r\n",
			bp[2], bp[3], bp[4], bp[5], bp[6]);
		break;
	    }
	else			/* Don't know */
	    fprintf(stderr, "(0x%lX, 0x%lX, 0x%lX, 0x%lX, 0x%lX)\r\n",
		    bp[2], bp[3], bp[4], bp[5], bp[6]);

	if (bp[0] == 0)
	    break;

	/* next frame point must be on the stack */
	if (bp[0] <= (long)bp)
	{
	    fprintf(stderr, "bad frame pointer 0x%lX\r\n", bp[0]);
	    break;
	}

	bp = (long *)bp[0];
    }

    inbacktrace = FALSE;
}

int
stacksnapshot(int size, uInt *stack, int skip)
{
    unsigned *bp = &((unsigned *)&size)[-2];
    int count = 0;
    int i;

    /* while (count < size && bp[1] >= 0) */
    while (count < size && bp != NULL && (unsigned long)bp[1] >= start_addr)
    {
	if (!skip)
	    stack[count++] = (uInt)bp[1];
	else
	    skip--;

	if (bp[0] == 0)
	    break;

	if (bp[0] <= (long)bp)
	{
	    fprintf(stderr, "bad frame pointer 0x%X\r\n", bp[0]);
	    break;
	}

	bp = (unsigned *)(bp[0]);
    }

    for (i = count; i < size; i++)
	stack[i] = 0UL;

    return count;
}

#ifdef EBUG

void
func(int arg)
{
    backtrace(TRUE);
    backtrace(FALSE);
}

int
main(int argc, const char *argv[])
{
    programfilename(argv[0]);
    func(32);
    return 0;
}

#endif	/* EBUG */
