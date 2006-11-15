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

/* (c) Copyright 1996-2000 by CodeGen, Inc.  All Rights Reserved. */

#include "defs.h"

EC(f_align);			/* align (--) */

/* More standard Fcode functions, only related to control */

C(f_evaluate)		/* evaluate (... str len -- ?) 0xCD */
{
	Int len;
	Byte *str;
	Byte *tmp;
	Retcode ret;
	
	IFCKSP(e, 2, 0);
	POP(e, len);
	POPT(e, str, Byte*);
	tmp = malloc(len + 1);

	if (tmp == NULL)
		return E_OUT_OF_MEMORY;

	memcpy(tmp, str, len);
	tmp[len] = '\0';
	ret = interp_text(e, tmp, len);
	free(tmp);
	return ret;
}

C(f_execute)		/* execute (... xt -- ?) 0x1D */
{
	Int xtok;
	
	IFCKSP(e, 1, 0);
	POP(e, xtok);
	return execute_xtok(e, xtok);
}

C(f_exit)			/* exit (--) (R: sys --) 0x33 */
{
	return R_END;
}

CC(f_abort)			/* abort (... --) (R: ... --) 0x216 */
{
	e->sp = e->stack;
	e->rsp = e->rstack;
	
	/* behave just like "-1 throw */
	return E_ABORT;
}

C(f_catch)			/* catch (... xt -- ? error-code | ? false) 0x217 */
{
	Int code;
	Retcode ret;
	Cell saveinst = e->currinst;
	Cell *saversp = e->rsp;
	Cell *savesp;
	
	IFCKSP(e, 1, 0);
	POP(e, code);
	savesp = e->sp;
	ret = execute_xtok(e, code);

	if (ret != NO_ERROR)
	{
		e->sp = savesp;
		e->rsp = saversp;
		e->currinst = saveinst;
	}

	/* pass these through to get back to a prompt */
	if (ret == E_USER_ABORT || ret == R_END)
		return ret;

	PUSH(e, ret);
	return NO_ERROR;
}

C(f_throw)			/* throw (... error-code -- ? error-code | ...) 0x218 */
{
	Cell t;
	
	IFCKSP(e, 1, 0);
	POP(e, t);
	return (Retcode)t;
}

C(f_here)			/* here (-- addr) 0xAD */
{
	/* not defined as a T_ADDRVAL so that "b(to)" cannot alter it */
	IFCKSP(e, 0, 1);
	PUSHP(e, e->fbrk);
	return NO_ERROR;
}

C(f_ccomma)			/* c, (byte --) 0xD0 */
{
	Int b;
	
	IFCKSP(e, 1, 0);
	POP(e, b);
	return compile_byte(e, b);
}

C(f_comma)			/* , (x --) 0xD3 */
{
	Cell w;
	
	IFCKSP(e, 1, 0);
	POP(e, w);

#ifdef SF_64BIT
	if (!e->fcode32)
		compile_num32(e, w >> QUADLET_SIZE);
#endif

	return compile_num32(e, w & QUADLET_MASK);
}

C(f_compilecomma)	/* compile, (xt --) 0xDD */
{
	Int xtok;
	
	IFCKSP(e, 1, 0);
	POP(e, xtok);
	return compile_fcode(e, xtok);
}

/* state 0xDC: T_ADDR to &e->comp_state */

C(f_tobody)			/* >body (xt -- a-addr) 0x86 */
{
	Int xt;

	IFCKSP(e, 1, 1);
	POP(e, xt);

	if (xt < 0 || xt >= e->numxtoks)	/* sanity check */
		return E_ILLEGAL_FCODE;

	switch (e->xtoks[xt]->type)
	{
	case T_CONST:
	case T_VALUE:
	case T_2CONST:
	case T_VAR:
	case T_FIELD:
	case T_EXECTOKEN:
	case T_DEFER:
		PUSHP(e, &e->xtoks[xt]->v.val);
		break;

	default:
		PUSHP(e, e->xtoks[xt]->v.addr);
		break;
	}

	return NO_ERROR;
}

C(f_frombody)		/* body> (a-addr -- xt) 0x85 */
{
	Byte *addr;
	Int xt;

	IFCKSP(e, 1, 1);
	POPT(e, addr, Byte*);

	for (xt = 0; xt < e->numxtoks; xt++)
	{
		switch (e->xtoks[xt]->type)
		{
		case T_CONST:
		case T_VALUE:
		case T_2CONST:
		case T_VAR:
		case T_FIELD:
		case T_EXECTOKEN:
		case T_DEFER:
			if (addr == (Byte*)&e->xtoks[xt]->v.val)
			{
				PUSHP(e, xt);
				return NO_ERROR;
			}

			break;

		default:
			if (addr == e->xtoks[xt]->v.addr)
			{
				PUSHP(e, xt);
				return NO_ERROR;
			}
			
			break;
		}
	}

	return E_INVALID_MEMORY_ADDR;
}

C(f_noop)			/* noop (--) 0x7B */
{
	return NO_ERROR;
}


/* 5.3.3.1 Defining new FCode functions */

