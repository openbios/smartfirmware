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

/* (c) Copyright 1996,1998-1999 by CodeGen, Inc.  All Rights Reserved. */

#include "defs.h"

#ifdef SF_64BIT

/* #370: 64-bit extensions available only if compiler supports them */

#ifndef __LONGLONG
#  error __LONGLONG must be defined and type Long must be a 64-bit integer.
#endif

/* from this point, Cell and Long are assumed to be 64-bit integers */


C(f_bxjoin)		/* bxjoin (b.lo b.2 b.3 b.4 b.5 b.6 b.7 b.hi -- oct) 0x241 */
{
	Cell t, v;
	
	IFCKSP(e, 4, 1);
	POP(e, t);
	v = (t & BYTE_MASK) << (BYTE_SIZE * 7);	
	POP(e, t);
	v |= (t & BYTE_MASK) << (BYTE_SIZE * 6);	
	POP(e, t);
	v |= (t & BYTE_MASK) << (BYTE_SIZE * 5);	
	POP(e, t);
	v |= (t & BYTE_MASK) << (BYTE_SIZE * 4);	
	POP(e, t);
	v |= (t & BYTE_MASK) << (BYTE_SIZE * 3);	
	POP(e, t);
	v |= (t & BYTE_MASK) << (BYTE_SIZE * 2);
	POP(e, t);
	v |= (t & BYTE_MASK) << BYTE_SIZE;
	POP(e, t);
	v |= (t & BYTE_MASK);
	PUSH(e, v);
	return NO_ERROR;
}

EC(f_getquad);		/* <l@ (qaddr -- n) 0x242 */

C(f_lxjoin)			/* lxjoin (q.lo q.hi -- oct) 0x243 */
{
	Cell t, v;
	
	IFCKSP(e, 2, 1);
	POP(e, t);
	v = (t & QUADLET_MASK) << QUADLET_SIZE;
	POP(e, t);
	v |= (t & QUADLET_MASK);
	PUSH(e, v);
	return NO_ERROR;
}


/* The 64-bit spec goes through some contortions for rx@ and rx! but they
   seem to be unnecessary given our current implementation of do_eval().
 */

C(f_rxget)			/* rx@ (oaddr -- o) 0x22E */
{
	Cell addr;
	Cell value = 0;
	
	IFCKSP(e, 1, 1);
	POP(e, addr);
	machine_reg_read(e, addr, &value, sizeof (Octlet));
	PUSH(e, value);
	return NO_ERROR;
}

C(f_rxset)			/* rx! (o oaddr --) 0x22F */
{
	Cell addr;
	Cell value;
	
	IFCKSP(e, 2, 0);
	POP(e, addr);
	POP(e, value);
	machine_reg_write(e, addr, value, sizeof (Octlet));
	return NO_ERROR;
}

C(f_wxjoin)			/* wxjoin (w.lo w.2 w.3 w.hi -- o) 0x244 */
{
	Cell t, v;
	
	IFCKSP(e, 4, 1);
	POP(e, t);
	v = (t & DOUBLET_MASK) << (DOUBLET_SIZE * 3);
	POP(e, t);
	v |= (t & DOUBLET_MASK) << (DOUBLET_SIZE * 2);
	POP(e, t);
	v |= (t & DOUBLET_MASK) << DOUBLET_SIZE;
	POP(e, t);
	v |= (t & DOUBLET_MASK);
	PUSH(e, v);
	return NO_ERROR;
}

C(f_xcomma)			/* x, (o --) 0x245 */
{
	Cell w;
	
	IFCKSP(e, 1, 0);
	POP(e, w);
	compile_num32(e, (w >> QUADLET_SIZE) & QUADLET_MASK);
	return compile_num32(e, w & QUADLET_MASK);
}

CC(f_getoct)			/* x@ (oaddr -- o) 0x246 */
{
	Cell t;
	
	IFCKSP(e, 1, 0);
	t = TOP(e);
	e->sp[0] = *(Octlet*)(uPtr)t;
	return NO_ERROR;
}

