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

#include "stdlib.h"
#include "defs.h"

#ifdef SUN_COMPATIBILITY
#include "exe.h"
#endif


Retcode g_hwfault = NO_ERROR;


static Retcode do_eval(Environ *e, Entry *code, Bool imm);
static Retcode eval(Environ *e, Entry *code, Bool imm);

static Int g_eval_count = 0;

#ifndef EVAL_ALARM_RUN
#    define EVAL_ALARM_RUN	32
#endif

/* this is to speed up calls to eval if there are no alarms set */
#define EVAL(e, code, imm)	\
	((e->inalarm || e->numalarms == 0 || g_eval_count++ < EVAL_ALARM_RUN) ?	\
		do_eval(e, code, imm) : eval(e, code, imm))

/* this speeds up executing xtoks */
#define EXEC_XTOK(e, xtok)	\
	((xtok < 0 || xtok >= e->numxtoks) ? E_ILLEGAL_XTOK :	\
		EVAL(e, e->xtoks[xtok], FALSE))

/* this speeds up reading Fcode bytes for the typical case */
#define NEXT_FCODE(e, b)	\
	((e->in.type == I_FCODE && e->in.fnext == MAGIC_FNEXT) ?	\
		(b = (uByte)e->in.buf[e->in.loc]), e->in.loc += e->in.fspread :	\
		(b = next_fcode_byte(e)) )


Retcode
compile_byte(Environ *e, Byte b)
{
	Byte *mem = falloc(e, sizeof (Byte));

	if (mem == NULL)
		return E_OUT_OF_MEMORY;

	*mem = b;
	return NO_ERROR;
}

Retcode
compile_fcode(Environ *e, Int code)
{
	Retcode ret = NO_ERROR;

	/* map fcode to xtok if defined */
	if (code >= 0 && code < NUM_FCODES &&
			e->fcodes[code] != INVALID_FCODE)
		code = e->fcodes[code] & FCODE_MASK;

	if (code >= 0x00 && code < 0x100)
	{
		ret = compile_byte(e, (uByte)code);
	}
	else if (code >= 0x100 && code < 0x1000)
	{
		ret = compile_byte(e, code >> BYTE_SIZE);

		if (ret == NO_ERROR)
			ret = compile_byte(e, code & BYTE_MASK);
	}
	else if (code >= 0x1000)
	{
		int i;

		/* if tokenizing, then there must not be any reference to fcodes
		   within this range */
		if (e->istokenizing)
			return E_ILLEGAL_FCODE;

		ret = compile_byte(e, 0x06);		/* vendor-specific fcode */

		if (code > 0x3FFFFF)
		{
			/* use MSBs as a marker for 4-byte xtoks */
			i = sizeof (Int) - 1;
			ret = compile_byte(e, (uByte)(code >> (BYTE_SIZE * i--)) | 0xC0);

			while (ret == NO_ERROR && i >= 0)
				ret = compile_byte(e, (uByte)(code >> (BYTE_SIZE * i--)));
		}
		else if (code > 0x7FFF)
		{
			/* use MSBs as a marker for 3-byte xtoks */
			i = sizeof (Int) - 2;
			ret = compile_byte(e, (uByte)(code >> (BYTE_SIZE * i--)) | 0x80);

			while (ret == NO_ERROR && i >= 0)
				ret = compile_byte(e, (uByte)(code >> (BYTE_SIZE * i--)));
		}
		else
		{
			/* otherwise use smaller 16-bit encoding */
			ret = compile_byte(e, code >> BYTE_SIZE);

			if (ret == NO_ERROR)
				ret = compile_byte(e, code & BYTE_MASK);
		}
	}
	else
		ret = E_ILLEGAL_FCODE;
	
	return ret;
}

Retcode
compile_offset(Environ *e, Int o)
{
	Retcode ret;

	if (o > 32767 || o < -32768)
		return E_OFFSET_RANGE;

	ret = compile_byte(e, (uByte)(o >> BYTE_SIZE));

	if (ret == NO_ERROR)
		ret = compile_byte(e, (uByte)(o & BYTE_MASK));

	return ret;
}

void
patch_offset(Byte *ptr, Int o)
{
	if (ptr == NULL)
		return;
		
	if (o > 32767 || o < -32768)
		error(E_OFFSET_RANGE);
		
	*ptr++ = (uByte)(o >> BYTE_SIZE);
	*ptr = (uByte)(o & BYTE_MASK);
}

Retcode
compile_num32(Environ *e, Int o)
{
	Retcode ret = NO_ERROR;
	Int i;

	for (i = sizeof (Int) - 1; ret == NO_ERROR && i >= 0; i--)
		ret = compile_byte(e, (uByte)(o >> (BYTE_SIZE * i)));
	
	return ret;
}

#ifdef SF_64BIT
Retcode
compile_n64(Environ *e, Cell o)
{
	Retcode ret = NO_ERROR;
	Int i;

	if (e->istokenizing)
		return E_NUM_OUT_OF_RANGE;

	for (i = sizeof (Cell) - 1; ret == NO_ERROR && i >= 0; i--)
		ret = compile_byte(e, (uByte)(o >> (BYTE_SIZE * i)));

	return ret;
}