C(f_instance)		/* instance (--) 0xC0 */
{
	e->instance = TRUE;
	return NO_ERROR;
}

C(f_newtoken)		/* new-token (F:/FCode#/ --) 0xB5 */
{
	Int fcode;

	if (f_align(e))
		return E_NEWDEF_ERROR;
	
	fcode = next_fcode_num(e);	/* get the fcode number */

	if (fcode < 0 || fcode >= NUM_FCODES)
		return E_ILLEGAL_FCODE;

	e->newdef = new_entry();

	if (e->newdef == NULL)
		return E_OUT_OF_MEMORY;

	e->newdef->flags |= F_HEADERLESS;
	e->newdef->type = T_FCODE;
	e->newdef->v.fcode = e->fbrk;
	set_xtok(e, e->newdef);
	e->fcodes[fcode] = e->newdef->xtok;
	return NO_ERROR;
}

C(f_namedtoken)		/* named-token (F:/FCode-string FCode#/ --) 0xB6 */
{
	Byte *str;
	Retcode r;
	
	next_fcode_string(e, &str);
	r = f_newtoken(e);
	
	if (r != NO_ERROR)
		return r;
	
	e->newdef->name = pstrdup(str);		/* need to malloc */

	if (get_config_bool(e, "fcode-debug?", CSTR))
		e->newdef->flags &= ~F_HEADERLESS;

	return NO_ERROR;
}

C(f_externaltoken)	/* external-token (F:/FCode-string FCode#/ --) 0xCA */
{
	Retcode r;
	
	r = f_namedtoken(e);
	
	if (r != NO_ERROR)
		return r;
	
	if (*e->newdef->name == '\0')
		return E_NEWDEF_ERROR;
	
	e->newdef->flags &= ~F_HEADERLESS;
	return NO_ERROR;
}

CC(f_bsemicolon)	/* b(;) (--) 0xC2 */
{
	Retcode ret;
	
	if (!e->iscompiling)
		return E_COMPILATION_ERROR;
	
	/* temp definitions get executed immediately */
	if (e->istemp)
		return resolve_temp_compile(e);

	/* compile the final byte, set our length, and take the next xtok slot */
	ret = compile_fcode(e, 0x00);		/* end0 */
	
	if (ret != NO_ERROR)
		return ret;
		
	e->newdef->len = e->fbrk - e->newdef->v.fcode;
	e->comp_state = FFALSE;
	e->iscompiling = FALSE;
	ret = create_newdef(e);
	return ret;
}

CC(f_bcolon)		/* b(:) (--) (E:... -- ?) 0xB7 */
{
	if (e->iscompiling)
		return E_COMPILATION_ERROR;
	
	e->newdef->type = T_FCODE;
	e->newdef->v.fcode = e->fbrk;
	e->comp_state = FTRUE;
	e->iscompiling = TRUE;
	e->tempsp = e->sp;
	return NO_ERROR;
}

CC(f_bbuffercolon)	/* b(buffer:) (size --) (E: -- a-addr) 0xBD */
{
	Int len;
	
	IFCKSP(e, 1, 0);
	POP(e, len);
	e->newdef->type = T_BUFFER;
	e->newdef->v.addr = malloc(len);
	
	if (e->newdef->v.addr == NULL)
		return E_OUT_OF_MEMORY;
	
	e->newdef->flags |= F_MALLOC;
	e->newdef->len = len;
	memset(e->newdef->v.addr, 0, len);
	return create_newdef(e);
}

CC(f_bconstant)		/* b(constant) (n1 --) (E:-- n1) 0xBA */
{
	IFCKSP(e, 1, 0);
	e->newdef->type = T_CONST;	
	POP(e, e->newdef->v.val);
	return create_newdef(e);
}

CC(f_bcreate)		/* b(create) (--) (E:-- a-addr) 0xBB */
{
	Retcode ret;
	Byte **mem;

	ret = f_align(e);

	if (ret != NO_ERROR)
		return ret;
	
	mem = (Byte**)falloc(e, sizeof (Cell));

	if (mem == NULL)
		return E_OUT_OF_MEMORY;

	*mem = 0;		/* store pointer to fcode here later in "does>" */
	e->newdef->type = T_CREATE;
	e->newdef->v.fcode = (Byte*)mem;
	return create_newdef(e);
}

CC(f_bdefer)		/* b(defer) (--) (E:... -- ?) 0xBC */
{
	e->newdef->type = T_DEFER;
	e->newdef->v.xtok = -1;	
	return create_newdef(e);
}

/* b(field) (offset size -- offset+size) (E:addr -- addr+offset) 0xBE */
CC(f_bfield)
{
	IFCKSP(e, 2, 0);
	e->newdef->type = T_FIELD;
	POP(e, e->newdef->len);			/* size */
	e->newdef->v.val = TOP(e);		/* offset */
	*e->sp += e->newdef->len;
	return create_newdef(e);
}

CC(f_bvalue)		/* b(value) (x --) (E: -- x) 0xB8 */
{
	IFCKSP(e, 1, 0);
	e->newdef->type = T_VALUE;
	POP(e, e->newdef->v.val);
	return create_newdef(e);
}