CC(f_setoct)		/* x! (o oaddr --) 0x247 */
{
	Octlet *t;
	
	IFCKSP(e, 2, 0);
	POPT(e, t, Octlet*);
	POP(e, *t);
	return NO_ERROR;
}

/* /x (-- n)  0x248 T_CONST */

C(f_multoct)		/* /x* (nu1 -- nu2) 0x249 */
{
	IFCKSP(e, 1, 0);
	e->sp[0] *= sizeof (Octlet);
	return NO_ERROR;
}

C(f_iplusoct)		/* xa+ (addr1 index -- addr2) 0x24A */
{
	Cell i;
	
	IFCKSP(e, 2, 0);
	POP(e, i);
	e->sp[0] += i * sizeof (Octlet);
	return NO_ERROR;
}

C(f_plusoct)		/* xa1+ (addr1 -- addr2) 0x24B */
{
	IFCKSP(e, 1, 0);
	e->sp[0] += sizeof (Octlet);
	return NO_ERROR;
}

C(f_xbflip)			/* xbflip (o1 -- o2) 0x24C */
{
	uOctlet op;
	
	IFCKSP(e, 1, 0);
	op = (uOctlet)TOP(e);
	e->sp[0] = ((op >> (BYTE_SIZE * 7)) & BYTE_MASK) |
			((op >> (BYTE_SIZE * 5)) & (BYTE_MASK << BYTE_SIZE)) |
			((op >> (BYTE_SIZE * 3)) & (BYTE_MASK << (BYTE_SIZE * 2))) |
			((op >> BYTE_SIZE) & (BYTE_MASK << (BYTE_SIZE * 3))) |
			((op & ((uOctlet)BYTE_MASK << (BYTE_SIZE * 3))) << BYTE_SIZE) |
			((op & ((uOctlet)BYTE_MASK << (BYTE_SIZE * 2))) << BYTE_SIZE * 3) |
			((op & ((uOctlet)BYTE_MASK << BYTE_SIZE)) << BYTE_SIZE * 5) |
			((op & BYTE_MASK) << (BYTE_SIZE * 7));
	return NO_ERROR;
}

C(f_xbflips)		/* xbflips (oaddr len --) 0x24D */
{
	Int i;
	uOctlet *op;
	
	IFCKSP(e, 2, 0);
	POP(e, i);
	i /= sizeof (Octlet);
	POPT(e, op, uOctlet*);
	
	for (; i--; op++)
		*op = ((*op >> (BYTE_SIZE * 7)) & BYTE_MASK) |
			((*op >> (BYTE_SIZE * 5)) & (BYTE_MASK << BYTE_SIZE)) |
			((*op >> (BYTE_SIZE * 3)) & (BYTE_MASK << (BYTE_SIZE * 2))) |
			((*op >> BYTE_SIZE) & (BYTE_MASK << (BYTE_SIZE * 3))) |
			((*op & ((uOctlet)BYTE_MASK << (BYTE_SIZE * 3))) << BYTE_SIZE) |
			((*op & ((uOctlet)BYTE_MASK << (BYTE_SIZE * 2))) << BYTE_SIZE * 3) |
			((*op & ((uOctlet)BYTE_MASK << BYTE_SIZE)) << BYTE_SIZE * 5) |
			((*op & BYTE_MASK) << (BYTE_SIZE * 7));
	
	return NO_ERROR;
}

