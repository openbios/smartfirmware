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


#include "defs.h"

static uLong
msr_read(int num)
{
        uInt valhi;
        uInt vallo;
        __asm __volatile("rdmsr" : "=d" (valhi), "=a" (vallo) : "c" (num));
        return ((uLong)valhi << 32) | vallo;
}

static void
msr_write(uInt num, uLong val)
{
        uInt valhi = (val >> 32) & 0xFFFFFFFFU;;
        uInt vallo = val & 0xFFFFFFFFU;;
        __asm __volatile("wrmsr" : : "d" (valhi), "a" (vallo), "c" (num));
}

C(f_msrget)				/* msr@ ( regnum -- val ) */
{
	Cell reg;
	uLong val;
	IFCKSP(e, 1, 1);
	POP(e, reg);
	val = msr_read(reg);
	PUSH(e, val);
	return NO_ERROR;
}

C(f_msrset)				/* msr! ( val regnum -- ) */
{
	Cell reg;
	Cell val;
	IFCKSP(e, 2, 0);
	POP(e, reg);
	POP(e, val);
	msr_write(reg, val);
	return NO_ERROR;
}

C(f_show_msrs)
{
	return NO_ERROR;
}

const Initentry init_msr[] =
{
	{ "msr@", f_msrget, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(reg -- val) " \
					"get Model Specific Register") },
	{ "msr!", f_msrset, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(val reg -- ) " \
					"set Model Specific Register") },
	{ "show-msrs", f_show_msrs, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- ) " \
					"show Model Specific Register") },

	{ NULL, NULL }
};
