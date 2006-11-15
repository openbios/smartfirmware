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

/* (c) Copyright 1996-2003 by CodeGen, Inc.  All Rights Reserved. */

#include "defs.h"


/* 5.3.2.1 Standard Forth FCode functions */

C(f_dup)			/* dup (x -- x x) 0x47 */
{
	Cell t;
	
	IFCKSP(e, 1, 1);
	t = TOP(e);
	PUSH(e, t);
	return NO_ERROR;
}

CC(f_2dup)			/* 2dup (x1 x2 -- x1 x2 x1 x2) 0x53 */
{
	Cell t, t2;
	
	IFCKSP(e, 2, 2);
	t = TOP(e);
	t2 = STACK(e, 1);
	PUSH(e, t2);
	PUSH(e, t);
	return NO_ERROR;
}

C(f_qdup)			/* ?dup (x -- 0|x x) 0x50 */
{
	IFCKSP(e, 1, 0);

	if (TOP(e) != 0)
		return f_dup(e);
	
	return NO_ERROR;
}

C(f_over)			/* over (x1 x2 -- x1 x2 x1) 0x48 */
{
	Cell t;
	
	IFCKSP(e, 2, 1);
	t = STACK(e, 1);
	PUSH(e, t);
	return NO_ERROR;
}

C(f_2over)			/* 2over (x1 x2 x3 x4 -- x1 x2 x3 x4 x1 x2) 0x54 */
{
	Cell t, t2;
	
	IFCKSP(e, 4, 2);
	t = STACK(e, 2);
	t2 = STACK(e, 3);
	PUSH(e, t2);
	PUSH(e, t);
	return NO_ERROR;
}

C(f_pick)			/* pick (xu..x1 x0 u -- xu..x1 x0 xu) 0x4E */
{
	Cell t, t2;
	
	IFCKSP(e, 1, 0);
	POP(e, t);
	IFCKSP(e, t + 1, 1);
	t2 = STACK(e, t);
	PUSH(e, t2);
	return NO_ERROR;
}

C(f_tuck)			/* tuck (x1 x2 -- x2 x1 x2) 0x4C */
{
	Cell t, t2;
	
	IFCKSP(e, 2, 3);
	POP(e, t);
	POP(e, t2);
	PUSH(e, t);
	PUSH(e, t2);
	PUSH(e, t);
	return NO_ERROR;
}

C(f_drop)			/* drop (x --) 0x46 */
{
	IFCKSP(e, 1, 0);
	DROP(e);
	return NO_ERROR;
}

C(f_2drop)			/* 2drop (x1 x2 --) 0x52 */
{
	IFCKSP(e, 2, 0);
	DROPN(e, 2);
	return NO_ERROR;
}

C(f_nip)			/* nip (x1 x2 -- x2) 0x4D */
{
	Cell t;
	
	IFCKSP(e, 2, 1);
	POP(e, t);
	DROP(e);
	PUSH(e, t);
	return NO_ERROR;
}

C(f_roll)			/* roll (xu...x1 x0 u -- xu-1..x1 x0 xu) 0x4F */
{
	Int i;
	Cell t;
	
	IFCKSP(e, 1, 0);
	POP(e, i);
	IFCKSP(e, i, 0);
	t = STACK(e, i);
	memmove(e->sp - i, e->sp - i + 1, i * sizeof *e->sp);
	*e->sp = t;
	return NO_ERROR;
}

C(f_rot)			/* rot (x1 x2 x3 -- x2 x3 x1) 0x4A */
{
	Cell t;
	
	IFCKSP(e, 3, 0);
	t = e->sp[0];
	e->sp[0] = e->sp[-2];
	e->sp[-2] = e->sp[-1];
	e->sp[-1] = t;
	return NO_ERROR;
}

C(f_mrot)			/* -rot (x1 x2 x3 -- x3 x1 x2) 0x4B */
{
	Cell t;
	
	IFCKSP(e, 3, 0);
	t = e->sp[0];
	e->sp[0] = e->sp[-1];
	e->sp[-1] = e->sp[-2];
	e->sp[-2] = t;
	return NO_ERROR;
}

C(f_2rot)			/* 2rot (x1 x2 x3 x4 x5 x6 -- x3 x4 x5 x6 x1 x2) 0x56 */
{
	Cell t;
	
	IFCKSP(e, 6, 0);
	t = e->sp[0];
	e->sp[0] = e->sp[-4];
	e->sp[-4] = e->sp[-2];
	e->sp[-2] = t;
	t = e->sp[-1];
	e->sp[-1] = e->sp[-5];
	e->sp[-5] = e->sp[-3];
	e->sp[-3] = t;
	return NO_ERROR;
}


C(f_swap)			/* swap (x1 x2 -- x2 x1) 0x49 */
{
	Cell t;
	
	IFCKSP(e, 2, 0);
	t = e->sp[0];
	e->sp[0] = e->sp[-1];
	e->sp[-1] = t;
	return NO_ERROR;
}

C(f_2swap)			/* 2swap (x1 x2 x3 x4 -- x3 x4 x1 x2) 0x55 */
{
	Cell t;
	
	IFCKSP(e, 4, 0);
	t = e->sp[0];
	e->sp[0] = e->sp[-2];
	e->sp[-2] = t;
	t = e->sp[-1];
	e->sp[-1] = e->sp[-3];
	e->sp[-3] = t;
	return NO_ERROR;
}

C(f_push_rstk)		/* >r (x --) (R: -- x) 0x30 */
{
	Cell t;
	
	IFCKSP(e, 1, 0);
	IFCKRETSP(e, 0, 1);
	POP(e, t);			/* get value to save */
	RPUSH(e, t);		/* push value to save */
	return NO_ERROR; 
}

C(f_pop_rstk)		/* r> (-- x) (R: x --) 0x31 */
{
	Cell t;
	
	IFCKRETSP(e, 1, 0);
	IFCKSP(e, 0, 1);
	RPOP(e, t);			/* get our saved value */
	PUSH(e, t);			/* and put the saved value on the stack */
	return NO_ERROR; 
}

C(f_top_rstk)		/* r@ (-- x) (R: x -- x) 0x32 */
{
	Cell t;
	
	IFCKRETSP(e, 1, 0);
	IFCKSP(e, 0, 1);
	t = RTOP(e);
	PUSH(e, t);
	return NO_ERROR; 
}

C(f_depth)			/* depth (-- n) 0x51 */
{
	Cell t;
	
	IFCKSP(e, 0, 1);
	t = e->sp - e->stack;
	PUSH(e, t);
	return NO_ERROR;
}

C(f_plus)			/* + (n1 n2 -- sum) 0x1E */
{
	IFCKSP(e, 2, 0);
	e->sp[-1] += e->sp[0];
	DROP(e);
	return NO_ERROR;
}

C(f_minus)			/* - (n1 n2 -- diff) 0x1F */
{
	IFCKSP(e, 2, 0);
	e->sp[-1] -= e->sp[0];
	DROP(e);
	return NO_ERROR;
}

C(f_times)			/* * (n1 n2 -- prod) 0x20 */
{
	IFCKSP(e, 2, 0);
	e->sp[-1] *= e->sp[0];
	DROP(e);
	return NO_ERROR;
}

static Retcode
divmod_cell(Cell num, Cell den, Cell *div, Cell *mod)
{
	Cell d;
	Cell m;
	Bool negd = FALSE;
	Bool negm = FALSE;

	if (den == 0)
		return E_DIVIDE_BY_ZERO;

	if (num < 0)
	{
		num = -num;
		negd = TRUE;
	}

	if (den < 0)
	{
		den = -den;
		negd = !negd;
		negm = TRUE;
	}

	d = num / den;
	m = num % den;

	if (negd)
	{
		if (m)
		{
			d++;
			m = den - m;
		}

		d = -d;
	}

	if (negm)
		m = -m;

	if (div)
		*div = d;

	if (mod)
		*mod = m;

	return NO_ERROR;
}

C(f_div)			/* / (n1 n2 -- quot) 0x21 */
{
	int ret;
	IFCKSP(e, 2, 0);
	ret = divmod_cell(e->sp[-1], e->sp[0], &e->sp[-1], NULL);
	DROP(e);
	return ret;
}

C(f_mod)			/* mod (n1 n2 -- rem) 0x22 */
{
	int ret;
	IFCKSP(e, 2, 0);
	ret = divmod_cell(e->sp[-1], e->sp[0], NULL, &e->sp[-1]);
	DROP(e);
	return ret;
}

C(f_divmod)			/* /mod (n1 n2 -- rem quot) 0x2A */
{
	IFCKSP(e, 2, 0);
	return divmod_cell(e->sp[-1], e->sp[0], &e->sp[0], &e->sp[-1]);
}

C(f_udivmod)		/* u/mod (u1 u2 -- urem uquot) 0x2B */
{
	Cell t, t2;

	IFCKSP(e, 2, 0);
	t = e->sp[-1];
	t2 = e->sp[0];

	if (t2 == 0)
		return E_DIVIDE_BY_ZERO;

	e->sp[-1] = (uCell)t % (uCell)t2;
	e->sp[0] = (uCell)t / (uCell)t2;
	return NO_ERROR;
}

C(f_abs)			/* abs (n -- u) 0x2D */
{
	IFCKSP(e, 1, 0);
	DO_MASK32(e, e->sp[0]);

	if (e->sp[0] < 0)
		e->sp[0] = -e->sp[0];
	
	return NO_ERROR;
}

C(f_negate)			/* negate (n1 -- n2) 0x2C */
{
	IFCKSP(e, 1, 0);
	*e->sp = -*e->sp;
	return NO_ERROR;
}

C(f_max)			/* max (n1 n2 -- n1|n2) 0x2F */
{
	Cell n1 = e->sp[0];
	Cell n2 = e->sp[-1];

	IFCKSP(e, 2, 0);
	DO_MASK32(e, n1);
	DO_MASK32(e, n2);

	if (n1 > n2)
		e->sp[-1] = e->sp[0];
	
	DROP(e);
	return NO_ERROR;
}