CC(f_bvariable)		/* b(variable) (--) (E: -- a-addr) 0xB9 */
{
	e->newdef->type = T_VAR;
	e->newdef->v.val = 0;
	return create_newdef(e);
}

/* (is-user-word) (name-str name-len xt --) (E:... -- ?) 0x214 */
C(f_isuserword)
{
	Int xt;
	Int len;
	Byte *str;
	
	IFCKSP(e, 3, 0);
	POP(e, xt);
	POP(e, len);
	POPT(e, str, Byte*);
	e->newdef = new_entry();

	if (e->newdef == NULL)
		return E_OUT_OF_MEMORY;

	e->newdef->type = T_EXECTOKEN;
	e->newdef->name = lstrdup(str, len);
	e->newdef->v.xtok = xt;
	e->comp_state = FTRUE;
	e->iscompiling = TRUE;
	e->tempsp = e->sp;
	f_bsemicolon(e);
	return NO_ERROR;
}

C(f_gettoken)		/* get-token (fcode# -- xt immediate?) 0xDA */
{
	Int fcode;
	
	IFCKSP(e, 1, 2);
	POP(e, fcode);

	/* only allow getting fcodes in OF-specified range */
	if (fcode < 0 || fcode >= NUM_FCODES)
		return E_ILLEGAL_FCODE;

	if ((fcode & 0xFFF) == 0x600)	/* in "escaped" vendor-specific range */
		fcode = 0xA4;				/* true */
	if (e->fcodes[fcode] == INVALID_FCODE)
		fcode = 0xFC;				/* ferror */
	else
		fcode = e->fcodes[fcode];	/* convert fcode to xtok */

	PUSH(e, fcode & FCODE_MASK);
	
	if ((fcode & FCODE_IMMEDIATE) || (e->xtoks[fcode]->flags & F_IMMEDIATE))
		PUSH(e, FTRUE);
	else
		PUSH(e, FFALSE);
	
	return NO_ERROR;
}

C(f_settoken)		/* set-token (xt immediate? fcode# --) 0xDB */
{
	Int fcode, xtok;
	Cell imm;
	
	IFCKSP(e, 3, 0);
	POP(e, fcode);
	POP(e, imm);
	POP(e, xtok);

	/* only allow changing fcodes in OF-specified range */
	if (fcode < 0 || fcode >= NUM_FCODES)
		return E_ILLEGAL_FCODE;

	e->fcodes[fcode] = xtok;

	if (imm)
		e->fcodes[fcode] |= FCODE_IMMEDIATE;

	return NO_ERROR;
}


/* 5.3.3.2 - Literals */

C(f_blit)			/* b(lit) (-- n1) 0x10 */
{
	Long i = next_fcode_num32(e);
	Retcode ret = NO_ERROR;
	
	if (e->iscompiling)
	{
		compile_fcode(e, 0x10);		/* b(lit) */
		ret = compile_num32(e, i);
	}
	else
	{
		IFCKSP(e, 0, 1);
		PUSH(e, i);
	}
	
	return ret;
}

#ifdef SF_64BIT
C(f_blit64)			/* b(lit64) (-- x1) 0x710 - vendor-specific */
{
	Long i = next_fcode_num64(e);
	
	if (e->istokenizing)
		return E_NUM_OUT_OF_RANGE;

	if (e->iscompiling)
		return compile_num64(e, i);

	IFCKSP(e, 0, 1);
	PUSH(e, i);
	return NO_ERROR;
}
#endif

C(f_bsquote)		/* b(') (-- xt) 0x11 */
{
	Int xt = next_fcode_num(e);

	if (e->iscompiling)
	{
		compile_fcode(e, 0x11);
		return compile_fcode(e, xt);
	}

	/* translate Fcode to xtok if necessary */
	if (xt < NUM_FCODES && e->fcodes[xt] != INVALID_FCODE)
		xt = e->fcodes[xt] & FCODE_MASK;

	if (xt < 0 || xt >= e->numxtoks)	/* sanity check */
		return E_ILLEGAL_FCODE;

	if (e->xtoks[xt] == NULL)
		return E_ILLEGAL_FCODE;

	IFCKSP(e, 0, 1);
	PUSH(e, xt);
	return NO_ERROR;
}

static Retcode
do_string(Environ *e, Byte *str)
{
	Retcode ret = NO_ERROR;

	if (e->istokenizing)
	{
		compile_fcode(e, 0x12);		/* b(") */
		ret = compile_str(e, str + 1, *(uByte*)str);
	}
	else if (e->iscompiling)
	{
		compile_fcode(e, 0x712);
		ret = compile_str(e, str + 1, *(uByte*)str);

		if (ret == NO_ERROR)
			ret = compile_byte(e, '\0');	/* null-terminate */
	}
	else
	{
		IFCKSP(e, 0, 2);
		PUSHP(e, str + 1);
		PUSH(e, *(uByte*)str);
	}
	
	return ret;
}