C(f_xbsplit)		/* xbsplit (o -- b.lo b.2 b.3 b.4 b.5 b.6 b.7 b.hi) 0x24E */
{
	Cell t;
	
	IFCKSP(e, 1, 4);
	POP(e, t);
	PUSH(e, ((uOctlet)t) & BYTE_MASK);
	PUSH(e, (((uOctlet)t) >> BYTE_SIZE) & BYTE_MASK);
	PUSH(e, (((uOctlet)t) >> (BYTE_SIZE * 2)) & BYTE_MASK);
	PUSH(e, (((uOctlet)t) >> (BYTE_SIZE * 3)) & BYTE_MASK);
	PUSH(e, (((uOctlet)t) >> (BYTE_SIZE * 4)) & BYTE_MASK);
	PUSH(e, (((uOctlet)t) >> (BYTE_SIZE * 5)) & BYTE_MASK);
	PUSH(e, (((uOctlet)t) >> (BYTE_SIZE * 6)) & BYTE_MASK);
	PUSH(e, (((uOctlet)t) >> (BYTE_SIZE * 7)) & BYTE_MASK);
	return NO_ERROR;
}

C(f_xlflip)			/* xlflip (o1 -- o2) 0x24F */
{
	uOctlet o;
	
	IFCKSP(e, 1, 1);
	POP(e, o);
	o = ((o & QUADLET_MASK) << QUADLET_SIZE) |
			((o >> QUADLET_SIZE) & QUADLET_MASK);
	PUSH(e, o);
	return NO_ERROR;
}

C(f_xlflips)		/* xlflips (oaddr len --) 0x250 */
{
	Int i;
	uOctlet *op;
	
	IFCKSP(e, 2, 0);
	POP(e, i);
	i /= sizeof (Octlet);
	POPT(e, op, uOctlet*);
	
	for (; i--; op++)
		*op = ((*op & QUADLET_MASK) << QUADLET_SIZE) |
				((*op >> QUADLET_SIZE) & QUADLET_MASK);
	
	return NO_ERROR;
}

C(f_xlsplit)		/* xlsplit (o -- quad.lo quad.hi) 0x251 */
{
	uOctlet o;
	
	IFCKSP(e, 1, 2);
	POP(e, o);
	PUSH(e, o & QUADLET_MASK);
	PUSH(e, (o >> QUADLET_SIZE) & QUADLET_MASK);
	return NO_ERROR;
}

C(f_xwflip)			/* xwflip (o1 -- o2) 0x252 */
{
	uOctlet op;
	
	IFCKSP(e, 1, 0);
	op = (uOctlet)TOP(e);
	e->sp[0] = ((op >> (DOUBLET_SIZE * 3)) & DOUBLET_MASK) |
			((op >> DOUBLET_SIZE) & (DOUBLET_MASK << DOUBLET_SIZE)) |
			((op << DOUBLET_SIZE) & 
					((uOctlet)DOUBLET_MASK << (DOUBLET_SIZE * 2))) |
			((op & DOUBLET_MASK) << (DOUBLET_SIZE * 3));
	return NO_ERROR;
}

C(f_xwflips)		/* xwflips (oaddr len --) 0x253 */
{
	Int i;
	uOctlet *op;
	
	IFCKSP(e, 2, 0);
	POP(e, i);
	i /= sizeof (Octlet);
	POPT(e, op, uOctlet*);
	
	for (; i--; op++)
		*op = ((*op >> (DOUBLET_SIZE * 3)) & DOUBLET_MASK) |
				((*op >> DOUBLET_SIZE) & (DOUBLET_MASK << DOUBLET_SIZE)) |
				((*op << DOUBLET_SIZE) &
						((uOctlet)DOUBLET_MASK << (DOUBLET_SIZE * 2))) |
				((*op & DOUBLET_MASK) << (DOUBLET_SIZE * 3));
	
	return NO_ERROR;
}

C(f_xwsplit)		/* xwsplit (o -- w.lo w.2 w.3 w.hi) 0x254 */
{
	uOctlet o;
	
	IFCKSP(e, 1, 4);
	POP(e, o);
	PUSH(e, o & DOUBLET_MASK);
	PUSH(e, (o >> DOUBLET_SIZE) & DOUBLET_MASK);
	PUSH(e, (o >> (DOUBLET_SIZE * 2)) & DOUBLET_MASK);
	PUSH(e, (o >> (DOUBLET_SIZE * 3)) & DOUBLET_MASK);
	return NO_ERROR;
}