C(f_min)			/* min (n1 n2 -- n1|n2) 0x2E */
{
	Cell n1 = e->sp[0];
	Cell n2 = e->sp[-1];

	IFCKSP(e, 2, 0);
	DO_MASK32(e, n1);
	DO_MASK32(e, n2);

	if (n1 < n2)
		e->sp[-1] = e->sp[0];
	
	DROP(e);
	return NO_ERROR;
}

C(f_bounds)			/* bounds (n cnt -- n+cnt n) 0xAC */
{
	Cell t;
	
	IFCKSP(e, 2, 0);
	t = e->sp[-1];
	e->sp[-1] += e->sp[0];
	e->sp[0] = t;
	return NO_ERROR;
}

C(f_lshift)			/* lshift (x1 u -- x2) 0x27 */
{
	IFCKSP(e, 2, 0);
	e->sp[-1] <<= (uInt)e->sp[0];
	DROP(e);
	return NO_ERROR;
}

C(f_rshift)			/* rshift (x1 u -- x2) 0x28 */
{
	IFCKSP(e, 2, 0);
	e->sp[-1] = (uCell)e->sp[-1] >> (uInt)e->sp[0];
	DROP(e);
	DO_MASK32(e, e->sp[0]);
	return NO_ERROR;
}
	
C(f_times2)			/* 2* (x1 -- x2) 0x59 */
{
	IFCKSP(e, 1, 0);
	e->sp[0] <<= 1;
	return NO_ERROR;
}

C(f_div2)			/* 2/ (x1 -- x2) 0x57 */
{
	IFCKSP(e, 1, 0);
	e->sp[0] >>= 1;
	return NO_ERROR;
}

C(f_and)			/* and (x1 x2 -- x3) 0x23 */
{
	IFCKSP(e, 2, 0);
	e->sp[-1] &= e->sp[0];
	DROP(e);
	return NO_ERROR;
}

C(f_or)				/* or (x1 x2 -- x3) 0x24 */
{
	IFCKSP(e, 2, 0);
	e->sp[-1] |= e->sp[0];
	DROP(e);
	return NO_ERROR;
}

C(f_xor)			/* xor (x1 x2 -- x3) 0x25 */
{
	IFCKSP(e, 2, 0);
	e->sp[-1] ^= e->sp[0];
	DROP(e);
	return NO_ERROR;
}

C(f_invert)			/* invert (x1 -- x2) 0x26 */
{
	IFCKSP(e, 1, 0);
	e->sp[0] = ~e->sp[0];
	return NO_ERROR;
}

static void
dplus(uCell *nh, uCell *nl, uCell uh, uCell ul)
{
	*nl += ul;
	*nh += uh;

	if (*nl < ul)
		*nh += 1;
}

static void
dminus(uCell *nh, uCell *nl, uCell uh, uCell ul)
{
	if (*nl < ul)
		*nh -= 1;

	*nl -= ul;
	*nh -= uh;
}

CC(f_dplus)			/* d+ (d1 d2 -- d.sum) 0xD8 */
{
	IFCKSP(e, 4, 0);
	dplus((uCell*)e->sp - 2, (uCell*)e->sp - 3, e->sp[0], e->sp[-1]);
	DROPN(e, 2);
	return NO_ERROR;
}

CC(f_dminus)		/* d- (d1 d2 -- d.diff) 0xD9 */
{
	IFCKSP(e, 4, 0);
	dminus((uCell*)e->sp - 2, (uCell*)e->sp - 3, e->sp[0], e->sp[-1]);
	DROPN(e, 2);
	return NO_ERROR;
}

#define CELL_SIZE (BYTE_SIZE * sizeof (Cell))

static void
dlshift(uCell *dh, uCell *dl)
{
	*dh <<= 1;

	if ((Cell)*dl < 0)
		*dh |= 1;

	*dl <<= 1;
}

static void
drshift(uCell *dh, uCell *dl)
{
	*dl >>= 1;

	if (*dh & 1)
		*dl |= ((uCell)1 << (CELL_SIZE - 1));

	*dh >>= 1;
}

CC(f_umtimes)		/* um* (u1 u2 -- ud.prod) 0xD4 */
{
	uCell u, ul, uh;

	IFCKSP(e, 2, 4);
	POP(e, u);
	POP(e, ul);		/* uh..ul make a double-num */
	uh = 0;

	/* running product calculated on top of stack */
	PUSH(e, 0);
	PUSH(e, 0);

	while (u)
	{
		if (u & 1)
			dplus((uCell*)e->sp, (uCell*)e->sp - 1, uh, ul);

		/* left-shift by one - double the number */
		dlshift(&uh, &ul);
		u >>= 1;		/* right-shift - halve the other number */
	}

	/* top should have double-number mult result */
	return NO_ERROR;
}

CC(f_umddivmod)		/* umd/mod (ud ud2 -- ud.rem ud.quot) [non-standard] */
{
	uCell uh, ul, qh, ql;
	int i;

	IFCKSP(e, 4, 4);
	POP(e, uh);
	POP(e, ul);

	/* leave double-num on stack to calculate the remainder */

	if (ul == 0 && uh == 0)
		return E_DIVIDE_BY_ZERO;

	qh = ql = 0;
	DPRINTF(("umd/mod: uh..ul=%#Cx.%Cx sp[0..1]=%#Cx.%Cx\n",
			uh, ul, e->sp[0], e->sp[-1]));

	for (i = 0; i < CELL_SIZE * 2; i++)
		if ((Cell)uh < 0 ||
				(uh > TOP(e) || (uh == TOP(e) && ul > STACK(e, 1))))
			break;
		else
			dlshift(&uh, &ul);

	DPRINTF(("umd/mod: i=%d ul..uh=%#Cx.%Cx\n", i, ul, uh));

	for (; i >= 0; i--)
	{
		dlshift(&qh, &ql);

		if (TOP(e) > uh || (TOP(e) == uh && STACK(e, 1) >= ul))
		{
			ql |= 1;
			dminus((uCell*)e->sp, (uCell*)e->sp - 1, uh, ul);
		}

		drshift(&uh, &ul);
	}

	DPRINTF(("umd/mod: sp[0..1]=%#Cx.%Cx ql..qh=%#Cx.%Cx\n",
			e->sp[0], e->sp[-1], ql, qh));
	PUSH(e, ql);
	PUSH(e, qh);
	return NO_ERROR;
}

CC(f_umdivmod)		/* um/mod (ud u -- urem uquot) 0xD5 */
{
	Retcode ret;

	IFCKSP(e, 0, 1);
	PUSH(e, 0);
	ret = f_umddivmod(e);

	if (ret != NO_ERROR)
		return ret;

	if (e->sp[-2] || e->sp[0])
		return E_RESULT_OUT_OF_RANGE;

	/* ignore high-words of both remainder and quotient */
	e->sp[-2] = e->sp[-1];
	DROPN(e, 2);
	return ret;
}

C(f_charsize)		/* char+ (addr1 -- addr2) 0x62 */
{
	IFCKSP(e, 1, 0);
	e->sp[0] += sizeof (char);
	return NO_ERROR;
}

C(f_cellsize)		/* cell+ (addr1 -- addr2) 0x65 */
{
	IFCKSP(e, 1, 0);
	e->sp[0] += (e->fcode32 ? sizeof (Quadlet) : sizeof (Cell));
	return NO_ERROR;
}

C(f_chartimes)		/* chars (nu1 -- nu2) 0x66 */
{
	IFCKSP(e, 1, 0);
	e->sp[0] *= sizeof (char);
	return NO_ERROR;
}

C(f_celltimes)		/* cells (nu1 -- nu2) 0x69 */
{
	IFCKSP(e, 1, 0);
	e->sp[0] *= (e->fcode32 ? sizeof (Quadlet) : sizeof (Cell));
	return NO_ERROR;
}

C(f_aligned)		/* aligned (n1 -- n1 | a-addr) 0xAE */
{
	IFCKSP(e, 1, 0);

	if (e->sp[0] % ALIGNMENT)
		e->sp[0] += ALIGNMENT - (e->sp[0] % ALIGNMENT);
	
	return NO_ERROR;
}

CC(f_get)			/* @ (a-addr -- x) 0x6D */
{
	IFCKSP(e, 1, 0);
	DO_UMASK32(e, e->sp[0]);

	if (e->fcode32)
		#ifdef SUN_COMPATIBILITY
			e->sp[0] = *(uQuadlet*)(uPtr)e->sp[0];
		#else
			e->sp[0] = *(Quadlet*)(uPtr)e->sp[0];
		#endif
	else
		e->sp[0] = *(Cell*)(uPtr)e->sp[0];

	return NO_ERROR;
}

C(f_2get)			/* 2@ (a-addr -- x1 x2) 0x76 */
{
	Cell t;
	
	IFCKSP(e, 1, 1);
	t = e->sp[0];

	if (e->fcode32)
	{
		t &= QUADLET_MASK;

		#ifdef SUN_COMPATIBILITY
			e->sp[0] = *(uQuadlet*)(uPtr)(t + sizeof (uQuadlet));
			PUSH(e, *(uQuadlet*)(uPtr)t);
		#else
			e->sp[0] = *(Quadlet*)(uPtr)(t + sizeof (uQuadlet));
			PUSH(e, *(Quadlet*)(uPtr)t);
		#endif
	}
	else
	{
		e->sp[0] = *(Cell*)(uPtr)(t + sizeof (Cell));
		PUSH(e, *(Cell*)(uPtr)t);
	}

	return NO_ERROR;
}

CC(f_charget)		/* c@ (addr -- byte) 0x71 */
{
	IFCKSP(e, 1, 0);
	DO_UMASK32(e, e->sp[0]);
	e->sp[0] = *(uByte*)(uPtr)e->sp[0];
	return NO_ERROR;
}