Retcode
compile_num64(Environ *e, Cell o)
{
	Retcode ret;

	if ((o >= 0 && (o >> (QUADLET_SIZE - 1) == 0)) ||
			(o < 0 && (-o >> QUADLET_SIZE) == 0))
	{
		DPRINTF(("compile_num64: compiling as 32: %Cx (%Cx) mask=%Cx: %Cx\n",
				o, -o, ~(Cell)QUADLET_MASK,
				-o & ~(Cell)QUADLET_MASK));
		ret = compile_fcode(e, 0x10);				/* b(lit) */

		if (ret == NO_ERROR)
			ret = compile_num32(e, (Int)o);

		return ret;
	}

	if (e->istokenizing)
		return E_NUM_OUT_OF_RANGE;

	DPRINTF(("compile_num64: compiling as 64: %Cx\n", o));
	ret = compile_fcode(e, 0x710);				/* b(lit64) */

	if (ret == NO_ERROR)
		ret = compile_n64(e, o);

	return ret;
}
#endif

/* not standard, but useful for in-memory use such as for strings */
Retcode
compile_pointer(Environ *e, Byte *ptr)
{
#ifdef SF_64BIT
	if (sizeof (uLong) == sizeof (uPtr))
		return compile_n64(e, (uPtr)ptr);
#endif

	return compile_num32(e, (uPtr)ptr);
}

Retcode
compile_str(Environ *e, Byte *str, Int len)
{
	Retcode ret;

	setstrlen(&str, &len);	
	ret = compile_byte(e, (uByte)len);
	
	while (len-- && ret == NO_ERROR)
		ret = compile_byte(e, *str++);

	return ret;
}

void
begin_temp_compile(Environ *e)
{
	if (e->iscompiling)
		return;
	
	e->tempsp = e->sp;
	e->istemp = TRUE;
	e->tempbrk = e->fbrk;
	e->comp_state = FTRUE;
	e->iscompiling = TRUE;
}

Retcode
resolve_temp_compile(Environ *e)
{
	static int tcount = 0;
	static Byte *tstart = NULL;
	Int len;
	Byte *buf;
	Retcode ret;

	if (!e->iscompiling || !e->istemp || e->sp != e->tempsp)
		return NO_ERROR;

	/* temp definitions get executed immediately */
	len = e->fbrk - e->tempbrk;
	buf = (Byte*)malloc(len * sizeof (Byte));

	if (buf == NULL)
		return E_OUT_OF_MEMORY;

	/* free memory from previous temp-compile, but only if no
	   other temp-compiles are currently executing */
	if (tcount++ == 0 && tstart != NULL)
		free(tstart);

	memcpy(buf, e->tempbrk, len);
	e->comp_state = FFALSE;
	e->iscompiling = FALSE;
	e->istemp = FALSE;
	e->fbrk = e->tempbrk;

	ret = execute(e, buf, len);

	/* do not free temp memory here so strings can still be
	   accessed, at least for a little while */

	if (--tcount == 0)
		tstart = buf;
	else
		free(buf);

	return ret;
}

Retcode
compile_branch(Environ *e, Int code, Int o)
{
	Retcode ret;

	if (!e->iscompiling)
		return E_COMPILATION_ERROR;
		
	ret = compile_fcode(e, code);

	if (ret != NO_ERROR)
		return ret;

	if (o < 0)
	{
		Int code;
		Byte *brk;

		IFCKSP(e, 2, 0);
		
		if (TOP(e) != 0xB1)			/* b(<mark) */
		{
			/* probably a do-while-repeat, so our b(<mark) should be
				underneath this b?branch */
			Int savcode;
			Byte *savbrk;
			
			IFCKSP(e, 4, 2);
			POP(e, savcode);
			POPT(e, savbrk, Byte*);
			POP(e, code);
			POPT(e, brk, Byte*);
			PUSHP(e, savbrk);
			PUSH(e, savcode);
		}
		else
		{
			POP(e, code);
			POPT(e, brk, Byte*);
		}
		
		if (code != 0xB1)		/* b(<mark) */
			return E_COMPILATION_ERROR;
		
		/* -offset to dest */
		ret = compile_offset(e, (Int)(Ptr)((Byte*)brk - (Byte*)e->fbrk));

		if (ret == NO_ERROR)
			ret = resolve_temp_compile(e);
	}
	else
	{
		IFCKSP(e, 0, 2);
		PUSHP(e, e->fbrk);		/* place to back-patch later */
		PUSH(e, code);		/* code to verify we will patch the right thing */
		ret = compile_offset(e, 0);	/* need to back-patch this at b(>resolve) */
	}
	
	return ret;
}

Retcode
resolve_branch(Environ *e)
{
	Byte *brk;
	Int code, offset;
	Retcode ret;
	
	if (!e->iscompiling)
		return E_COMPILATION_ERROR;
		
	IFCKSP(e, 2, 0);
	POP(e, code);
	POPT(e, brk, Byte*);
	
	/* if the previous compiled fcode was a "bbranch", assume it is the "else"
	   clause of an "if-then" as this "b(>resolve)" resolves the earlier
	   "b?branch" */
	if (e->fbrk[-3] == 0x13 && e->fbrk[-2] == 0 && e->fbrk[-1] == 0)
	{
		/* swap the two saved patch values on the stack and their codes */
		Int savcode;
		Byte *savbrk;
		
		IFCKSP(e, 2, 2);
		POP(e, savcode);
		POPT(e, savbrk, Byte*);
		PUSHP(e, brk);
		PUSH(e, code);
		brk = savbrk;
		code = savcode;
	}
	
	ret = compile_fcode(e, 0xB2);		/* b(>resolve) */

	if (ret != NO_ERROR)
		return ret;

	/* back-patch branch to come here */
	offset = (Byte*)e->fbrk - (Byte*)brk;
	patch_offset(brk, offset);
	return resolve_temp_compile(e);
}