C(f_bstring)		/* b(string) (-- str len) 0x712 vendor-specific */
{
	Int len = next_fcode_byte(e);
	Byte *str = e->in.buf + e->in.loc - 1;
	e->in.loc += (len + 1) * e->in.fspread;

	return do_string(e, str);
}

C(f_bdquote)		/* b(") (-- str len) 0x12 */
{
	Byte *str;

	next_fcode_string(e, &str);

	if (!e->istokenizing && !e->iscompiling)
	{
		#ifndef NUM_STRING_BUFS
		#	define NUM_STRING_BUFS	2	/* OF spec says at least two bufs */
		#endif
		static Byte sbuf[NUM_STRING_BUFS][STR_SIZE];
		static unsigned int snum = 0;
		Byte *s = sbuf[snum++ % NUM_STRING_BUFS];
		
		memcpy(s, str, *(uByte*)str + 2);
		str = s;
	}

	return do_string(e, str);
}


/* 5.3.3.3 - Controlling values and defers */

C(f_behavior)		/* behavior (defer-xt -- contents-xt) 0xDE */
{
	Int xt;
	Entry *ent;
	
	IFCKSP(e, 1, 1);
	POP(e, xt);
	ent = e->xtoks[xt];
	
	if (ent->type != T_DEFER)
		return E_DEFER_ERROR;
	
	PUSH(e, ent->v.xtok);
	return NO_ERROR;
}

C(f_bto)			/* b(to) (params --) 0xC3 */
{
	Int fcode = next_fcode_num(e);
	
	if (e->iscompiling)
	{
		compile_fcode(e, 0xC3);			/* b(to) */
		return compile_fcode(e, fcode);
	}

	if (fcode < NUM_FCODES && e->fcodes[fcode] != INVALID_FCODE)
		fcode = e->fcodes[fcode] & FCODE_MASK;

	if (fcode < 0 || fcode >= e->numxtoks)
		return E_ILLEGAL_FCODE;

	if (e->xtoks[fcode] == NULL)
		return E_DEFER_ERROR;
	
	return do_to(e, e->xtoks[fcode]);
}


/* 5.3.3.4 - Control flow */

C(f_offset16)		/* offset16 (--) 0xCC */
{
	if (e->in.type != I_FCODE)
		return E_INPUT_ERROR;
	
	e->in.foffset = TRUE;
	return NO_ERROR;
}

C(f_bbranch)		/* bbranch (--) 0x13 */
{
	Int o = next_fcode_offset(e);
	
	if (e->iscompiling)
		return compile_branch(e, 0x13, o);

	fcode_branch(e, o - 2);
	return NO_ERROR;
}

C(f_begin_qbranch)	/* begin-?branch (continue? --) 0x714 - vendor-specific */
{
	Int o = next_fcode_offset(e);
	Cell c;

	IFCKSP(e, 1, 0);
	POP(e, c);
	
	if (!c)
		fcode_branch(e, o - 2);
	
	return NO_ERROR;
}

C(f_bqbranch)		/* b?branch (continue? --) 0x14 */
{
	Int o = next_fcode_offset(e);
	
	begin_temp_compile(e);
	return compile_branch(e, e->istokenizing ? 0x14 : 0x714, o);
}

C(f_begin_mark)		/* begin-mark (--) 0x701 - vendor-specific */
{
	/* this is a no-op marker to aid in compiling b(<mark) opcodes for
		tokenizing */
	return NO_ERROR;
}

CC(f_bltmark)		/* b(<mark) (--) 0xB1 */
{
	Retcode ret;
	
	IFCKSP(e, 0, 2);
	begin_temp_compile(e);

	/* begin-mark marker */
	ret = compile_fcode(e, e->istokenizing ? 0xB1 : 0x701);

	if (ret != NO_ERROR)
		return ret;

	PUSHP(e, e->fbrk);		/* setup for back-patching */
	PUSH(e, 0xB1);			/* b(<mark) */
	return NO_ERROR;
}

C(f_bgtresolve)		/* b(>resolve) (--) 0xB2 */
{
	if (e->iscompiling)
		return resolve_branch(e);
	
	return NO_ERROR;
}

C(f_bloop)			/* b(loop) (--) 0x15 */
{
	Int branch = next_fcode_offset(e);

	if (e->iscompiling)
		return compile_loop(e, 0x15);

	IFCKRETSP(e, LOOP_SIZE, 0);
	e->rsp[-LOOP_INDEX] += 1;

	if (RSTACK(e, LOOP_INDEX) >= RSTACK(e, LOOP_LIMIT))
		RDROPN(e, LOOP_SIZE);
	else
		fcode_branch(e, branch - 2);

	return NO_ERROR;
}

C(f_bplusloop)		/* b(+loop) (delta --) 0x16 */
{
	Int branch = next_fcode_offset(e);
	Cell delta;

	if (e->iscompiling)
		return compile_loop(e, 0x16);

	IFCKRETSP(e, LOOP_SIZE, 0);
	IFCKSP(e, 1, 0);
	POP(e, delta);
	DO_MASK32(e, delta);
	e->rsp[-LOOP_INDEX] += delta;

	if ((delta > 0 && RSTACK(e, LOOP_INDEX) >= RSTACK(e, LOOP_LIMIT)) ||
			(delta < 0 && RSTACK(e, LOOP_INDEX) < RSTACK(e, LOOP_LIMIT)))
		RDROPN(e, LOOP_SIZE);
	else
		fcode_branch(e, branch - 2);

	return NO_ERROR;
}