CC(f_set)			/* ! (x a-addr --) 0x72 */
{
	Cell *t, t2;

	IFCKSP(e, 2, 0);
	DO_UMASK32(e, e->sp[0]);
	POPT(e, t, Cell*);
	POP(e, t2);

	if (e->fcode32)
		*(Quadlet*)(uPtr)t = (Quadlet)t2;
	else
		*t = t2;

	return NO_ERROR;
}

C(f_2set)			/* 2! (x1 x2 a-addr --) 0x77 */
{
	Cell *t, x1, x2;
	
	IFCKSP(e, 3, 0);
	DO_UMASK32(e, e->sp[0]);
	POPT(e, t, Cell*);
	POP(e, x2);
	POP(e, x1);

	if (e->fcode32)
	{
		Quadlet *q = (Quadlet*)(uPtr)t;
		*(q + 1) = (Quadlet)x1;
		*q = (Quadlet)x2;
	}
	else
	{
		*(t + 1) = x1;
		*t = x2;
	}

	return NO_ERROR;
}

C(f_plusset)		/* +! (nu a-addr --) 0x6C */
{
	Cell *t, t2;
	
	IFCKSP(e, 2, 0);
	DO_UMASK32(e, e->sp[0]);
	POPT(e, t, Cell*);
	POP(e, t2);

	if (e->fcode32)
		*(Quadlet*)t += (Quadlet)t2;
	else
		*t += t2;

	return NO_ERROR;
}

C(f_charset)		/* c! (byte addr --) 0x75 */
{
	Byte *t;
	
	IFCKSP(e, 2, 0);
	DO_UMASK32(e, e->sp[0]);
	POPT(e, t, Byte*);
	POP(e, *t);
	return NO_ERROR;
}

C(f_move)			/* move (src-addr dest-addr len --) 0x78 */
{
	Int i;
	Byte *t, *t2;
	
	IFCKSP(e, 3, 0);
	POP(e, i);
	DO_UMASK32(e, e->sp[0]);
	POPT(e, t, Byte*);
	DO_UMASK32(e, e->sp[0]);
	POPT(e, t2, Byte*);

	if (i > 0)
		memmove(t, t2, i);

	return NO_ERROR;
}

CC(f_fill)			/* fill (addr len byte --) 0x79 */
{
	Int len;
	Byte byte, *addr;
	
	IFCKSP(e, 3, 0);
	POPTYPE(e, byte, Byte);
	POP(e, len);
	DO_UMASK32(e, e->sp[0]);
	POPT(e, addr, Byte*);
	DPRINTF(("f_fill: addr=%p len=%#x byte=%#x\n", addr, len, byte));

	if (len > 0)
		memset(addr, byte, len);

	return NO_ERROR;
}

C(f_keyq)			/* key? (-- pressed?) 0x8D */
{
	IFCKSP(e, 0, 1);
	PUSH(e, key_down(e));
	return NO_ERROR;
}

C(f_key)			/* key (-- char) 0x8E */
{
	IFCKSP(e, 0, 1);
	PUSH(e, get_key(e));
	return NO_ERROR;
}

C(f_expect)			/* expect (addr len --) 0x8A */
{
	Byte *addr;
	Int len, elen;
	
	IFCKSP(e, 2, 0);
	POP(e, len);
	POPT(e, addr, Byte*);
	expect_line(e, addr, len, &elen, FALSE);
	e->expect_len = elen;

	/* erase any trailing newline, if any */
	if (e->expect_len > 0 && addr[e->expect_len - 1] == '\n')
		e->expect_len--;

	return NO_ERROR;
}

/* span: T_ADDR to e->expect_len */

/* bl 0xA9: T_CONST for 0x20 */

C(f_emit)			/* emit (char --) 0x8F */
{
	Cell t;
	
	IFCKSP(e, 1, 0);
	POP(e, t);
	display_char(e, t);
	return NO_ERROR;
}

C(f_type)			/* type (text-str text-len --) 0x90 */
{
	Int i;
	Byte *t;
	
	IFCKSP(e, 2, 0);
	POP(e, i);
	POPT(e, t, Byte*);
	display_text(e, t, i);
	return NO_ERROR;
}

C(f_cr)				/* cr (--) 0x92 */
{
	display_text(e, "\r\n", 2);
	return NO_ERROR;
}

C(f_count)			/* count (pstr -- str len) 0x84 */
{
	uByte *str;
	
	IFCKSP(e, 1, 2);
	POPT(e, str, uByte*);	/* counted string */
	PUSHP(e, str + 1);
	PUSH(e, *str);
	return NO_ERROR;
}

/* base (-- a-addr) 0xA0: T_ADDR to e->radix */

C(f_dot)			/* . (nu --) 0x9D */
{
	return display_number(e, e->radix, TRUE, FALSE);
}

C(f_udot)			/* u. (u --) 0x9B */
{
	return display_number(e, e->radix, FALSE, FALSE);
}
			
C(f_dotr)			/* .r (n size --) 0x9E */
{
	return display_number(e, e->radix, TRUE, TRUE);
}

C(f_udotr)			/* u.r (u size --) 0x9C */
{
	return display_number(e, e->radix, FALSE, TRUE);
}

C(f_dots)			/* .s (... -- ...) 0x9F */
{
	display_stack(e);
	return NO_ERROR;
}

C(f_lthash)			/* <# (--) 0x96 */
{
	if (e->numlen != -1)
		return E_PICNUM_FORMAT;
	
	e->numlen = 0;
	e->numptr = e->numbuf + STR_SIZE - 1;
	*e->numptr = '\0';
	return NO_ERROR;
}

C(f_hash)			/* # (ud1 -- ud2) 0xC7 */
{
	Retcode ret;
	uCell s;

	IFCKSP(e, 2, 2);

	if (e->numlen >= STR_SIZE - 1)
		return E_PICNUM_OVERFLOW;

	PUSH(e, e->radix);
	PUSH(e, 0);
	ret = f_umddivmod(e);

	if (ret != NO_ERROR)
		return ret;

	e->numlen++;
	e->numptr--;
	s = e->sp[-3];
	*e->numptr = s + (s < 10 ? '0' : HEX_A - 10);

	e->sp[-3] = e->sp[-1];
	e->sp[-2] = e->sp[0];
	DROPN(e, 2);
	return NO_ERROR;
}

C(f_hashs)			/* #s (ud -- 0 0) 0xC8 */
{
	Retcode ret = NO_ERROR;

	do
		ret = f_hash(e);
	while (ret == NO_ERROR && (TOP(e) || STACK(e, 1)));

	return ret;
}

C(f_hashgt)			/* #> (ud -- str len) 0xC9 */
{
	IFCKSP(e, 2, 2);
	DROPN(e, 2);
	PUSHP(e, e->numptr);
	PUSH(e, e->numlen);
	e->numlen = -1;
	return NO_ERROR;
}


C(f_hold)			/* hold (char --) 0x95 */
{
	IFCKSP(e, 1, 0);

	if (e->numlen >= STR_SIZE - 1)
		return E_PICNUM_OVERFLOW;

	e->numlen++;
	e->numptr--;
	POP(e, *e->numptr);
	return NO_ERROR;
}

C(f_sign)			/* sign (n --) 0x98 */
{
	Cell t;
	
	IFCKSP(e, 1, 0);
	POP(e, t);
	
	if (e->numlen >= STR_SIZE - 1)
		return E_PICNUM_OVERFLOW;
	
	if (t < 0)
	{
		e->numlen++;
		e->numptr--;
		*e->numptr = '-';
	}
	
	return NO_ERROR;
}

C(f_lt)				/* < (n1 n2 -- less?) 0x3A */
{
	Cell t;
	
	IFCKSP(e, 2, 0);
	POP(e, t);
	DO_MASK32(e, t);
	DO_MASK32(e, e->sp[0]);
	e->sp[0] = (e->sp[0] < t) ? FTRUE : FFALSE;
	return NO_ERROR;
}

C(f_noteq)			/* <> (x1 x2 -- not-equal?) 0x3D */
{
	Cell t;
	
	IFCKSP(e, 2, 0);
	POP(e, t);
	DO_MASK32(e, t);
	DO_MASK32(e, e->sp[0]);
	e->sp[0] = (e->sp[0] != t) ? FTRUE : FFALSE;
	return NO_ERROR;
}

C(f_eq)				/* = (x1 x2 -- equal?) 0x3C */
{
	Cell t;
	
	IFCKSP(e, 2, 0);
	POP(e, t);
	DO_MASK32(e, t);
	DO_MASK32(e, e->sp[0]);
	e->sp[0] = (e->sp[0] == t) ? FTRUE : FFALSE;
	return NO_ERROR;
}

C(f_gt)				/* > (n1 n2 -- greater?) 0x3B */
{
	Cell t;
	
	IFCKSP(e, 2, 0);
	POP(e, t);
	DO_MASK32(e, t);
	DO_MASK32(e, e->sp[0]);
	e->sp[0] = (e->sp[0] > t) ? FTRUE : FFALSE;
	return NO_ERROR;
}

C(f_within)			/* within (n min max -- min<=n<max?) 0x45 */
{
	Cell t, t2;
	
	IFCKSP(e, 3, 0);
	POP(e, t);
	POP(e, t2);
	DO_MASK32(e, t);
	DO_MASK32(e, t2);
	DO_MASK32(e, e->sp[0]);
	e->sp[0] = (t2 <= e->sp[0] && e->sp[0] < t) ? FTRUE : FFALSE;
	return NO_ERROR;
}

C(f_lt0)			/* 0< (n -- less-than-0?) 0x36 */
{
	IFCKSP(e, 1, 0);
	DO_MASK32(e, e->sp[0]);
	e->sp[0] = (e->sp[0] < 0) ? FTRUE : FFALSE;
	return NO_ERROR;
}

C(f_neq0)			/* 0<> (n -- not-equal-to-0?) 0x35 */
{
	IFCKSP(e, 1, 0);
	DO_MASK32(e, e->sp[0]);
	e->sp[0] = (e->sp[0] != 0) ? FTRUE : FFALSE;
	return NO_ERROR;
}