Retcode
compile_of(Environ *e)
{
	Retcode ret;

	if (!e->iscompiling)
		return E_COMPILATION_ERROR;
		
	IFCKSP(e, 0, 1);
	ret = compile_fcode(e, 0x1C);		/* b(of) */
	PUSHP(e, e->fbrk);			/* needs to be back-patched by "endof" */

	if (ret == NO_ERROR)
		ret = compile_offset(e, 0);

	return ret;
}

Retcode
compile_endof(Environ *e)
{
	Byte *ofp;
	Retcode ret;

	if (!e->iscompiling)
		return E_COMPILATION_ERROR;
		
	IFCKSP(e, 2, 1);
	POPT(e, ofp, Byte*);		/* previous "of" */
	/* leave any preceding "endof" on the stack for endcase to patch */

	ret = compile_fcode(e, 0xC6);		/* b(endof) */
	PUSHP(e, e->fbrk);	/* back-patched later by next "endof" or "endcase" */

	if (ret == NO_ERROR)
		ret = compile_offset(e, 0);

	if (ofp != NULL && ret == NO_ERROR)
		/* back-patch prev "of" to after this "endof" */
		patch_offset(ofp, (Byte*)e->fbrk - (Byte*)ofp);
	
	return ret;
}

Retcode
compile_do(Environ *e, Int code)
{
	Retcode ret;

	IFCKSP(e, 0, 2);
	begin_temp_compile(e);
	ret = compile_fcode(e, code);
	PUSHP(e, e->fbrk);

	if (ret == NO_ERROR)
		ret = compile_offset(e, 0);	/* need to back-patch this at b(loop) */

	return ret;
}

Retcode
compile_loop(Environ *e, Int code)
{
	Byte *b;
	Int o;
	Retcode ret;

	if (!e->iscompiling)
		return E_COMPILATION_ERROR;
		
	IFCKSP(e, 1, 0);
	ret = compile_fcode(e, code);

	if (ret != NO_ERROR)
		return ret;

	POPT(e, b, Byte*);			/* pointer to beginning of "do" loop */
	o = (Byte*)e->fbrk - (Byte*)b;
	ret = compile_offset(e, -o + 2);		/* back to beginning */

	if (ret != NO_ERROR)
		return ret;

	patch_offset(b, o + 2);			/* point "do" to after end of loop */
	return resolve_temp_compile(e);
}

Retcode
create_newdef(Environ *e)
{
	Table *table;
	Entry *ent;

	/* we may have already inserted this into a table by "recursive" */
	ent = lookup_sym(e, e->newdef->name, PSTR);
	
	if (ent == e->newdef)
		return NO_ERROR;

	/* make sure we have an xtok for it so we can immediately compile it */
	if (e->newdef->xtok == INVALID_XTOK)
		set_xtok(e, e->newdef);
	else		/* make sure fcode table is correctly set */
		e->xtoks[e->newdef->xtok] = e->newdef;

	/* definitions created in 32-bit compatibility mode retain it */
	if (e->fcode32)
		e->newdef->flags |= F_FCODE32;
	
	/* figure out which table to insert this new definition into */
	if (e->instance && e->currinst &&
			e->newdef->type != T_FCODE &&
			e->newdef->type != T_FORTH &&
			e->newdef->type != T_EXECTOKEN &&
			e->newdef->type != T_FUNC &&
			e->newdef->type != T_CODE &&
			e->newdef->type != T_LABEL &&
			e->newdef->type != T_REGISTER)
	{
		/* instances are not allowed to contain "executable" words */
		Instance *inst = (Instance*)(uPtr)e->currinst;

		if (inst->dict == NULL)
			inst->dict = new_table();
		
		table = inst->dict;
		e->newdef->flags |= F_INSTANCE;
		e->newdef->offset = table->num;
	}
	else if (e->currpkg)
	{
		if (e->currpkg->dict == NULL)
			e->currpkg->dict = new_table();
		
		table = e->currpkg->dict;
	}
	else
		table = e->names;
	
	e->instance = FALSE;

	/* warn user about overriding an existing word */
	ent = find_table(table, e->newdef->name, PSTR);

	if (ent && e->in.type != I_FCODE)
		cprintf(e, "warning: \"%P\" isn't unique\n", ent->name);

	/* and finally stick it in the table */
	if (!insert_table(table, e->newdef))
		return E_TABLE_ERROR;
	
	return NO_ERROR;
}