C(f_begin_do)		/* begin-do (limit start --) 0x717 - vendor-specific */
{
	Int branch = next_fcode_offset(e);
	Cell limit, index;
	
	IFCKSP(e, 2, 0);
	IFCKRETSP(e, 0, LOOP_SIZE);
	POP(e, index);
	POP(e, limit);
	DO_MASK32(e, index);
	DO_MASK32(e, limit);
	RPUSH(e, e->in.loc + branch);	/* change relative to absolute location */
	RPUSH(e, limit);
	RPUSH(e, index);
	return NO_ERROR;
}

C(f_bdo)			/* b(do) (limit start --) 0x17 */
{
	(void)next_fcode_offset(e);	/* ignored */
	begin_temp_compile(e);
	return compile_do(e, e->istokenizing ? 0x17 : 0x717);
}

C(f_begin_qdo)		/* begin-?do (limit start --) 0x718 - vendor-specific */
{
	Int branch = next_fcode_offset(e);
	Cell limit, index;
	
	IFCKSP(e, 2, 0);
	POP(e, index);
	POP(e, limit);
	DO_MASK32(e, index);
	DO_MASK32(e, limit);
	
	if (index == limit)
		fcode_branch(e, branch - 2);
	else
	{
		IFCKRETSP(e, 0, LOOP_SIZE);
		RPUSH(e, e->in.loc + branch); /* change relative to absolute location */
		RPUSH(e, limit);
		RPUSH(e, index);
	}
	
	return NO_ERROR;
}

C(f_bqdo)			/* b(?do) (limit start --) 0x18 */
{
	(void)next_fcode_offset(e);	/* ignored */
	begin_temp_compile(e);
	return compile_do(e, e->istokenizing ? 0x18 : 0x718);
}

C(f_i)				/* i (-- index) (R: sys -- sys) 0x19 */
{
	IFCKSP(e, 0, 1);
	IFCKRETSP(e, LOOP_SIZE, 0);
	PUSH(e, e->rsp[-LOOP_INDEX]);
	return NO_ERROR;
}

C(f_j)				/* j (-- index) (R: sys -- sys) 0x1A */
{
	IFCKSP(e, 0, 1);
	IFCKRETSP(e, 2 * LOOP_SIZE, 0);
	PUSH(e, e->rsp[-(LOOP_SIZE + LOOP_INDEX)]);
	return NO_ERROR;
}

C(f_unloop)			/* unloop (--) (R: sys --) 0x89 */
{
	IFCKRETSP(e, LOOP_SIZE, 0);
	RDROPN(e, LOOP_SIZE);
	return NO_ERROR;
}

CC(f_bleave)		/* b(leave) (--) 0x1B */
{
	Int branch;
	
	if (e->rsp <= e->rstack)
		return E_RSTACK_UNDERFLOW;
	
	IFCKRETSP(e, LOOP_SIZE, 0);
	RDROPN(e, LOOP_SIZE - 1);
	RPOP(e, branch);
	branch -= 2;

	/* change absolute back to relative offset */
	fcode_branch(e, branch - e->in.loc);
	return NO_ERROR;
}

C(f_begin_case)		/* begin-case (sel -- sel) 0x704 - vendor-specific */
{
	/* this is a no-op marker to aid in compiling b(case) opcodes when
		tokenizing */
	return NO_ERROR;
}

CC(f_bcase)			/* b(case) (sel -- sel) 0xC4 */
{
	Retcode ret;
	
	IFCKSP(e, 0, 2);
	begin_temp_compile(e);

	/* begin-case marker */
	ret = compile_fcode(e, e->istokenizing ? 0xC4 : 0x704);
	
	if (ret != NO_ERROR)
		return ret;
		
	PUSH(e, NULL);	/* end-of-list for back-patching b(endof) or b(endcase) */
	return NO_ERROR;
}

CC(f_bendcase)		/* b(endcase) (sel | <nothing> --) 0xC5 */
{
	if (e->iscompiling)
	{
		Byte *b;
		Retcode ret;
		
		IFCKSP(e, 1, 0);
		ret = compile_fcode(e, 0xC5);
		
		if (ret != NO_ERROR)
			return ret;
			
		POPT(e, b, Byte*);
		
		while (b != NULL)
		{
			/* back-patch each b(endof) to after b(endcase) */
			patch_offset(b, (Byte*)e->fbrk - (Byte*)b);
			POPT(e, b, Byte*);
		}
	
		return resolve_temp_compile(e);
	}
	
	/* if we get here, there was no matching "of", so drop the selector */
	IFCKSP(e, 1, 0);
	DROP(e);
	return NO_ERROR;
}