C(f_eq0)			/* 0= (n -- equal-to-0?) 0x34 */
{
	IFCKSP(e, 1, 0);
	DO_MASK32(e, e->sp[0]);
	e->sp[0] = (e->sp[0] == 0) ? FTRUE : FFALSE;
	return NO_ERROR;
}

C(f_gt0)			/* 0> (n -- greater-than-0?) 0x38 */
{
	IFCKSP(e, 1, 0);
	DO_MASK32(e, e->sp[0]);
	e->sp[0] = (e->sp[0] > 0) ? FTRUE : FFALSE;
	return NO_ERROR;
}

C(f_ult)			/* u< (u1 u2 -- unsigned-less?) 0x40 */
{
	Cell t;
	
	IFCKSP(e, 2, 0);
	POP(e, t);
	DO_UMASK32(e, t);
	DO_UMASK32(e, e->sp[0]);
	e->sp[0] = ((uCell)e->sp[0] < (uCell)t) ? FTRUE : FFALSE;
	return NO_ERROR;
}

C(f_ugt)			/* u> (u1 u2 -- unsigned-greater?) 0x3E */
{
	Cell t;
	
	IFCKSP(e, 2, 0);
	POP(e, t);
	DO_UMASK32(e, t);
	DO_UMASK32(e, e->sp[0]);
	e->sp[0] = ((uCell)e->sp[0] > (uCell)t) ? FTRUE : FFALSE;
	return NO_ERROR;
}


/* 5.3.2.2 Other simple Forth FCode functions */

/* /c (-- n) 0x5A T_CONST */
/* /w (-- n) 0x5B T_CONST */
/* /l (-- n) 0x5C T_CONST */

C(f_slashn)			/* /n (-- n) 0x5D */
{
	IFCKSP(e, 0, 1);

	if (e->fcode32)
		PUSH(e, sizeof (Quadlet));
	else
		PUSH(e, sizeof (Cell));

	return NO_ERROR;
}

C(f_iplusbyte)		/* ca+ (addr1 index -- addr2) 0x5E */
{
	Cell i;
	
	IFCKSP(e, 2, 0);
	POP(e, i);
	e->sp[0] += i * sizeof (Byte);
	return NO_ERROR;
}

C(f_iplusdoub)		/* wa+ (addr1 index -- addr2) 0x5F */
{
	Cell i;
	
	IFCKSP(e, 2, 0);
	POP(e, i);
	e->sp[0] += i * sizeof (Doublet);
	return NO_ERROR;
}

C(f_iplusquad)		/* la+ (addr1 index -- addr2) 0x60 */
{
	Cell i;
	
	IFCKSP(e, 2, 0);
	POP(e, i);
	e->sp[0] += i * sizeof (Quadlet);
	return NO_ERROR;
}

C(f_ipluscell)		/* na+ (addr1 index -- addr2) 0x61 */
{
	Cell i;
	
	IFCKSP(e, 2, 0);
	POP(e, i);
	e->sp[0] += i * (e->fcode32 ? sizeof (Quadlet) : sizeof (Cell));
	return NO_ERROR;
}

C(f_plusdoub)		/* wa1+ (addr1 -- addr2) 0x63 */
{
	IFCKSP(e, 1, 0);
	e->sp[0] += sizeof (Doublet);
	return NO_ERROR;
}

C(f_plusquad)		/* la1+ (addr1 -- addr2) 0x64 */
{
	IFCKSP(e, 1, 0);
	e->sp[0] += sizeof (Quadlet);
	return NO_ERROR;
}

C(f_multdoub)		/* /w* (nu1 -- nu2) 0x67 */
{
	IFCKSP(e, 1, 0);
	e->sp[0] *= sizeof (Doublet);
	return NO_ERROR;
}

C(f_multquad)		/* /l* (nu1 -- nu2) 0x68 */
{
	IFCKSP(e, 1, 0);
	e->sp[0] *= sizeof (Quadlet);
	return NO_ERROR;
}
C(f_getudoub)		/* w@ (waddr -- w) 0x6F */
{
	Cell t;
	
	IFCKSP(e, 1, 0);
	t = TOP(e);
	DO_UMASK32(e, t);
	e->sp[0] = *(uDoublet*)(uPtr)t;
	return NO_ERROR;
}

CC(f_getdoub)		/* <w@ (waddr -- n) 0x70 */
{
	Cell t;
	
	IFCKSP(e, 1, 0);
	t = TOP(e);
	DO_UMASK32(e, t);
	e->sp[0] = *(Doublet*)(uPtr)t;
	return NO_ERROR;
}

CC(f_getquad)		/* l@ (qaddr -- q) 0x6E */
{
	Cell t;
	
	IFCKSP(e, 1, 0);
	t = TOP(e);
	DO_UMASK32(e, t);
	e->sp[0] = *(uQuadlet*)(uPtr)t;
	return NO_ERROR;
}

CC(f_setdoub)		/* w! (w waddr --) 0x74 */
{
	Doublet *t;
	
	IFCKSP(e, 2, 0);
	DO_UMASK32(e, e->sp[0]);
	POPT(e, t, Doublet*);
	POP(e, *t);
	return NO_ERROR;
}

CC(f_setquad)		/* l! (quad qaddr --) 0x73 */
{
	Quadlet *t;
	
	IFCKSP(e, 2, 0);
	DO_UMASK32(e, e->sp[0]);
	POPT(e, t, Quadlet*);
	POP(e, *t);
	return NO_ERROR;
}

C(f_aligndoub)		/* w, (w --) 0xD1 */
{
	Doublet *mem;

	IFCKSP(e, 1, 0);

	if (((Ptr)e->fbrk % DALIGNMENT) &&
			falloc(e, DALIGNMENT - ((Ptr)e->fbrk % DALIGNMENT)) == NULL)
		return E_OUT_OF_MEMORY;
	
	mem = (Doublet*)falloc(e, sizeof (Doublet));

	if (mem == NULL)
		return E_OUT_OF_MEMORY;

	POP(e, *mem);
	return NO_ERROR;
}

C(f_alignquad)		/* l, (quad --) 0xD2 */
{
	Quadlet *mem;

	IFCKSP(e, 1, 0);

	if (((Ptr)e->fbrk % DALIGNMENT) &&
			falloc(e, DALIGNMENT - ((Ptr)e->fbrk % DALIGNMENT)) == NULL)
		return E_OUT_OF_MEMORY;

	mem = (Quadlet*)falloc(e, sizeof (Quadlet));

	if (mem == NULL)
		return E_OUT_OF_MEMORY;
	
	/* doublet aligned, so a simple quad write may not work */
	machine_unalign_write(e, (Cell)(uPtr)mem, TOP(e), sizeof (Quadlet));
	DROP(e);
	return NO_ERROR;
}

C(f_off)			/* off (a-addr --) 0x6B */
{
	IFCKSP(e, 1, 0);
	DO_UMASK32(e, e->sp[0]);

	if (e->fcode32)
	{
		Quadlet *t;
		POPT(e, t, Quadlet*);
		*t = FFALSE;
	}
	else
	{
		Cell *t;
		POPT(e, t, Cell*);
		*t = FFALSE;
	}

	return NO_ERROR;
}

C(f_on)				/* on (a-addr --) 0x6A */
{
	IFCKSP(e, 1, 0);
	DO_UMASK32(e, e->sp[0]);

	if (e->fcode32)
	{
		Quadlet *t;
		POPT(e, t, Quadlet*);
		*t = FTRUE;
	}
	else
	{
		Cell *t;
		POPT(e, t, Cell*);
		*t = FTRUE;
	}

	return NO_ERROR;
}

C(f_uhash)			/* u# (u1 -- u2) 0x99 */
{
	uCell i;
	Cell s;

	IFCKSP(e, 1, 0);

	if (e->numlen >= STR_SIZE - 1)
		return E_PICNUM_OVERFLOW;

	POP(e, i);
	s = i % e->radix;
	i /= e->radix;
	e->numlen++;
	e->numptr--;
	*e->numptr = s + (s < 10 ? '0' : HEX_A - 10);
	PUSH(e, i);
	return NO_ERROR;
}

C(f_uhashs)			/* u#s (u -- 0) 0x9A */
{
	Retcode ret = NO_ERROR;

	do
		ret = f_uhash(e);
	while (ret == NO_ERROR && TOP(e));

	return ret;
}

C(f_uhashgt)		/* u#> (u -- str len) 0x97 */
{
	IFCKSP(e, 1, 2);
	DROP(e);
	PUSHP(e, e->numptr);
	PUSH(e, e->numlen);
	e->numlen = -1;
	return NO_ERROR;
}

C(f_comp)			/* comp (addr1 addr2 len -- ?diff?) 0x7A */
{
	Int i;
	Byte *t, *t2;

	IFCKSP(e, 3, 1);
	POP(e, i);
	POPT(e, t, Byte*);
	POPT(e, t2, Byte*);
	PUSH(e, memcmp(t2, t, i));
	return NO_ERROR;
}

C(f_lbsplit)		/* lbsplit (quad -- b.lo b2 b3 b4.hi) 0x7E */
{
	Cell t;
	
	IFCKSP(e, 1, 4);
	POP(e, t);
	PUSH(e, t & BYTE_MASK);
	PUSH(e, ((t & QUADLET_MASK) >> BYTE_SIZE) & BYTE_MASK);
	PUSH(e, ((t & QUADLET_MASK) >> (BYTE_SIZE * 2)) & BYTE_MASK);
	PUSH(e, ((t & QUADLET_MASK) >> (BYTE_SIZE * 3)) & BYTE_MASK);
	return NO_ERROR;
}

C(f_lwsplit)		/* lwsplit (quad -- w1.lo w2.hi) 0x7C */
{
	Cell t;
	
	IFCKSP(e, 1, 2);
	POP(e, t);
	PUSH(e, t & DOUBLET_MASK);
	PUSH(e, ((t & QUADLET_MASK) >> DOUBLET_SIZE) & DOUBLET_MASK);
	return NO_ERROR;
}