Retcode
do_to(Environ *e, Entry *ent)
{
	IFCKSP(e, 1, 0);

	/* ent may not point to the right instance so look it up by offset */
	if (ent->flags & F_INSTANCE)
	{
		Instance *inst = (Instance*)(uPtr)e->currinst;

		if (inst && inst->dict)
		{
			Entry *nent = inst->dict->list;
			Int i = inst->dict->num - ent->offset - 1;

			while (nent && i-- > 0)
				nent = nent->link;

			if (nent)
				ent = nent;
		}
	}
	
	switch (ent->type)
	{
#ifdef SUN_COMPATIBILITY
	/* allow changing constants and variables */
	case T_VAR:
	case T_CONST:
#endif
	case T_VALUE:
		POP(e, ent->v.val);
		return NO_ERROR;
	
	case T_DEFER:
		POP(e, ent->v.xtok);
		
		if (ent->v.xtok < NUM_FCODES && e->fcodes[ent->v.xtok] != INVALID_FCODE)
			ent->v.xtok = e->fcodes[ent->v.xtok] & FCODE_MASK;

		if (ent->v.xtok < 0 || ent->v.xtok >= e->numxtoks)
			return E_ILLEGAL_FCODE;

		return NO_ERROR;
	
#ifdef SUN_COMPATIBILITY
	case T_ADDR:
#endif
	case T_ADDRVAL:
		POP(e, *(Cell*)ent->v.addr);
		return NO_ERROR;

	case T_REGISTER:
#ifdef CPU_REGISTERS
		/* set a CPU-register */
		{
			Cell v;

			POP(e, v);
			return cpu_register_write(e, ent->v.val, v);
		}
#else
		cprintf(e, "do_to: CPU register support not available\n");
		return E_ABORT;
#endif
	}

#ifdef SUN_COMPATIBILITY
	/* allow setting integer nvram parameters using "to" */
	if (get_config(e, ent->name, PSTR) != NULL)
	{
		Cell val;
		Byte buf[64];

		POP(e, val);
		bprintf(buf, "%Cd", val);
		DPRINTF(("to %P: %Cd \"%s\"\n", ent->name, val, buf));
		return save_config(e, ent->name, PSTR, buf, strlen(buf), TRUE);
	}
#endif
	
	return E_DEFER_ERROR;
}

Retcode
set_defer(Environ *e, char *name, char *func)
{
	Entry *ent = find_table(e->names, name, CSTR);
	Entry *f;
	
	if (ent == NULL || ent->type != T_DEFER)
		return E_BAD_SYMBOL;
	
	f = lookup_sym(e, func, CSTR);
	
	if (f == NULL)
		return E_BAD_SYMBOL;
	
	ent->v.xtok = f->xtok;
	return NO_ERROR;
}

Int
next_fcode_byte(Environ *e)
{
	Int byte;
	
	if (e->in.type != I_FCODE)
	{
		error(E_INPUT_ERROR);
		return 0;
	}
	
	if (e->in.fnext != MAGIC_FNEXT)		/* magic value specified in standard */
	{
		PUSHP(e, e->in.buf + e->in.loc);
		(void)EXEC_XTOK(e, e->in.fnext);		/* ignore errors for now */
		POP(e, byte);
	}
	else
		byte = (uByte)e->in.buf[e->in.loc];
	
	e->in.loc += e->in.fspread;
	return byte;
}

Int
next_fcode_num(Environ *e)
{
	Int f, b;

	NEXT_FCODE(e, f);
	
	if (f >= 0x01 && f <= 0x0F)
	{
		if (f == 0x06)	/* vendor-specific range used for in-memory xtok */
		{
			int i;

			NEXT_FCODE(e, f);

			/* marker for 4-byte encoded xtoks > 0x3FFFFF */
			if ((f & 0xC0) == 0xC0)		/* both bits must be on */
			{
				f &= 0x3F;	/* clear marker */

				for (i = 0; i < sizeof (Int) - 1; i++)
				{
					NEXT_FCODE(e, b);
					f = (f << BYTE_SIZE) | (uByte)b;
				}
			}
			if (f & 0x80)	/* marker for 3-byte encoded xtoks > 0x7FFF */
			{
				f &= 0x7F;	/* clear marker */

				for (i = 0; i < sizeof (Int) - 2; i++)
				{
					NEXT_FCODE(e, b);
					f = (f << BYTE_SIZE) | (uByte)b;
				}
			}
			else
			{
				/* 16-bit encoding */
				NEXT_FCODE(e, b);
				f = (f << BYTE_SIZE) | b;
			}
		}
		else
		{
			/* standard IEEE-1275 encoding */
			NEXT_FCODE(e, b);
			f = (f << BYTE_SIZE) | b;
		}
	}
	
	return f;
}

Int
next_fcode_offset(Environ *e)
{
	Short o;
	Int b;
	
	if (e->in.type != I_FCODE)
	{
		error(E_INPUT_ERROR);
		return 0;
	}
	
	if (e->in.foffset)
	{
		NEXT_FCODE(e, b);
		o = b & BYTE_MASK;
		NEXT_FCODE(e, b);
		o = (o << BYTE_SIZE) | (b & BYTE_MASK);
	}
	else
	{
		NEXT_FCODE(e, o);
	}
	
	return o;
}

Int
next_fcode_num32(Environ *e)
{
	int i;
	Int n, b;
	
	NEXT_FCODE(e, n);

	for (i = 0; i < sizeof (Int) - 1; i++)
	{
		NEXT_FCODE(e, b);
		n = (n << BYTE_SIZE) | (uByte)b;
	}

	return n;
}

#ifdef SF_64BIT
Cell
next_fcode_num64(Environ *e)
{
	int i;
	Cell n, b;
	
	NEXT_FCODE(e, n);

	for (i = 0; i < sizeof (Cell) - 1; i++)
	{
		NEXT_FCODE(e, b);
		n = (n << BYTE_SIZE) | (uByte)b;
	}

	return n;
}
#endif

/* not standard, but useful for in-memory items such as strings */
Byte *
next_fcode_pointer(Environ *e)
{	
#ifdef SF_64BIT
	if (sizeof (uLong) == sizeof (uPtr))
		return (Byte*)(uPtr)next_fcode_num64(e);
#endif

	return (Byte*)(uPtr)next_fcode_num32(e);
}

void
next_fcode_string(Environ *e, Byte **str)
{
	static Byte wbuf[STR_SIZE];
	Byte *buf = wbuf;
	Int len, b;

	NEXT_FCODE(e, len);
	*str = buf;
	*buf++ = (uByte)len;

	while (len-- > 0)
	{
		NEXT_FCODE(e, b);
		*buf++ = b;
	}
	
	*buf = '\0';
}