C(f_bof)			/* b(of) (sel of-val -- sel | <nothing> --) 0x1C */
{
	Int o = next_fcode_offset(e);
	
	if (e->iscompiling)
		return compile_of(e);
	
	IFCKSP(e, 2, 0);

	if (TOP(e) != STACK(e, 1))
	{
		DROP(e);
		fcode_branch(e, o - 2);
	}
	else
	{
		DROP(e);
		DROP(e);
	}
	
	return NO_ERROR;
}

C(f_bendof)			/* b(endof) (--) 0xC6 */
{
	Int o = next_fcode_offset(e);

	if (e->iscompiling)
		return compile_endof(e);

	fcode_branch(e, o - 2);
	return NO_ERROR;
}


/* 5.3.7.6  Start and end */

static Retcode
do_fcode_header(Environ *e)
{
	Int checksum;
	Int len;
	Int format = next_fcode_byte(e);

	if ((format != 0x08) && (format != 3))	/* magic Fcode value */
		return E_BAD_FCODE_FORMAT;
	
	checksum = (uByte)next_fcode_byte(e) << BYTE_SIZE;
	checksum |= (uByte)next_fcode_byte(e);
	/* may not be easy to check the checksum if fnext is 0, so pretend it is OK */
	
	len = (uByte)next_fcode_byte(e);
	len = (len << BYTE_SIZE) | (uByte)next_fcode_byte(e);
	len = (len << BYTE_SIZE) | (uByte)next_fcode_byte(e);
	len = (len << BYTE_SIZE) | (uByte)next_fcode_byte(e);
	
	e->in.len = len;
	return NO_ERROR;
}

C(f_start0)			/* start0 (--) 0xF0 */
{
	e->in.fspread = 0;
	e->in.foffset = TRUE;	/* 16 */
	return do_fcode_header(e);
}

C(f_start1)			/* start1 (--) 0xF1 */
{
	e->in.fspread = 1;
	e->in.foffset = TRUE;	/* 16 */
	return do_fcode_header(e);
}

C(f_start2)			/* start2 (--) 0xF2 */
{
	e->in.fspread = 2;
	e->in.foffset = TRUE;	/* 16 */
	return do_fcode_header(e);
}

C(f_start4)			/* start4 (--) 0xF3 */
{
	e->in.fspread = 4;
	e->in.foffset = TRUE;	/* 16 */
	return do_fcode_header(e);
}

C(f_version1)			/* version1 (--) 0xFD */
{
	e->in.fspread = 1;
	e->in.foffset = FALSE;	/* 8 */
	return do_fcode_header(e);
}

C(f_end)			/* end0 (--) 0x00; end1 (--) 0xFF */
{
	return R_END;
}

C(f_suspend_fcode)	/* suspend-fcode (--) 0x215 */
{
	/* resuming execution after suspension is tricky, so we just
	   punt and continue execution - the spec does not say that we have
	   to suspend execution, only that we may choose to do so -
	   we do not so choose
	 */

	return NO_ERROR;
}

CC(f_byte_load)		/* byte-load (addr xt --) 0x23E */
{
	Int fnext;
	Byte *fcode;
	Retcode ret;
	
	IFCKSP(e, 2, 0);
	POP(e, fnext);
	POPT(e, fcode, Byte*);
	e->fcode32 = TRUE;
	ret = interp_fcode(e, fcode, fnext);
	e->fcode32 = FALSE;
	return ret;
}


