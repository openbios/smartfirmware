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

/* Driver for ARM 720T CPU, handles machine registers */

/*#define DEBUG*/

#include "defs.h"

C(f_arm_dotregisters)			/* .registers (--) */
{
	cprintf(e, "R0-7:   %08X %08X %08X %08X  %08X %08X %08X %08X\n",
			e->mstate.r[0], e->mstate.r[1], e->mstate.r[2], e->mstate.r[3],
			e->mstate.r[4], e->mstate.r[5], e->mstate.r[6], e->mstate.r[7]);
	cprintf(e, "R8-15:  %08X %08X %08X %08X  %08X %08X %08X %08X\n",
			e->mstate.r[8], e->mstate.r[9], e->mstate.r[10], e->mstate.r[11],
			e->mstate.r[12], e->mstate.r[13], e->mstate.r[14], e->mstate.r[15]);
	cprintf(e, "PC:     %08X                        PSR: %08X\n",
			e->mstate.r[15], e->mstate.psr);
	
	return NO_ERROR;
}

const Initentry init_arm[] =
{
	{ ".registers",      f_arm_dotregisters,          INVALID_FCODE },
	{ "psr", (Command)offsetof(Environ, mstate.psr),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- psr) " \
					"processor status register") },
	{ "r0", (Command)offsetof(Environ, mstate.r[0]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r0) " \
					"processor register r0") },
	{ "r1", (Command)offsetof(Environ, mstate.r[1]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r1) " \
					"processor register r1") },
	{ "r2", (Command)offsetof(Environ, mstate.r[2]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r2) " \
					"processor register r2") },
	{ "r3", (Command)offsetof(Environ, mstate.r[3]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r3) " \
					"processor register r3") },
	{ "r4", (Command)offsetof(Environ, mstate.r[4]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r4) " \
					"processor register r4") },
	{ "r5", (Command)offsetof(Environ, mstate.r[5]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r5) " \
					"processor register r5") },
	{ "r6", (Command)offsetof(Environ, mstate.r[6]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r6) " \
					"processor register r6") },
	{ "r7", (Command)offsetof(Environ, mstate.r[7]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r7) " \
					"processor register r7") },
	{ "r8", (Command)offsetof(Environ, mstate.r[8]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r8) " \
					"processor register r8") },
	{ "r9", (Command)offsetof(Environ, mstate.r[9]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r9) " \
					"processor register r9") },
	{ "r10", (Command)offsetof(Environ, mstate.r[10]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r10) " \
					"processor register r10") },
	{ "r11", (Command)offsetof(Environ, mstate.r[11]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r11) " \
					"processor register r11") },
	{ "r12", (Command)offsetof(Environ, mstate.r[12]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r12) " \
					"processor register r12") },
	{ "r13", (Command)offsetof(Environ, mstate.r[13]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r13) " \
					"processor register r13") },
	{ "r14", (Command)offsetof(Environ, mstate.r[14]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r14) " \
					"processor register r14") },
	{ "r15", (Command)offsetof(Environ, mstate.r[15]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r15) " \
					"processor register r15") },
	{ "up", (Command)offsetof(Environ, mstate.r[9]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r9) " \
					"processor register up") },
	{ "tos", (Command)offsetof(Environ, mstate.r[10]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r10) " \
					"processor register tos") },
	{ "rp", (Command)offsetof(Environ, mstate.r[11]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r11) " \
					"processor register rp") },
	{ "ip", (Command)offsetof(Environ, mstate.r[12]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r12) " \
					"processor register ip") },
	{ "sp", (Command)offsetof(Environ, mstate.r[13]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r13) " \
					"processor register sp") },
	{ "lr", (Command)offsetof(Environ, mstate.r[14]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r14) " \
					"processor register lr") },
	{ "pc", (Command)offsetof(Environ, mstate.r[15]),
			INVALID_FCODE, F_NONE, T_ADDRVAL
			HELP("(-- r15) " \
					"processor register pc") },
	{ NULL,        NULL },
};