void
fcode_branch(Environ *e, Int o)
{
	if (e->in.type != I_FCODE)
	{
		error(E_INPUT_ERROR);
		return;
	}

	e->in.loc += o * e->in.fspread;
}

static void
push_pointer(Environ *e, Byte *addr, Bool fcode32)
{
#if defined SF_64BIT && !defined LITTLE_ENDIAN
	/* need to point to the LSB Quadlet for Fcode32 compliance
	   - addr should already be correct for little-endian systems
	 */
	if (fcode32)
		addr += sizeof (Quadlet);
#endif

	PUSHP(e, addr);
}

static Retcode
do_eval(Environ *e, Entry *code, Bool imm)
{
	Retcode ret = NO_ERROR;
	Bool iscompiling = e->iscompiling;
	Bool save32 = e->fcode32;
	Bool savepaginate = e->paginate;
	Exec_stack estk;

	if (e->debug)
	{
		if (code->name)
			ret = do_debug(e, code->name + 1, *(uByte*)code->name, code);
		else
			ret = do_debug(e, NULL, 0, code);

		if (ret != NO_ERROR)
			return ret;
	}

	/* immediate words have to be executed, but others can be compiled here */
	if (iscompiling && (imm || !(code->flags & F_IMMEDIATE)))
	{
		if (e->istokenizing)
		{
			/* deref the aliases if tokenizing */
			while (code->type == T_ALIAS)
				code = code->v.alias;

			/* any forth macros must be expanded here */
			if (code->type == T_FORTH && code->flags & F_TOKENIZE)
				return interp_text(e, code->v.fcode, CSTR);
		}

		return compile_fcode(e, code->xtok);
	}

	/* run-time call-stack info is chained on the C stack */
	estk.code = code;
	estk.prev = e->estk;
	e->estk = &estk;

	/* stay in FCode32-mode if we are already in it, otherwise switch to it */
	if (code->flags & F_FCODE32)
		e->fcode32 = TRUE;

	if (code->flags & F_PAGINATE)
		e->paginate = TRUE;

	/* allow breakpoints on built-in C funcs, vars, etc */
	if (e->debug && (code->flags & F_DEBUG) &&
			code->type != T_FCODE &&
			code->type != T_CREATE)
	{
		if (code->name)
			ret = do_debug(e, code->name + 1, *(uByte*)code->name, code);
		else
			ret = do_debug(e, NULL, 0, code);

		if (ret != NO_ERROR)
			goto done;
	}

	/* xtok entry may not point to the right instance so look it up offset */
	if (code->flags & F_INSTANCE)
	{
		Instance *inst = (Instance*)(uPtr)e->currinst;

		if (inst && inst->dict)
		{
			Entry *nent = inst->dict->list;
			Int i = inst->dict->num - code->offset - 1;

			while (nent && i-- > 0)
				nent = nent->link;

			if (nent)
				code = nent;
		}
	}

	switch (code->type)
	{
	case T_FUNC:	
		/* pointer to function - just call it */
		if (code->v.cfunc)
			ret = code->v.cfunc(e);
		else
			ret = E_EXECUTION_ERROR;

		break;

	case T_LABEL:
		/* machine code - push its addr on the stack */
		IFCKSP(e, 0, 1);
		PUSHP(e, code->v.addr);

		/* FALL-THROUGH */

	case T_CODE:
		/* jump to arbitrary code sequence here - machine-dependent */
#ifdef MACHINE_CODE
		ret = machine_code_jump(e, code->v.addr);
#else
		cprintf(e, "do_eval: machine-code not supported yet\n");
#endif
		break;

	case T_REGISTER:
#ifdef CPU_REGISTERS
		/* get contents of a CPU-register */
		{
			Cell v;
			IFCKSP(e, 0, 1);
			ret = cpu_register_read(e, code->v.val, &v);

			if (ret == NO_ERROR)
				PUSH(e, v);
		}
#else
		cprintf(e, "do_eval: CPU registers not supported\n");
#endif

		break;

	case T_BUFFER:
		/* contains the pointer to a buffer */	
		IFCKSP(e, 0, 1);
		PUSHP(e, code->v.addr);
		break;

	case T_ADDR:
		/* contains the address of a C variable */	
		IFCKSP(e, 0, 1);
		push_pointer(e, code->v.addr, save32);
		break;

	case T_ADDRVAL:
		/* also addr of C variable, but push its value */
		IFCKSP(e, 0, 1);
		PUSH(e, *(Cell*)code->v.addr);
		break;

	case T_CONST:
	case T_VALUE:
		/* simply push the data, whatever it means */
		IFCKSP(e, 0, 1);
		PUSH(e, code->v.val);
		break;

	case T_2CONST:
		/* Forth 2constant - push two values on the stack */
		IFCKSP(e, 0, 2);
		PUSH(e, code->v.val);
		PUSH(e, code->len);
		break;

	case T_VAR:
		/* Forth variable - push its address */
		IFCKSP(e, 0, 1);
		push_pointer(e, (Byte*)&code->v.val, save32);
		break;

	case T_FIELD:
		/* Forth field - offset the top-of-stack to point to the field */
		IFCKSP(e, 1, 0);
		e->sp[0] += code->v.val;
		break;

	case T_STRING:
		/* Pascal-style string - push its contents and then its length */
		IFCKSP(e, 0, 2);
		PUSHP(e, code->v.array + 1);
		PUSH(e, *(uByte*)code->v.array);
		break;

	case T_ALIAS:
		/* pointer to another Entry - just eval it - avoid simple recursion */
		if (code->v.alias == code)		/* catch this simple case */
			ret = E_EXECUTION_ERROR;
		else
			ret = do_eval(e, code->v.alias, FALSE);

		break;

	case T_FCODE:
		/* compiled fcode - execute it, switching off compiling if immediate */
		if (iscompiling && (code->flags & F_IMMEDIATE))
			e->iscompiling = FALSE;

		ret = execute(e, code->v.fcode, code->len);
		e->iscompiling = iscompiling;
		break;

	case T_CREATE:
		/* special create word - push pointer to data area before executing */
		if (iscompiling && (code->flags & F_IMMEDIATE))
			e->iscompiling = FALSE;

		IFCKSP(e, 0, 1);
		PUSHP(e, code->v.fcode + sizeof (Cell));
		ret = execute(e, *(Byte**)code->v.fcode, code->len);
		e->iscompiling = iscompiling;
		break;

	case T_EXECTOKEN:
	case T_DEFER:
		/* single execution token - just execute it */
		if (code->v.xtok < 0 || code->v.xtok >= e->numxtoks)
			ret = E_INVALID_XTOKEN;
		else
			ret = EXEC_XTOK(e, code->v.xtok);

		break;

	case T_FORTH:
		/* Forth text - interpret it, although we could also compile this
		   on-the-fly and replace the original definition here */
		ret = interp_text(e, code->v.array, CSTR);
		break;

	case T_PROPERTY:
		/* special case to handle nodefault-bytes - push addr+len of
		   property - built-in props are handled as T_FUNCs */
		IFCKSP(e, 0, 2);
		PUSHP(e, code->v.array);
		PUSH(e, code->len - 1);		/* assume these are strings */
		break;

	case T_TEMP:
		/* no-op for tokenizing - should never get here */
	default:
	#ifdef DEBUG
		dprintf("do_eval: code=%p illegal type=%d\n", code,
				code ? code->type : -1);
		display_rstack(e);
		display_stack(e);
	#endif
		ret = E_ILLEGAL_TYPE;
		break;
	}

	/* convert out-of-band hardware fault into a Forth exception */
	if (g_hwfault != NO_ERROR)
	{
		ret = g_hwfault;
		g_hwfault = NO_ERROR;
	}

	/* do not over-write an older ftrace */
	if (ret != NO_ERROR && ret != R_END && e->estack[0] == NULL)
	{
		/* save current call stack for ftrace */
		Exec_stack *es = e->estk;
		int i = 0;

		while (es && i < EXEC_STACK_SIZE - 1)
		{
			e->estack[i++] = es->code;
			es = es->prev;
		}

		e->estack[i++] = NULL;
	}

done:
	/* allow callee to stop estack recording by setting estk to NULL */
	if (e->estk)
		e->estk = estk.prev;

	e->fcode32 = save32;
	e->paginate = savepaginate;
	return ret;
}