const Initentry init_funcs64[] =
{
	{ "bxjoin", f_bxjoin, 0x241, F_NONE, T_FUNC
			HELP("(b.lo b.2 b.3 b.4 b.5 b.6 b.7 b.hi -- oct)\n" \
					"\tjoin eight bytes to form an octlet") },
	{ "<l@", f_getquad, 0x242, F_NONE, T_FUNC
			HELP("(qaddr -- n) fetch sign-extended quadlet from qaddr") },
	{ "lxjoin", f_lxjoin, 0x243, F_NONE, T_FUNC
			HELP("(q.lo q.hi -- oct) join two quadlets to form an octlet") },
	{ "rx@", f_rxget, 0x22E, F_NONE, T_FUNC
			HELP("(oaddr -- o) fetch octlet from device register at oaddr") },
	{ "rx!", f_rxset, 0x22F, F_NONE, T_FUNC
			HELP("(o oaddr --) store octlet to device register at oaddr") },
	{ "wxjoin", f_wxjoin, 0x244, F_NONE, T_FUNC
		HELP("(w.lo w.2 w.3 w.hi -- o) join four doublets to form an octlet") },
	{ "x,", f_xcomma, 0x245, F_NONE, T_FUNC
		HELP("(o --) compile an octlet into the dictionary, doublet aligned") },
	{ "x@", f_getoct, 0x246, F_NONE, T_FUNC
			HELP("(oaddr -- o) fetch octlet from octlet aligned oaddr") },
	{ "x!", f_setoct, 0x247, F_NONE, T_FUNC
			HELP("(o oaddr --) store octlet to octlet aligned oaddr") },
	{ "/x", (Command)sizeof (Octlet), 0x248, F_NONE, T_CONST
			HELP("(-- n) number of address units in an octlet") },
	{ "/x*", f_multoct, 0x249, F_NONE, T_FUNC
			HELP("(nu1 -- nu2) multiply nu1 by /x") },
	{ "xa+", f_iplusoct, 0x24A, F_NONE, T_FUNC
			HELP("(addr1 index -- addr2) increment addr1 by index times /x") },
	{ "xa1+", f_plusoct, 0x24B, F_NONE, T_FUNC
			HELP("(addr1 -- addr2) increment addr1 by /x") },
	{ "xbflip", f_xbflip, 0x24C, F_NONE, T_FUNC
			HELP("(o1 -- o2) reverse bytes in octlet") },
	{ "xbflips", f_xbflips, 0x24D, F_NONE, T_FUNC
			HELP("(oaddr len --) reverse bytes in each octlet in region") },
	{ "xbsplit", f_xbsplit, 0x24E, F_NONE, T_FUNC
			HELP("(o -- b.lo b.2 b.3 b.4 b.5 b.6 b.7 b.hi)\n" \
					"\tsplit an octlet into eight bytes") },
	{ "xlflip", f_xlflip, 0x24F, F_NONE, T_FUNC
			HELP("(o1 -- o2) reverse the quadlets within an octlet") },
	{ "xlflips", f_xlflips, 0x250, F_NONE, T_FUNC
			HELP("(oaddr len --) reverse quadlets within " \
					"each octlet in the region") },
	{ "xlsplit", f_xlsplit, 0x251, F_NONE, T_FUNC
			HELP("(o -- quad.lo quad.hi) split an octlet into two quadlets") },
	{ "xwflip", f_xwflip, 0x252, F_NONE, T_FUNC
			HELP("(o1 -- o2) reverse doublets within an octlet") },
	{ "xwflips", f_xwflips, 0x253, F_NONE, T_FUNC
		HELP("(oaddr len --) reverse doublets within all octlets in region") },
	{ "xwsplit", f_xwsplit, 0x254, F_NONE, T_FUNC
		HELP("(o -- w.lo w.2 w.3 w.hi) split an octlet into four doublets") },

	{ NULL, NULL }
};

#else /* !SF_64BIT */

const Initentry init_funcs64[] =
{
	{ NULL, NULL }
};

#endif /* !SF_64BIT */