C(f_wbsplit)		/* wbsplit (w -- b1.lo b2.hi) 0xAF */
{
	Cell t;
	
	IFCKSP(e, 1, 2);
	POP(e, t);
	PUSH(e, t & BYTE_MASK);
	PUSH(e, ((t & DOUBLET_MASK) >> BYTE_SIZE) & BYTE_MASK);
	return NO_ERROR;
}

C(f_bljoin)			/* bljoin (b1.lo b2 b3 b4.hi -- quad) 0x7F */
{
	Cell t, v;
	
	IFCKSP(e, 4, 1);
	POP(e, t);
	v = (t & BYTE_MASK) << (BYTE_SIZE * 3);	
	POP(e, t);
	v |= (t & BYTE_MASK) << (BYTE_SIZE * 2);
	POP(e, t);
	v |= (t & BYTE_MASK) << BYTE_SIZE;
	POP(e, t);
	v |= (t & BYTE_MASK);
	PUSH(e, v);
	return NO_ERROR;
}

C(f_bwjoin)			/* bwjoin (b.lo b.hi -- w) 0xB0 */
{
	Cell t, v;
	
	IFCKSP(e, 2, 1);
	POP(e, t);
	v = (t & BYTE_MASK) << BYTE_SIZE;
	POP(e, t);
	v |= (t & BYTE_MASK);
	PUSH(e, v);
	return NO_ERROR;
}

C(f_wljoin)			/* wljoin (w.lo w.hi -- quad) 0x7D */
{
	Cell t, v;
	
	IFCKSP(e, 2, 1);
	POP(e, t);
	v = (t & DOUBLET_MASK) << DOUBLET_SIZE;
	POP(e, t);
	v |= (t & DOUBLET_MASK);
	PUSH(e, v);
	return NO_ERROR;
}

C(f_wbflip)			/* wbflip (w1 -- w2) 0x80 */
{
	uDoublet dp;
	
	IFCKSP(e, 1, 0);
	dp = TOP(e) & DOUBLET_MASK;
	e->sp[0] = ((dp >> BYTE_SIZE) & BYTE_MASK) |
			((dp & BYTE_MASK) << BYTE_SIZE);
	return NO_ERROR;
}

C(f_wbflips)		/* wbflips (waddr len --) 0x236 */
{
	Int i;
	uDoublet *dp;
	
	IFCKSP(e, 2, 0);
	POP(e, i);
	i /= sizeof (Doublet);
	POPT(e, dp, uDoublet*);
	
	for (; i--; dp++)
		*dp = ((*dp >> BYTE_SIZE) & BYTE_MASK) |
				((*dp & BYTE_MASK) << BYTE_SIZE);
	
	return NO_ERROR;
}

C(f_lbflip)			/* lbflip (q1 -- q2) 0x227 */
{
	uQuadlet qp;
	
	IFCKSP(e, 1, 0);
	qp = TOP(e) & QUADLET_MASK;
	e->sp[0] = ((qp >> (BYTE_SIZE * 3)) & BYTE_MASK) |
			((qp >> BYTE_SIZE) & (BYTE_MASK << BYTE_SIZE)) |
			((qp << BYTE_SIZE) & (BYTE_MASK << (BYTE_SIZE * 2))) |
			((qp & BYTE_MASK) << (BYTE_SIZE * 3));
	return NO_ERROR;
}

C(f_lbflips)		/* lbflips (qaddr len --) 0x228 */
{
	Int i;
	uQuadlet *qp;
	
	IFCKSP(e, 2, 0);
	POP(e, i);
	i /= sizeof (Quadlet);
	POPT(e, qp, uQuadlet*);
	
	for (; i--; qp++)
		*qp = ((*qp >> (BYTE_SIZE * 3)) & BYTE_MASK) |
				((*qp >> BYTE_SIZE) & (BYTE_MASK << BYTE_SIZE)) |
				((*qp << BYTE_SIZE) & (BYTE_MASK << (BYTE_SIZE * 2))) |
				((*qp & BYTE_MASK) << (BYTE_SIZE * 3));
	
	return NO_ERROR;
}

C(f_lwflip)			/* lwflip (q1 -- q2) 0x226 */
{
	uQuadlet qp;
	
	IFCKSP(e, 1, 0);
	qp = TOP(e) & QUADLET_MASK;
	e->sp[0] = ((qp >> DOUBLET_SIZE) & DOUBLET_MASK) |
			((qp & DOUBLET_MASK) << DOUBLET_SIZE);
	return NO_ERROR;
}

C(f_lwflips)		/* lwflips (qaddr len --) 0x237 */
{
	Int i;
	uQuadlet *qp;
	
	IFCKSP(e, 2, 0);
	POP(e, i);
	i /= sizeof (Quadlet);
	POPT(e, qp, uQuadlet*);
	
	for (; i--; qp++)
		*qp = ((*qp >> DOUBLET_SIZE) & DOUBLET_MASK) |
				((*qp & DOUBLET_MASK) << DOUBLET_SIZE);
	
	return NO_ERROR;
}

C(f_u2div)			/* u2/  (x1 -- x2) 0x58 */
{
	Cell t;
	
	IFCKSP(e, 1, 0);
	t = TOP(e);

	if (e->fcode32)
		e->sp[0] = (t & QUADLET_MASK) >> 1;
	else
		e->sp[0] = ((uCell)t) >> 1;

	return NO_ERROR;
}

C(f_between)		/* between (n min max -- min<=n<=max) 0x44 */
{
	Cell t, t2;
	
	IFCKSP(e, 3, 0);
	POP(e, t2);
	POP(e, t);
	DO_MASK32(e, t);
	DO_MASK32(e, t2);
	DO_MASK32(e, e->sp[0]);
	e->sp[0] = (TOP(e) >= t && TOP(e) <= t2) ?
			FTRUE : FFALSE;
	return NO_ERROR;
}

C(f_gteq)			/* >= (n1 n2 -- greater-or-equal?) 0x42 */
{
	Cell t;
	
	IFCKSP(e, 2, 0);
	POP(e, t);
	DO_MASK32(e, t);
	DO_MASK32(e, e->sp[0]);
	e->sp[0] = (TOP(e) >= t) ? FTRUE : FFALSE;
	return NO_ERROR;
}

C(f_lteq)			/* <= (n1 n2 -- less-or-equal?) 0x43 */
{
	Cell t;
	
	IFCKSP(e, 2, 0);
	POP(e, t);
	DO_MASK32(e, t);
	DO_MASK32(e, e->sp[0]);
	e->sp[0] = (TOP(e) <= t) ? FTRUE : FFALSE;
	return NO_ERROR;
}

C(f_lteq0)			/* 0<= (n -- less-or-equal-to-0?) 0x37 */
{
	IFCKSP(e, 1, 0);
	DO_MASK32(e, e->sp[0]);
	e->sp[0] = (TOP(e) <= 0) ? FTRUE : FFALSE;
	return NO_ERROR;
}

C(f_gteq0)			/* 0>= (n -- greater-or-equal-to-0?) 0x39 */
{
	IFCKSP(e, 1, 0);
	DO_MASK32(e, e->sp[0]);
	e->sp[0] = (TOP(e) >= 0) ? FTRUE : FFALSE;
	return NO_ERROR;
}

C(f_ulteq)			/* u<= (u1 u2 -- unsigned-less-or-equal?) 0x3F */
{
	Cell t;
	
	IFCKSP(e, 2, 0);
	POP(e, t);
	DO_UMASK32(e, t);
	DO_UMASK32(e, e->sp[0]);
	e->sp[0] = ((uCell)TOP(e) <= (uCell)t) ? FTRUE : FFALSE;
	return NO_ERROR;
}

C(f_ugteq)			/* u>= (u1 u2 -- unsigned-greater-or-equal?) 0x41 */
{
	Cell t;
	
	IFCKSP(e, 2, 0);
	POP(e, t);
	DO_UMASK32(e, t);
	DO_UMASK32(e, e->sp[0]);
	e->sp[0] = ((uCell)TOP(e) >= (uCell)t) ? FTRUE : FFALSE;
	return NO_ERROR;
}

C(f_arshft)			/* >>a (x1 u -- x2) 0x29 */
{
	uInt i;
	
	IFCKSP(e, 2, 0);
	POPTYPE(e, i, uInt);
	e->sp[0] >>= i;
	DO_MASK32(e, e->sp[0]);
	return NO_ERROR;
}

/* bell 0xAB: T_CONST for 0x07 */
/* bs 0xAA: T_CONST for 0x08 */
/* #line (-- a-addr) 0x94: T_ADDR to e->linesout */
/* #out (-- a-addr) 0x93: T_ADDR to e->outcol */

C(f_pack)			/* pack (str len addr -- pstr) 0x83 */
{
	Byte *t, *t2;
	Int i;
	
	IFCKSP(e, 3, 1);
	POPT(e, t, Byte*);
	POP(e, i);
	POPT(e, t2, Byte*);
	
	if (i > BYTE_MASK)
		i = BYTE_MASK;
		
	memmove(t + 1, t2, i);
	*(uByte*)t = i;
	t[1 + i] = '\0';
	PUSHP(e, t);
	return NO_ERROR;
}

C(f_lcc)			/* lcc (char1 -- char2) 0x82 */
{
	IFCKSP(e, 1, 0);

	if (!islower(e->sp[0]))
		e->sp[0] = tolower(e->sp[0]);
	
	return NO_ERROR;
}

C(f_upc)			/* upc (char1 -- char2) 0x81 */
{
	IFCKSP(e, 1, 0);

	if (!isupper(e->sp[0]))
		e->sp[0] = toupper(e->sp[0]);
	
	return NO_ERROR;
}

/* 0 0xA5: T_CONST for 0 */
/* 1 0xA6: T_CONST for 1 */
/* 2 0xA7: T_CONST for 2 */
/* 3 0xA8: T_CONST for 3 */