/* need to check alarms here, but want to avoid getting into recursive loops
   when running them, so part of eval() is moved to do_eval() above */
static Retcode
eval(Environ *e, Entry *code, Bool imm)
{
	Int i;
	uLong ms;
	Retcode ret, r;
	Int savelpp;
	
	/* do not skip timers if there is an error */
	ret = do_eval(e, code, imm);

	if (e->inalarm || e->numalarms == 0 || g_eval_count++ < EVAL_ALARM_RUN)
		return ret;

	g_eval_count = 0;
	e->inalarm = TRUE;
	savelpp = e->linesperpage;
	e->linesperpage = e->lines + 1;		/* shut off pagination */
	ms = get_msecs(e);
	
	for (i = 0; i < MAX_ALARMS; i++)
	{
		if (e->alarms[i].xtok && e->alarms[i].ms &&
				ms - e->alarms[i].lastms >= e->alarms[i].ms)
		{
			Cell saveinst = e->currinst;
			Bool iscompiling = e->iscompiling;		/* do not compile alarms! */

			e->alarms[i].lastms = ms;
			
			if (e->alarms[i].xtok < 0 && e->alarms[i].xtok >= e->numxtoks)
				continue;

			e->currinst = (uPtr)e->alarms[i].inst;
			e->iscompiling = FALSE;
			r = do_eval(e, e->xtoks[e->alarms[i].xtok], FALSE);
			e->currinst = saveinst;
			e->iscompiling = iscompiling;

			if (r != NO_ERROR && r != R_END)
			{
				/* bail out of whatever we were doing */
				error(r);
				ret = E_USER_ABORT;		/* magic to skip past any catch */
				break;
			}
		}
	}
	
	e->linesperpage = savelpp;
	e->inalarm = FALSE;
	return ret;
}

Retcode
execute_xtok(Environ *e, Int xtok)
{
	if (xtok < 0 || xtok >= e->numxtoks)
		return E_ILLEGAL_XTOK;
		
	return EVAL(e, e->xtoks[xtok], FALSE);
}

Retcode
execute(Environ *e, Byte *code, Int len)
{
	Retcode ret = NO_ERROR;
	Input savinput;
	
	push_input(e, &savinput);
	e->in.type = I_FCODE;
	e->in.buf = code;
	e->in.len = len;
	/* fspread, foffset, and fnext are initialized in push_input */
	
	while (e->in.loc < len && ret == NO_ERROR)
	{
		Int x = next_fcode_num(e);
		ret = EXEC_XTOK(e, x);
	}
	
	if (ret == R_END)
		ret = NO_ERROR;

	pop_input(e);	
	return ret;
}