const Initentry init_control[] =
{
	{ "eval", f_evaluate, 0xCD, F_NONE, T_FUNC 			/* synonym */
			HELP("(... str len -- ?) synonym for evaluate") },
	{ "evaluate", f_evaluate, 0xCD, F_NONE, T_FUNC
			HELP("(... str len -- ?) execute Forth text from a string") },
	{ "execute", f_execute, 0x1D, F_NONE, T_FUNC
			HELP("(... xt -- ?) " \
					"execute command whose execution token is xt") },
	{ "exit", f_exit, 0x33, F_NONE, T_FUNC
			HELP("(--) (R: sys --) " \
					"exit from the currently executing command") },
	{ "abort", f_abort, 0x216, F_NONE, T_FUNC
			HELP("(... --) (R: ... --) " \
					"abort program execution; clear stacks") },
	{ "catch", f_catch, 0x217, F_NONE, T_FUNC
			HELP("(... xt -- ? error-code | ? false)\n" \
					"\texecute command xt; return throw result") },
	{ "throw", f_throw, 0x218, F_NONE, T_FUNC
			HELP("(... error-code -- ? error-code | ...)\n" \
					"\ttransfer back to catch if error-code is non-zero") },
	{ "here", f_here, 0xAD, F_NONE, T_FUNC
			HELP("(-- addr) return current dictionary pointer") },
	{ "c,", f_ccomma, 0xD0, F_NONE, T_FUNC
			HELP("(byte --) compile a byte into the dictionary") },
	{ ",", f_comma, 0xD3, F_NONE, T_FUNC
			HELP("(x --) append x to data space") },
	{ "compile,", f_compilecomma, 0xDD, F_NONE, T_FUNC
			HELP("(xt --) compile the behavior of the word given by xt") },
	{ "state", (Command)offsetof(Environ, comp_state), 0xDC, F_NONE, T_ADDR
			HELP("(-- a-addr) " \
					"variable containing true if in compilation state") },
	{ ">body", f_tobody, 0x86, F_NONE, T_FUNC
			HELP("(xt -- a-addr) convert execution token to data field addr") },
	{ "body>", f_frombody, 0x85, F_NONE, T_FUNC
			HELP("(a-addr -- xt) convert data field addr to execution token") },
	{ "noop", f_noop, 0x7B, F_NONE, T_FUNC
			HELP("(--) no operation - do nothing gracefully") },

	{ "instance", f_instance, 0xC0, F_NONE, T_FUNC
			HELP("(--) mark next defining word as instance-specific") },
	{ "new-token", f_newtoken, 0xB5, F_NONE, T_FUNC
			HELP("(F:/FCode#/ --) create a new unnamed FCode function") },
	{ "named-token", f_namedtoken, 0xB6, F_NONE, T_FUNC
			HELP("(F:/FCode-string FCode#/ --)\n" \
					"\tcreate a new possibly named FCode function") },
	{ "external-token", f_externaltoken, 0xCA, F_NONE, T_FUNC
			HELP("(F:/FCode-string FCode#/ --) " \
					"create a new named FCode function") },
	{ "b(;)", f_bsemicolon, 0xC2, F_IMMEDIATE, T_FUNC
			HELP("(--) end an FCode colon definition") },
	{ "b(:)", f_bcolon, 0xB7, F_IMMEDIATE, T_FUNC
			HELP("(--) (E:... -- ?) " \
					"define type of new FCode function as colon definition") },
	{ "b(buffer:)", f_bbuffercolon, 0xBD, F_NONE, T_FUNC
			HELP("(size --) (E: -- a-addr)\n" \
					"\tdefine type of new FCode function as buffer:") },
	{ "b(constant)", f_bconstant, 0xBA, F_NONE, T_FUNC
			HELP("(n1 --) (E:-- n1) " \
					"define type of new FCode function as constant") },
	{ "b(create)", f_bcreate, 0xBB, F_NONE, T_FUNC
			HELP("(--) (E:-- a-addr)\n" \
					"\tdefine type of new FCode function as create word") },
	{ "b(defer)", f_bdefer, 0xBC, F_NONE, T_FUNC
			HELP("(--) (E:... -- ?) " \
					"define type of new FCode function as defer word") },
	{ "b(field)", f_bfield, 0xBE, F_NONE, T_FUNC
			HELP("(offset size -- offset+size) (E:addr -- addr+offset)\n" \
					"\tdefine type of new FCode function as field") },
	{ "b(value)", f_bvalue, 0xB8, F_NONE, T_FUNC
			HELP("(x --) (E: -- x) " \
					"define type of new FCode function as value") },
	{ "b(variable)", f_bvariable, 0xB9, F_NONE, T_FUNC
			HELP("(--) (E: -- a-addr) " \
					"define type of new FCode function as variable") },
	{ "(is-user-word)", f_isuserword, 0x214, F_NONE, T_FUNC
			HELP("(name-str name-len xt --) (E:... -- ?)\n" \
					"\tcreate a new named user-interface command") },
	{ "get-token", f_gettoken, 0xDA, F_NONE, T_FUNC
			HELP("(fcode# -- xt immediate?) "  \
					"convert FCode number to execution token") },
	{ "set-token", f_settoken, 0xDB, F_NONE, T_FUNC
			HELP("(xt immediate? fcode# --) " \
					"assign FCode number to existing function") },
	{ "b(lit)", f_blit, 0x10, F_IMMEDIATE, T_FUNC
			HELP("(-- n1) numeric literal FCode, followed by fcode-num32") },
#ifdef SF_64BIT
	{ "b(lit64)", f_blit64, 0x710, F_IMMEDIATE, T_FUNC
			HELP("(-- x1) numeric literal FCode, followed by fcode-num64") },
#endif
	{ "b(')", f_bsquote, 0x11, F_IMMEDIATE, T_FUNC
			HELP("(-- xt) function literal FCode, followed by FCode#") },
	{ "b(string)", f_bstring, 0x712, F_IMMEDIATE, T_FUNC
			HELP("(-- str len) " \
				"compiled b(\") FCode, followed by FCode-string-ptr") },
			/* compiled b(") */
	{ "b(\")", f_bdquote, 0x12, F_IMMEDIATE, T_FUNC
			HELP("(-- str len) " \
				"string literal FCode, followed by FCode-string") },
	{ "behavior", f_behavior, 0xDE, F_NONE, T_FUNC
			HELP("(defer-xt -- contents-xt) " \
					"get execution token of defer word") },
	{ "b(to)", f_bto, 0xC3, F_IMMEDIATE, T_FUNC
			HELP("(params --) " \
			"FCode for setting value and defer words, followed by Fcode#") },
	{ "offset16", f_offset16, 0xCC, F_NONE, T_FUNC
			HELP("(--) make subsequent FCode-offsets 16-bits (not 8-bits)") },
	{ "bbranch", f_bbranch, 0x13, F_IMMEDIATE, T_FUNC
			HELP("(--) unconditional branch FCode, followed by FCode-offset") },
	{ "begin ?branch", f_begin_qbranch, 0x714, F_NONE, T_FUNC
			HELP("(continue? --) " \
					"compiled b?branch Fcode, followed by FCode-offset") },
			/* compiled b?branch */
	{ "b?branch", f_bqbranch, 0x14, F_IMMEDIATE, T_FUNC
			HELP("(continue? --) " \
					"conditional branch FCode, followed by FCode-offset") },
	{ "begin mark", f_begin_mark, 0x701, F_NONE, T_FUNC
			HELP(" (--) compiled b(<mark) FCode,") },
			/* compiled b(<mark) */
	{ "b(<mark)", f_bltmark, 0xB1, F_IMMEDIATE, T_FUNC
			HELP(" (--) FCode target of backward branches") },
	{ "b(>resolve)", f_bgtresolve, 0xB2, F_IMMEDIATE, T_FUNC
			HELP("(--) FCode target of forward branches") },
	{ "b(loop)", f_bloop, 0x15, F_IMMEDIATE, T_FUNC
			HELP("(--) end FCode do...loop, followed by FCode-offset") },
	{ "b(+loop)", f_bplusloop, 0x16, F_IMMEDIATE, T_FUNC
			HELP("(delta --) end FCode do...+loop, followed by FCode-offset") },
	{ "begin do", f_begin_do, 0x717, F_NONE, T_FUNC
			HELP("(limit start --) " \
					"compiled b(do) FCode, followed by FCode-offset") },
			/* compiled b(do) */
	{ "b(do)", f_bdo, 0x17, F_IMMEDIATE, T_FUNC
			HELP("(limit start --) " \
					"begin FCode doo..loop, followed by FCode-offset") },
	{ "begin ?do", f_begin_qdo, 0x718, F_NONE, T_FUNC
			HELP("(limit start --) " \
					"compiled b(?do) FCode, followed by FCode-offset") },
			/* compiled b(?do) */
	{ "b(?do)", f_bqdo, 0x18, F_IMMEDIATE, T_FUNC
			HELP("(limit start --) " \
					"begin FCode ?do...loop, followed by FCode-offset") },
	{ "i", f_i, 0x19, F_NONE, T_FUNC
			HELP("(-- index) (R: sys -- sys) return current loop index") },
	{ "j", f_j, 0x1A, F_NONE, T_FUNC
			HELP("(-- index) (R: sys -- sys) return next outer loop index") },
	{ "unloop", f_unloop, 0x89, F_NONE, T_FUNC
			HELP("(--) (R: sys --) discard loop control parameters") },
	{ "b(leave)", f_bleave, 0x1B, F_NONE, T_FUNC
			HELP("(--) FCode to exit from do...loop") },
	{ "begin case", f_begin_case, 0x704, F_NONE, T_FUNC
			HELP("(sel -- sel) compiled b(case) FCode") },
			/* compiled b(case) */
	{ "b(case)", f_bcase, 0xC4, F_IMMEDIATE, T_FUNC
			HELP("(sel -- sel) FCode to begin a case statement") },
	{ "b(endcase)", f_bendcase, 0xC5, F_IMMEDIATE, T_FUNC
			HELP("(sel | <nothing> --) FCode to end a case statement") },
	{ "b(of)", f_bof, 0x1C, F_IMMEDIATE, T_FUNC
			HELP("(sel of-val -- sel | <nothing> --)\n" \
			"\tFCode for of in case statement, followed by FCode-offset") },
	{ "b(endof)", f_bendof, 0xC6, F_IMMEDIATE, T_FUNC
			HELP("b(endof) (--) " \
			"FCode for endof in case statement, followed by FCode-offset") },

	{ "start0", f_start0, 0xF0, F_NONE, T_FUNC
			HELP("(--) begin FCode program with spread zero, 16-bit offsets") },
	{ "start1", f_start1, 0xF1, F_NONE, T_FUNC
			HELP("(--) begin FCode program with spread one, 16-bit offsets") },
	{ "start2", f_start2, 0xF2, F_NONE, T_FUNC
			HELP("(--) begin FCode program with spread two, 16-bit offsets") },
	{ "start4", f_start4, 0xF3, F_NONE, T_FUNC
			HELP("(--) begin FCode program with spread four, 16-bit offsets") },
	{ "version1", f_version1, 0xFD, F_NONE, T_FUNC
			HELP("(--) begin FCode program with spread one, 8-bit offsets") },
	{ "end0", f_end, 0x00, F_NONE, T_FUNC
			HELP("(--) end of FCode program") },
	{ "end1", f_end, 0xFF, F_NONE, T_FUNC
			HELP("(--) end of FCode program") },
	{ "suspend-fcode", f_suspend_fcode, 0x215, F_NONE, T_FUNC
			HELP("(--) suspend interpreting an FCode program") },
	{ "byte-load", f_byte_load, 0x23E, F_NONE, T_FUNC
			HELP("(addr fnext-xt --) execute FCode beginning at addr") },

	{ NULL, NULL }
};