C(f_opcr)			/* (cr (--) 0x91 */
{
	display_char(e, 0x0D);
	return NO_ERROR;
}

C(f_dlrnumber)		/* $number (addr len -- true|n false) 0xA2 */
{
	Int len;
	Byte *str;
	Cell num, err;
	
	IFCKSP(e, 2, 2);
	POP(e, len);
	POPT(e, str, Byte*);
	parse_number(e->radix, &str, &len, &num, &err, TRUE);
	DO_MASK32(e, num);
	
	if (err == FFALSE)
		PUSH(e, num);
	
	PUSH(e, err);
	return NO_ERROR;
}

C(f_digit)			/* digit (char base -- digit true | char false) 0xA3 */
{
	Int base, chr;

	IFCKSP(e, 2, 2);
	POP(e, base);
	chr = TOP(e);
	
	if (chr >= '0' && chr <= '9')
		chr -= '0';
	else if (chr >= 'a' && chr <= 'z')
		chr = chr - 'a' + 10;
	else if (chr >= 'A' && chr <= 'Z')
		chr = chr - 'A' + 10;
	else
	{
		PUSH(e, FFALSE);
		return NO_ERROR;
	}
	
	if (chr < 0 || chr >= base)
		PUSH(e, FFALSE);
	else
	{
		DROP(e);
		PUSH(e, chr);
		PUSH(e, FTRUE);
	}
	
	return NO_ERROR;
}

C(f_dlrfind)		/* $find (str len -- xt true | str len false) 0xCB */
{
	Int i;
	Byte *p;
	Entry *ent;

	IFCKSP(e, 2, 2);
	POP(e, i);
	POPT(e, p, Byte*);
	ent = lookup_sym(e, p, i);
	
	if (ent)
	{
		PUSH(e, ent->xtok);
		PUSH(e, FTRUE);
	}
	else
	{
		PUSHP(e, p);
		PUSH(e, i);
		PUSH(e, FFALSE);
	}
		
	return NO_ERROR;
}

C(f_allocmem)		/* alloc-mem (len -- a-addr) 0x8B */
{
	IFCKSP(e, 1, 0);
	e->sp[0] = (uPtr)malloc(e->sp[0]);
	return (e->sp[0] == 0) ? E_OUT_OF_MEMORY : NO_ERROR;
}

C(f_allocaligned)		/* alloc-aligned (alignment len -- a-addr) */
{
	IFCKSP(e, 2, 1);
	e->sp[-1] = (uPtr)memalign(e->sp[-1], e->sp[0]);
	DROP(e);
	return (e->sp[0] == 0) ? E_OUT_OF_MEMORY : NO_ERROR;
}

C(f_freemem)		/* free-mem (a-addr len --) 0x8C */
{
	IFCKSP(e, 2, 0);
	DROP(e);		/* ignore the length - malloc takes care of itself */
	
	if (e->sp[0] != 0)
		free((char*)(uPtr)e->sp[0]);
	
	DROP(e);
	return NO_ERROR;
}