Retcode
execute_method_name(Environ *e, Instance *inst, Byte *str, Int len)
{
	Entry *ent = NULL;
	Cell saveinst;
	Retcode ret;
	
	if (inst == NULL)
		return E_NULL_INSTANCE;
	
	if (inst->package == NULL)
		return E_NULL_PACKAGE;
	
	if (inst->dict)
		ent = find_table(inst->dict, str, len);

	if (ent == NULL)
		ent = find_table(inst->package->dict, str, len);
	
	if (ent == NULL)
		return E_NO_METHOD;
	
	saveinst = e->currinst;
	e->currinst = (uPtr)inst;
	ret = EVAL(e, ent, FALSE);
	e->currinst = saveinst;
	return ret;
}

Retcode
execute_static_method_name(Environ *e, Package *pkg, Byte *str, Int len)
{
	Entry *ent;
	Retcode ret;
	Package *savepkg;
	
	if (pkg == NULL)
		return E_NO_METHOD;
	
	ent = find_table(pkg->dict, str, len);
	
	if (ent == NULL)
		return E_NO_METHOD;
	
	savepkg = e->currpkg;
	e->currpkg = pkg;
	ret = EVAL(e, ent, FALSE);
	e->currpkg = savepkg;
	return ret;
}

Retcode
execute_method(Environ *e, Instance *inst, Int xtok)
{
	Entry *ent;
	Retcode ret;
	Cell saveinst;
	
	if (xtok < 0 || xtok >= e->numxtoks || e->xtoks[xtok] == NULL)
		return E_NO_METHOD;

	ent = e->xtoks[xtok];
	saveinst = e->currinst;
	e->currinst = (uPtr)inst;
	ret = EVAL(e, ent, FALSE);
	e->currinst = saveinst;
	return ret;
}

Retcode
execute_word(Environ *e, Byte *word)
{
	Entry *ent = find_table(e->names, word, CSTR);
	
	if (ent == NULL)
		return E_UNDEFINED_WORD;

	return EVAL(e, ent, FALSE);
}

Retcode
interp_fcode(Environ *e, Byte *fcode, Int fnext)
{
	Int f;
	Retcode ret = NO_ERROR;
	Int i;
	Input savinput;
	Int *savfcodes;
	#define USER_FCODE 0x800
	
	if (fcode == NULL)
		return E_BAD_FCODE_FORMAT;
	
	push_input(e, &savinput);
	e->in.type = I_FCODE;
	e->in.buf = fcode;
	/* fspread and foffset are initialized in push_input */
	e->in.fnext = fnext;
	
	/* check to see if there is a proper start byte, in case it is not Fcode */
	f = next_fcode_num(e);
	
	if (f != 0xF0 /* start0 */ &&
			f != 0xF1 /* start1 */ &&
			f != 0xF2 /* start2 */ &&
			f != 0xF3 /* start3 */ &&
			f != 0xFD /* version1 */)
	{
		pop_input(e);
		return E_BAD_FCODE_FORMAT;
	}

	/* save a copy of the current user-defined range of fcodes */
	savfcodes = (Int*)malloc((NUM_FCODES - USER_FCODE) * sizeof (Int));

	if (savfcodes == NULL)
		return E_OUT_OF_MEMORY;

	memcpy(savfcodes, e->fcodes + USER_FCODE,
			(NUM_FCODES - USER_FCODE) * sizeof (Int));

	/* init all user-defined fcodes to ferror */
	for (i = USER_FCODE; i < NUM_FCODES; i++)
		e->fcodes[i] = 0xFC;	/* ferror */

	/* evalulate start byte to initialize e->in */
	ret = EVAL(e, e->xtoks[f], FALSE);
	
	/* continue into our read-eval loop */
	for (f = next_fcode_num(e); ret == NO_ERROR && e->in.loc < e->in.len;
			f = next_fcode_num(e))
	{
		if (f < 0 || f >= NUM_FCODES || e->fcodes[f] == INVALID_FCODE)
		{
			ret = E_ILLEGAL_FCODE;
			break;
		}
		
		ret = EVAL(e, e->xtoks[e->fcodes[f] & FCODE_MASK],
				!!(e->fcodes[f] & FCODE_IMMEDIATE));
	}
	
	if (ret == R_END)
		ret = NO_ERROR;

	/* retore the previous user-defined fcodes table */
	memcpy(e->fcodes + USER_FCODE, savfcodes,
			(NUM_FCODES - USER_FCODE) * sizeof (Int));
	free(savfcodes);
	pop_input(e);
	return ret;
}

static Retcode
do_interpret(Environ *e)
{
	Byte *word;
	Int wlen;
	Retcode ret = NO_ERROR;
	Entry *ent;
	
	do
	{
		ret = parse_word_len_refill(e, &word, &wlen, TRUE);

		if (ret != NO_ERROR)
			break;

		if (wlen == 0 || word == NULL)
			continue;

		/* only allow certain commands in secure mode */
		if (e->security == SM_NVRAMRC)
		{
			if (compare_strs(word, wlen, "boot", CSTR) ||
					compare_strs(word, wlen, "go", CSTR) ||
					compare_strs(word, wlen, "nvedit", CSTR) ||
					compare_strs(word, wlen, "password", CSTR) ||
					compare_strs(word, wlen, "reset-all", CSTR))
			{
				cprintf(e, "Sorry - cannot execute \"%S\" in nvramrc\n",
						word, wlen);
				return E_ABORT;
			}
		}

#ifdef ASSEMBLER
		/* if machine-dependent assembler is included, and we are
		   assembling code into a word, then lookup this word in the 
		   assembler memnonics list instead of the various symbol tables
		 */
		ent = NULL;

		if (e->iscompiling && e->assemble)
			ent = lookup_asm(e, word, wlen);

		if (ent == NULL)
#endif
		ent = lookup_sym(e, word, wlen);
		
		if (ent == NULL)
		{
			Cell num;
			Cell err;
			Byte *str = word;
			Int len = wlen;
			
			parse_number(e->radix, &str, &len, &num, &err, TRUE);

			if (err)
			{
				/* what the heck - try parsing it as a C-style hex constant */
				if (word[0] == '0' && (word[1] == 'x' || word[1] == 'X'))
				{
					str = word + 2;
					len = wlen - 2;;
					parse_number(16, &str, &len, &num, &err, TRUE);
				}

			#ifdef SUN_COMPATIBILITY
				if (err)
				{
					/* first try looking it up in ourself */
					Sym_ent *sym = exec_sym2addr(e, e->oursyms, word, wlen);
					Cell *sp = e->sp;

					if (sym == NULL)
					{
						/* no luck - try using the symbol callback */
						IFCKSP(e, 0, 2);
						PUSHP(e, word);
						PUSH(e, wlen);
						ret = execute_word(e, "sym>value");

						/* try calling our generic callback hook directly */
						if (ret != NO_ERROR)
							ret = execute_word(e, "callback sym>value");

						if (ret == NO_ERROR)
						{
							POP(e, num);

							if (num == FTRUE)
							{
								POP(e, num);	/* addr of symbol */
								err = FFALSE;
							}
							else
								DROPN(e, 2);	/* drop our string */
						}
						else
							e->sp = sp;		/* restore stack pointer */
					}
					else
					{
						IFCKSP(e, 0, 1);
						PUSH(e, sym->addr);
						err = FFALSE;
					}
				}
			#endif /* SUN_COMPATIBILITY */

				if (err)		/* no luck - could not parse the whole thing */
				{
					cprintf(e, "no such symbol \"%S\"\n", word, wlen);
					ret = E_UNDEFINED_WORD;
					break;
				}

				err = FALSE;
			}

			if (e->debug)
			{
				ret = do_debug(e, word, wlen, NULL);
				
				if (ret != NO_ERROR)
					return ret;
			}
			
			/* we have a dnum if it ends with a period */
			if (e->iscompiling)
			{
#ifdef SF_64BIT
				ret = compile_num64(e, num);
#else
				ret = compile_fcode(e, 0x10);			/* b(lit) */

				if (ret == NO_ERROR)
					ret = compile_num32(e, num);
#endif
				if (ret != NO_ERROR)
					return ret;
			}
			else
			{
				IFCKSP(e, 0, 1);
				PUSH(e, num);
			}

			/* can only parse lower word of dnums for now */
			if (wlen > 0 && word[wlen - 1] == '.')
			{
				if (e->iscompiling)
					ret = compile_fcode(e, 0xA5);		/* 0 */
				else
				{
					IFCKSP(e, 0, 1);
					PUSH(e, 0);
				}
			}
			
			continue;
		}

		ret = EVAL(e, ent, FALSE);

	} while (e->in.loc < e->in.len && ret == NO_ERROR);
	
	return ret;
}

Retcode
interp_text(Environ *e, Byte *str, Int len)
{
	Retcode ret;
	Input savinput;
	
	setstrlen(&str, &len);
	push_input(e, &savinput);
	e->in.type = I_STRING;
	e->in.buf = str;
	e->in.len = len;
	ret = do_interpret(e);
	pop_input(e);
	
	if (ret == R_END)
		ret = NO_ERROR;

	return ret;
}

Retcode
interpret(Environ *e)
{
	Retcode r;
	Byte *word;
	Int wlen;
	Entry *ent;

	/* only allow certain commands in secure modes */
	if (e->security != SM_NONE && !e->got_password)
	{
#ifdef SUN_COMPATIBILITY
		cprintf(e, "\nType boot, go (continue), or login (command mode)\n");
#endif

		while (!e->got_password)
		{
			r = parse_word_len_refill(e, &word, &wlen, TRUE);

			if (word == NULL || wlen == 0)
				continue;

			switch (e->security)
			{
			case SM_COMMAND:
				if (!e->got_password &&
						!compare_strs(word, wlen, "boot", CSTR) &&
						!compare_strs(word, wlen, "go", CSTR) &&
						!check_password(e))
					continue;

				break;

			case SM_FULL:
				if (!e->got_password &&
						!compare_strs(word, wlen, "go", CSTR) &&
						!check_password(e))
					continue;

				break;

			default:
				break;
			}

			ent = find_table(e->names, word, wlen);
			
			if (ent == NULL)
				return E_UNDEFINED_WORD;

			r = EVAL(e, ent, FALSE);

			if (r != NO_ERROR && r != R_END && r != E_USER_ABORT)
				error(r);
		}
	}

	while ((r = do_interpret(e)) != R_END && r != E_USER_ABORT)
	{
		error(r);

		/* these exceptions need the stacks to be reset or else the next
		   command will likely trigger the same exception again */

		if (r == E_STACK_OVERFLOW || r == E_STACK_UNDERFLOW)
			e->sp = e->stack;

		if (r == E_RSTACK_OVERFLOW || r == E_RSTACK_UNDERFLOW ||
				r == E_RSTACK_IMBALANCE)
			e->rsp = e->rstack;
	}

	return r;
}