const Initentry init_funcs[] =
{
	{ "dup", f_dup, 0x47, F_NONE, T_FUNC
			HELP("(x -- x x) duplicate top item on stack") },
	{ "2dup", f_2dup, 0x53, F_NONE, T_FUNC
			HELP("(x1 x2 -- x1 x2 x1 x2) duplicate top two items on stack") },
	{ "?dup", f_qdup, 0x50, F_NONE, T_FUNC
			HELP("(x -- 0 | x x) duplicate top item if it is nonzero") },
	{ "over", f_over, 0x48, F_NONE, T_FUNC
			HELP("(x1 x2 -- x1 x2 x1) copy second item to top of stack") },
	{ "2over", f_2over, 0x54, F_NONE, T_FUNC
			HELP("(x1 x2 x3 x4 -- x1 x2 x3 x4 x1 x2) " \
					"copy second pair of items to top ") },
	{ "pick", f_pick, 0x4E, F_NONE, T_FUNC
			HELP("(xu..x1 x0 u -- xu..x1 x0 xu) " \
					"copy uth item to top of stack") },
	{ "tuck", f_tuck, 0x4C, F_NONE, T_FUNC
			HELP("(x1 x2 -- x2 x1 x2) copy top item underneath second item") },
	{ "drop", f_drop, 0x46, F_NONE, T_FUNC
			HELP("(x --) remove top item from stack") },
	{ "2drop", f_2drop, 0x52, F_NONE, T_FUNC
			HELP("(x1 x2 --) remove top two items from stack") },
	{ "nip", f_nip, 0x4D, F_NONE, T_FUNC
			HELP("(x1 x2 -- x2) remove second item from stack") },
	{ "roll", f_roll, 0x4F, F_NONE, T_FUNC
			HELP("(xu...x1 x0 u -- xu-1..x1 x0 xu) rotate u+1 stack items") },
	{ "rot", f_rot, 0x4A, F_NONE, T_FUNC
			HELP("(x1 x2 x3 -- x2 x3 x1) rotate top three items on stack") },
	{ "-rot", f_mrot, 0x4B, F_NONE, T_FUNC
			HELP("(x1 x2 x3 -- x3 x1 x2) rotate top three items on stack") },
	{ "2rot", f_2rot, 0x56, F_NONE, T_FUNC
			HELP("(x1 x2 x3 x4 x5 x6 -- x3 x4 x5 x6 x1 x2) " \
					"rotate three pairs of items") },
	{ "swap", f_swap, 0x49, F_NONE, T_FUNC
			HELP("(x1 x2 -- x2 x1) exchange top two stack items") },
	{ "2swap", f_2swap, 0x55, F_NONE, T_FUNC
			HELP("(x1 x2 x3 x4 -- x3 x4 x1 x2) " \
					"exchange top two pairs of items") },
	{ ">r", f_push_rstk, 0x30, F_NONE, T_FUNC
			HELP("(x --) (R: -- x) move top stack item to return stack") },
	{ "r>", f_pop_rstk, 0x31, F_NONE, T_FUNC
			HELP("(-- x) (R: x --) move top return stack item to stack") },
	{ "r@", f_top_rstk, 0x32, F_NONE, T_FUNC
			HELP("(-- x) (R: x -- x) copy top return stack item to stack") },
	{ "depth", f_depth, 0x51, F_NONE, T_FUNC
			HELP("(-- n) return count of items on the stack") },
	{ "+", f_plus, 0x1E, F_NONE, T_FUNC
			HELP("(n1 n2 -- sum) add n1 to n2") },
	{ "-", f_minus, 0x1F, F_NONE, T_FUNC
			HELP("(n1 n2 -- diff) subtract n2 from n1") },
	{ "*", f_times, 0x20, F_NONE, T_FUNC
			HELP("(n1 n2 -- prod) multiply n1 by n2") },
	{ "/", f_div, 0x21, F_NONE, T_FUNC
			HELP("(n1 n2 -- quot) divide n1 by n2, return quotient") },
	{ "mod", f_mod, 0x22, F_NONE, T_FUNC
			HELP("(n1 n2 -- rem) divide n1 by n2, return remainder") },
	{ "/mod", f_divmod, 0x2A, F_NONE, T_FUNC
			HELP("(n1 n2 -- rem quot) " \
					"divide n1 by n2, return remainder and quotient") },
	{ "u/mod", f_udivmod, 0x2B, F_NONE, T_FUNC
			HELP("(u1 u2 -- urem uquot)\n" \
				"\tunsigned divide u1 by u2, return remainder and quotient") },
	{ "abs", f_abs, 0x2D, F_NONE, T_FUNC
			HELP("(n -- u) return absolute value of n") },
	{ "negate", f_negate, 0x2C, F_NONE, T_FUNC
			HELP("(n1 -- n2) return negation of n1 (change sign)") },
	{ "max", f_max, 0x2F, F_NONE, T_FUNC
			HELP("(n1 n2 -- n1|n2) return greater of n1 or n2") },
	{ "min", f_min, 0x2E, F_NONE, T_FUNC
			HELP("(n1 n2 -- n1|n2) return lesser of n1 or n2") },
	{ "bounds", f_bounds, 0xAC, F_NONE, T_FUNC
			HELP("(n cnt -- n+cnt n) prepare argument for do or do? loop") },
	{ "<<", f_lshift, 0x27, F_NONE, T_FUNC
			HELP("(x1 u -- x2) synonym for lshift") },	/* synonym */
	{ "lshift", f_lshift, 0x27, F_NONE, T_FUNC
			HELP("(x1 u -- x2) " \
					"shift x1 left by u bit-places, zero-fill low bits") },
	{ ">>", f_rshift, 0x28, F_NONE, T_FUNC
			HELP("(x1 u -- x2) synonym for rshift") },	/* synonym */
	{ "rshift", f_rshift, 0x28, F_NONE, T_FUNC
			HELP("(x1 u -- x2) " \
					"shift x1 right by u bit-places, zero-fill high bits") },
	{ "2*", f_times2, 0x59, F_NONE, T_FUNC
			HELP("(x1 -- x2) shift x1 left by one bit - multiply by two") },
	{ "2/", f_div2, 0x57, F_NONE, T_FUNC
			HELP("(x1 -- x2) shift x1 right by one bit - divide by two") },
	{ "and", f_and, 0x23, F_NONE, T_FUNC
			HELP("(x1 x2 -- x3) return bitwise logical and of x1 and x2") },
	{ "or", f_or, 0x24, F_NONE, T_FUNC
			HELP("(x1 x2 -- x3) return bitwise inclusive-or of x1 and x2") },
	{ "xor", f_xor, 0x25, F_NONE, T_FUNC
			HELP("(x1 x2 -- x3) return bitwise exclusive-or of x1 and x2") },
	{ "not", f_invert, 0x26, F_NONE, T_FUNC
			HELP("(x1 -- x2) synonym for invert") },	/* synonym */
	{ "invert", f_invert, 0x26, F_NONE, T_FUNC
			HELP("(x1 -- x2) invert all bits of x1") },
	{ "d+", f_dplus, 0xD8, F_NONE, T_FUNC
			HELP("(d1 d2 -- d.sum) add d1 to d2 giving double-number d.sum") },
	{ "d-", f_dminus, 0xD9, F_NONE, T_FUNC
			HELP("(d1 d2 -- d.diff) " \
					"subtract d2 from d1 giving double-number d.diff") },
	{ "um*", f_umtimes, 0xD4, F_NONE, T_FUNC
			HELP("(u1 u2 -- ud.prod) " \
					"unsigned multiply with double-number product") },
	{ "umd/mod", f_umddivmod, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(ud1 ud2 -- ud.rem ud.quot) " \
					"divide unsigned double-number ud1 by double-number ud2") },
			/* [non-standard] */
	{ "um/mod", f_umdivmod, 0xD5, F_NONE, T_FUNC
			HELP("(ud u -- urem uquot) " \
					"divide unsigned double-number ud by u") },
	{ "ca1+", f_charsize, 0x62, F_NONE, T_FUNC
			HELP("(addr1 -- addr2) symonym for char+") },	/* synonym */
	{ "char+", f_charsize, 0x62, F_NONE, T_FUNC
			HELP("(addr1 -- addr2) increment addr1 by value of /c") },
	{ "na1+", f_cellsize, 0x65, F_NONE, T_FUNC
			HELP("(addr1 -- addr2) synonym for cell+") },
	{ "cell+", f_cellsize, 0x65, F_NONE, T_FUNC
			HELP("(addr1 -- addr2) increment addr1 by value of /n") },
	{ "/c*", f_chartimes, 0x66, F_NONE, T_FUNC
			HELP("(nu1 -- nu2) synonym for chars") },	/* synonym */
	{ "chars", f_chartimes, 0x66, F_NONE, T_FUNC
			HELP("(nu1 -- nu2) multiply nu1 by value of /c") },
	{ "/n*", f_celltimes, 0x69, F_NONE, T_FUNC
			HELP("(nu1 -- nu2) synonym for cells") },	/* synonym */
	{ "cells", f_celltimes, 0x69, F_NONE, T_FUNC
			HELP("(nu1 -- nu2) multiply nu1 by value of /n") },
	{ "aligned", f_aligned, 0xAE, F_NONE, T_FUNC
			HELP("(n1 -- n1 | a-addr) " \
					"increase n1 to a valid address boundary") },
	{ "@", f_get, 0x6D, F_NONE, T_FUNC
			HELP("(a-addr -- x) fetch item x from cell at a-addr") },
	{ "2@", f_2get, 0x76, F_NONE, T_FUNC
			HELP("(a-addr -- x1 x2) fetch cell pair from a-addr") },
	{ "c@", f_charget, 0x71, F_NONE, T_FUNC
			HELP("(addr -- byte) fetch byte from addr") },
	{ "!", f_set, 0x72, F_NONE, T_FUNC
			HELP("(x a-addr --) store item x to cell at a-addr") },
	{ "2!", f_2set, 0x77, F_NONE, T_FUNC
			HELP("(x1 x2 a-addr --) store cell pair at a-addr") },
	{ "+!", f_plusset, 0x6C, F_NONE, T_FUNC
			HELP("(nu a-addr --) add nu to cell at a-addr") },
	{ "c!", f_charset, 0x75, F_NONE, T_FUNC
			HELP("(byte addr --) store byte to addr") },
	{ "move", f_move, 0x78, F_NONE, T_FUNC
			HELP("(src-addr dest-addr len --) " \
					"copy len bytes from scr-addr to dest-addr") },
	{ "fill", f_fill, 0x79, F_NONE, T_FUNC
			HELP("(addr len byte --) set len bytes at addr to byte") },
	{ "key?", f_keyq, 0x8D, F_NONE, T_FUNC
			HELP("(-- pressed?) " \
					"return true if an input character is available") },
	{ "key", f_key, 0x8E, F_NONE, T_FUNC
			HELP("(-- char) read a character from console input device") },
	{ "expect", f_expect, 0x8A, F_NONE, T_FUNC
			HELP("(addr len --) get edited input line stored at addr") },
	{ "span", (Command)offsetof(Environ, expect_len), 0x88, F_NONE, T_ADDR
			HELP("(-- a-addr) " \
					"variable holding number of chars received by expect") },
	{ "bl", (Command)0x20, 0xA9, F_NONE, T_CONST
			HELP("(-- 0x20) ASCII code for space (blank) character") },
	{ "emit", f_emit , 0x8F, F_NONE, T_FUNC
			HELP("(char --) display the given ASCII character") },
	{ "type", f_type, 0x90, F_NONE, T_FUNC
			HELP("(text-str text-len --) " \
					"display text-len chars beginning at text-str") },
	{ "cr", f_cr, 0x92, F_NONE, T_FUNC
			HELP("cr (--) display a carriage-return and newline") },
	{ "count", f_count, 0x84, F_NONE, T_FUNC
			HELP("(pstr -- str len) " \
					"unpack a counted (Pascal) string to a text string") },
	{ "base", (Command)offsetof(Environ, radix), 0xA0, F_NONE, T_ADDR
			HELP("(-- a-addr) variable containing number-conversion radix") },
	{ ".", f_dot, 0x9D, F_NONE, T_FUNC
			HELP("(nu --) display number nu and a trailing space") },
	{ "u.", f_udot, 0x9B, F_NONE, T_FUNC
			HELP("(u --) display an unsigned number and trailing space") },
	{ ".r", f_dotr, 0x9E, F_NONE, T_FUNC
			HELP("(n size --) display a right-justified signed number") },
	{ "u.r", f_udotr, 0x9C, F_NONE, T_FUNC
			HELP("(u size --) display a right-justified unsigned number") },
	{ ".s", f_dots, 0x9F, F_PAGINATE, T_FUNC
			HELP("(... -- ...) display entire stack contents (unchanged)") },
	{ "<#", f_lthash, 0x96, F_NONE, T_FUNC
			HELP("(--) initialize pictured numeric output conversion") },
	{ "#", f_hash, 0xC7, F_NONE, T_FUNC
			HELP("(ud1 -- ud2) " \
					"convert a digit in pictured numeric output conversion") },
	{ "#s", f_hashs, 0xC8, F_NONE, T_FUNC
			HELP("(ud -- 0 0) " \
					"convert remaining digits in pictured numeric output") },
	{ "#>", f_hashgt, 0xC9, F_NONE, T_FUNC
			HELP("(ud -- str len) end pictured numeric output conversion") },
	{ "hold", f_hold, 0x95, F_NONE, T_FUNC
			HELP("(char --) " \
					"add char in pictured numeric output conversion ") },
	{ "sign", f_sign, 0x98, F_NONE, T_FUNC
			HELP("(n --) if n < 0 insert - in pictured numeric output") },
	{ "<", f_lt, 0x3A, F_NONE, T_FUNC
			HELP("< (n1 n2 -- less?) return true if n1 is less than n2") },
	{ "<>", f_noteq, 0x3D, F_NONE, T_FUNC
			HELP("<> (x1 x2 -- not-equal?) " \
					"return true if x1 is not equal to x2") },
	{ "=", f_eq, 0x3C, F_NONE, T_FUNC
			HELP("(x1 x2 -- equal?) return true if x1 is equal to x2") },
	{ ">", f_gt, 0x3B, F_NONE, T_FUNC
			HELP("(n1 n2 -- greater?) return true if n1 is greater than n2") },
	{ "within", f_within, 0x45, F_NONE, T_FUNC
			HELP("(n min max -- min<=n<max?)\n" \
					"\treturn true if n is between min and max-1 inclusive") },
	{ "0<", f_lt0, 0x36, F_NONE, T_FUNC
			HELP("(n -- less-than-0?) return true if n is less than zero") },
	{ "0<>", f_neq0, 0x35, F_NONE, T_FUNC
			HELP("(n -- not-equal-to-0?) " \
					"return true if n is not equal to zero") },
	{ "0=", f_eq0, 0x34, F_NONE, T_FUNC
			HELP("(n -- equal-to-0?) return true if n is equal to zero") },
	{ "0>", f_gt0, 0x38, F_NONE, T_FUNC
			HELP("0> (n -- greater-than-0?) " \
					"return true if n is greater than zero") },
	{ "u<", f_ult, 0x40, F_NONE, T_FUNC
			HELP("(u1 u2 -- unsigned-less?) " \
					"return true if u1 is less than u2, unsigned") },
	{ "u>", f_ugt, 0x3E, F_NONE, T_FUNC
			HELP("(u1 u2 -- unsigned-greater?) " \
					"return true if u1 is greater than u2, unsigned") },

	{ "/c", (Command)sizeof (Byte), 0x5A, F_NONE, T_CONST
			HELP("(-- n) the number of address units in a byte") },
	{ "/w", (Command)sizeof (Doublet), 0x5B, F_NONE, T_CONST
			HELP("(-- n) the number of address units in a doublet") },
	{ "/l", (Command)sizeof (Quadlet), 0x5C, F_NONE, T_CONST
			HELP("(-- n) the number of address units in a quadlet") },
	{ "/n", f_slashn, 0x5D, F_NONE, T_FUNC
			HELP("(-- n) the number of address units in a cell") },

	{ "ca+", f_iplusbyte, 0x5E, F_NONE, T_FUNC
			HELP("(addr1 index -- addr2) increment addr1 by index times /c") },
	{ "wa+", f_iplusdoub, 0x5F, F_NONE, T_FUNC
			HELP("(addr1 index -- addr2) increment addr1 by index times /w") },
	{ "la+", f_iplusquad, 0x60, F_NONE, T_FUNC
			HELP("(addr1 index -- addr2) increment addr1 by index times /l") },
	{ "na+", f_ipluscell, 0x61, F_NONE, T_FUNC
			HELP("(addr1 index -- addr2) increment addr1 by index times /n") },
	{ "wa1+", f_plusdoub, 0x63, F_NONE, T_FUNC
			HELP("(addr1 -- addr2) increment addr1 by /w") },
	{ "la1+", f_plusquad, 0x64, F_NONE, T_FUNC
			HELP("(addr1 -- addr2) increment addr1 by /l") },
	{ "/w*", f_multdoub, 0x67, F_NONE, T_FUNC
			HELP("(nu1 -- nu2) multiply nu1 by /w") },
	{ "/l*", f_multquad, 0x68, F_NONE, T_FUNC
			HELP("(nu1 -- nu2) multiply nu1 by /l") },
	{ "w@", f_getudoub, 0x6F, F_NONE, T_FUNC
			HELP("(waddr -- w) " \
					"fetch doublet w from waddr, no sign extension") },
	{ "<w@", f_getdoub, 0x70, F_NONE, T_FUNC
			HELP("(waddr -- w) fetch doublet w from waddr, sign extended") },
	{ "l@", f_getquad, 0x6E, F_NONE, T_FUNC
			HELP("(qaddr -- q) fetch quadlet q from qaddr") },
	{ "w!", f_setdoub, 0x74, F_NONE, T_FUNC
			HELP("(w waddr --) store doublet w to waddr") },
	{ "l!", f_setquad, 0x73, F_NONE, T_FUNC
			HELP("(quad qaddr --) store quadlet to qaddr") },
	{ "w,", f_aligndoub, 0xD1, F_NONE, T_FUNC
			HELP("(w --) compile doublet w into dictionary, doublet-aligned") },
	{ "l,", f_alignquad, 0xD2, F_NONE, T_FUNC
			HELP("(q --) compile quadlet q into dictionary, doublet-aligned") },
	{ "off", f_off, 0x6B, F_NONE, T_FUNC
			HELP("(a-addr --) store false to cell at a-addr") },
	{ "on", f_on, 0x6A, F_NONE, T_FUNC
			HELP("(a-addr --) store true to cell at a-addr") },
	{ "u#", f_uhash, 0x99, F_NONE, T_FUNC
			HELP("(u1 -- u2) " \
					"convert an unsigned digit in pictured numeric output") },
	{ "u#s", f_uhashs, 0x9A, F_NONE, T_FUNC
			HELP("(u -- 0) " \
			"convert remaining unsigned digits in pictured numeric output") },
	{ "u#>", f_uhashgt, 0x97, F_NONE, T_FUNC
			HELP("(u -- str len) end pictured numeric output conversion") },
	{ "comp", f_comp, 0x7A, F_NONE, T_FUNC
			HELP("(addr1 addr2 len -- ?diff?) " \
					"compare two arrays of length len") },
	{ "lbsplit", f_lbsplit, 0x7E, F_NONE, T_FUNC
			HELP("(quad -- b.lo b2 b3 b4.hi) " \
					"split a quadlet into four bytes") },
	{ "lwsplit", f_lwsplit, 0x7C, F_NONE, T_FUNC
			HELP("(quad -- w1.lo w2.hi) " \
					"split a quadlet into two doublets") },
	{ "wbsplit", f_wbsplit, 0xAF, F_NONE, T_FUNC
			HELP("(w -- b1.lo b2.hi) split a doublet into two bytes") },
	{ "bljoin", f_bljoin, 0x7F, F_NONE, T_FUNC
			HELP("(b1.lo b2 b3 b4.hi -- quad) " \
					"join four bytes to form a quadlet") },
	{ "bwjoin", f_bwjoin, 0xB0, F_NONE, T_FUNC
			HELP("(b.lo b.hi -- w) join four bytes to form a doublet") },
	{ "wljoin", f_wljoin, 0x7D, F_NONE, T_FUNC
			HELP("(w.lo w.hi -- quad) join two doublets to form a quadlet") },
	{ "flip", f_wbflip, 0x80, F_NONE, T_FUNC		/* obsolete synonym */
			HELP("(w1 -- w2) obsolete synonym for wbflip") },
	{ "wbflip", f_wbflip, 0x80, F_NONE, T_FUNC
			HELP("(w1 -- w2) swap the bytes within a doublet") },
	{ "wflips", f_wbflips, 0x236, F_NONE, T_FUNC		/* obsolete synonym */
			HELP("(waddr len --) obsolete synonym for wbflips") },
	{ "wbflips", f_wbflips, 0x236, F_NONE, T_FUNC
			HELP("(waddr len --) " \
					"swap the bytes within each doublet in the region") },
	{ "lbflip", f_lbflip, 0x227, F_NONE, T_FUNC
			HELP("(q1 -- q2) reverse the bytes in a quadlet") },
	{ "lbflips", f_lbflips, 0x228, F_NONE, T_FUNC
			HELP("(qaddr len --) " \
				"reverse the bytes in all quadlets in the region") },
	{ "lwflip", f_lwflip, 0x226, F_NONE, T_FUNC
			HELP("(q1 -- q2) swap the doublets within a quadlet") },
	{ "lwflips", f_lwflips, 0x237, F_NONE, T_FUNC
			HELP("(qaddr len --) " \
					"swap the doublets in all quadlets in the region") },
	{ "u2/", f_u2div, 0x58, F_NONE, T_FUNC
			HELP("(x1 -- x2) " \
					"shift x1 right by one bit (unsigned divide by two)") },
	{ "between", f_between, 0x44, F_NONE, T_FUNC
			HELP("(n min max -- min<=n<=max)\n" \
					"\treturn true if n is between min and max, inclusive") },
	{ ">=", f_gteq, 0x42, F_NONE, T_FUNC
			HELP("(n1 n2 -- greater-or-equal?)\n" \
					"\treturn true if n1 is greater than or equal to n2") },
	{ "<=", f_lteq , 0x43, F_NONE, T_FUNC
			HELP("(n1 n2 -- less-or-equal?) " \
					"return true if n1 is less than or equal to n2") },
	{ "0<=", f_lteq0, 0x37, F_NONE, T_FUNC
			HELP("(n -- less-or-equal-to-0?) " \
					"return true if n is less than or equal to zero") },
	{ "0>=", f_gteq0, 0x39, F_NONE, T_FUNC
			HELP("(n -- greater-or-equal-to-0?)\n" \
					"\treturn true if n is greater than or equal to zero") },
	{ "u<=", f_ulteq, 0x3F, F_NONE, T_FUNC
			HELP("(u1 u2 -- unsigned-less-or-equal?)\n" \
				"\treturn true if u1 is less than or equal to u2, unsigned") },
	{ "u>=", f_ugteq, 0x41, F_NONE, T_FUNC
			HELP("(u1 u2 -- unsigned-greater-or-equal?)\n" \
			"\treturn true if u1 is greater than or equal to u2, unsigned") },
	{ ">>a", f_arshft, 0x29, F_NONE, T_FUNC
			HELP("(x1 u -- x2) arithmetic shift x1 right by u bits") },
	{ "bell", (Command)0x07, 0xAB, F_NONE, T_CONST
			HELP("(-- 0x07) ASCII code for bell") },
	{ "bs", (Command)0x08, 0xAA, F_NONE, T_CONST 
			HELP("(-- 0x08) ASCII code for backspace") },

	{ "#line", (Command)offsetof(Environ, linesout), 0x94, F_NONE, T_ADDR
			HELP("(-- a-addr) variable holding the output line number") },
	{ "#out", (Command)offsetof(Environ, outcol), 0x93, F_NONE, T_ADDR
			HELP("(-- a-addr) variable holding the output column number") },

	{ "pack", f_pack, 0x83, F_NONE, T_FUNC
			HELP("(str len addr -- pstr) " \
					"pack a text string into a counted (Pascal) string") },
	{ "lcc", f_lcc, 0x82, F_NONE, T_FUNC
			HELP("(char1 -- char2) convert ASCII char1 to lowercase") },
	{ "upc", f_upc, 0x81, F_NONE, T_FUNC
			HELP("(char1 -- char2) convert ASCII char1 to uppercase") },

	/* these are the same fcode as -1 and 0 respectively for tokenizing */
	{ "true", (Command)FTRUE, 0xA4, F_NONE, T_CONST		/* synonym */
			HELP("(-- -1) true constant (negative one)") },
	{ "false", (Command)FFALSE, 0xA5, F_NONE, T_CONST		/* synonym */
			HELP("(-- 0) false constant (zero)") },

	{ "-1", (Command)-1, 0xA4, F_NONE, T_CONST
			HELP("(-- -1) constant negative-one") },
	{ "0", (Command)0, 0xA5, F_NONE, T_CONST
			HELP("(-- 0) constant zero") },
	{ "1", (Command)1, 0xA6, F_NONE, T_CONST
			HELP("(-- 1) constant one") },
	{ "2", (Command)2, 0xA7, F_NONE, T_CONST
			HELP("(-- 2) constant two") },
	{ "3", (Command)3, 0xA8, F_NONE, T_CONST
			HELP("(-- 3) constant three") },
	{ "(cr", f_opcr, 0x91, F_NONE, T_FUNC
			HELP("(--) display the carriage-return character, 0x0D") },
	{ "$number", f_dlrnumber, 0xA2, F_NONE, T_FUNC
			HELP("(addr len -- true|n false) convert a string to a number") },
	{ "digit", f_digit, 0xA3, F_NONE, T_FUNC
			HELP("(char base -- digit true | char false)\n" \
					"\tconvert a character to a digit in base") },
	{ "$find", f_dlrfind, 0xCB, F_NONE, T_FUNC
			HELP("(name len -- xt true | name len false)\n" \
					"\tfind the command name in dictionary") },
	{ "alloc-mem", f_allocmem, 0x8B, F_NONE, T_FUNC
			HELP("(len -- a-addr) allocate len bytes of memory") },
	{ "alloc-aligned", f_allocaligned, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(alignment len -- a-addr) allocate len bytes of memory aligned") },
	{ "free-mem", f_freemem, 0x8C, F_NONE, T_FUNC
			HELP("(a-addr len --) free memory allocated by alloc-mem") },
	
	{ NULL, NULL }
};
