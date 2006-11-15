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

/* (c) Copyright 1996-1998 by CodeGen, Inc.  All Rights Reserved. */

/* Tokenize the contents of a given text file. */

#include <stdio.h>
#include "defs.h"
#include "exe.h"

#ifndef STANDALONE
#error Tokenizer code may only be used in a hosted environment.
#endif

/*#define LIMITED 512*/	/* max size of tokenzied output for limited edition */


static Retcode
read_file(Environ *e, const char *file, Byte **ebuf, Int *elen)
{
	FILE *fp;
	Int len;
	Byte *buf;
	
	fp = fopen(file, "rb+");
	
	if (fp == NULL)
		return E_NO_FILE;
	
	/* figure out length of file */
	if (fseek(fp, 0, SEEK_END) < 0)
	{
		fclose(fp);
		return E_FILE_IO;
	}
	
	len = ftell(fp);
	
	if (len < 0)
	{
		fclose(fp);
		return E_FILE_IO;
	}
		
	/* go back to the beginning of the file */
	if (fseek(fp, 0, SEEK_SET) < 0)
	{
		fclose(fp);
		return E_FILE_IO;
	}
	
	/* now we know how much space to allocate, plus a null-byte */
	buf = (Byte*)malloc(len + 1);
	
	if (buf == NULL)
	{
		fclose(fp);
		return E_OUT_OF_MEMORY;
	}
	
	/* finally read in the file */
	if (fread(buf, sizeof *buf, len, fp) != len)
	{
		fclose(fp);
		return E_FILE_IO;
	}
	
	fclose(fp);
	buf[len] = '\0';

	*ebuf = buf;
	*elen = len;
	return NO_ERROR;
}

static Retcode
tokenize_file(Environ *e, const char *file)
{
	Int len;
	Byte *buf;
	Retcode ret;
	
	ret = read_file(e, file, &buf, &len);
	
	if (ret != NO_ERROR)
		return ret;
	
	ret = interp_text(e, buf, len);		/* actually tokenizes! */
	free(buf);
	return ret;
}

Retcode
tokenize(Environ *e, const char *source, const char *target)
{
	Byte *brk = e->fbrk;
	Int i;
	Retcode ret;
	FILE *fp;
	
	if (e->istokenizing)
		return E_TOKENIZER_ERROR;
	
	e->istokenizing = TRUE;
	e->iscompiling = TRUE;
	e->tokfcodes = 0x800;		/* start of user-defined range */
	ret = tokenize_file(e, source);
	
	if (ret == NO_ERROR)
	{
		fp = fopen(target, "w");
	
		if (fp == NULL)
			ret = E_NO_FILE;
		else
		{
			Doublet csum = 0;
			Int i;
			uInt len;
			uByte header[8];
			
			len = e->fbrk - brk;
			
			for (i = 0; i < len; i++)
				csum += (uByte)brk[i];
			
			header[0] = 0xF1;		/* start1 - only version we support */
			header[1] = 0x08;		/* magic format number - meets OF spec */
			header[2] = (uDoublet)csum >> BYTE_SIZE;
			header[3] = csum & BYTE_MASK;
			len += sizeof header;
			header[4] = len >> (BYTE_SIZE * 3);
			header[5] = (len >> (BYTE_SIZE * 2)) & BYTE_MASK;
			header[6] = (len >> BYTE_SIZE) & BYTE_MASK;
			header[7] = len & BYTE_MASK;
			len -= sizeof header;
	
			if (fwrite(header, 1, sizeof header, fp) != sizeof header)
				ret = E_FILE_IO;

#ifdef LIMITED
			/* truncate output for size-limited demo program */
			if (len > LIMITED)
			{
				cprintf(e, "Sorry, tokenizing truncated in demo version.\n");
				len = LIMITED;
			}
#endif

			if (fwrite(brk, 1, len, fp) != len)
				ret = E_FILE_IO;
			
			fclose(fp);
		}
	}
	
	if (e->tokfcodes >= NUM_FCODES)
		ret = E_OUT_OF_USER_FCODES;
	
	/* free all words that were defined in the user-defined fcode range */
	for (i = 0x800; i < NUM_FCODES; i++)
		if (e->xtoks[i] && e->xtoks[i]->xtok != 0xFC)	/* not ferror */
			forget_sym(e, e->xtoks[i]->name, PSTR, FALSE);
	
	e->istokenizing = FALSE;
	e->iscompiling = FALSE;
	e->fbrk = brk;
	return ret;
}

/* tokenize ("< >source-file< >target-file<cr>" --) [non-standard] */
C(f_tokenize)
{
	Byte *source, *target;
	Retcode ret;
	
	if (e->istokenizing)
		return E_TOKENIZER_ERROR;
	
	/* don't assume files are null-terminated; buffers may be reused */
	source = lstrdup(parse_word(e), PSTR);
	target = lstrdup(parse_line(e), PSTR);
	
	if (source == NULL || *source == 0 ||
			target == NULL || *target == 0)
		return E_EMPTY_STRING;
	
	ret = tokenize(e, source + 1, target + 1);
	free(target);
	free(source);
	return ret;
}

C(f_tokenizerb)		/* tokenizer[ (--) */
{
	if (!e->istokenizing)
		return E_TOKENIZER_ERROR;
	
	e->istokenizing = FALSE;
	e->iscompiling = FALSE;
	return NO_ERROR;
}

C(f_btokenizer)		/* ]tokenizer (--) */
{
	if (e->istokenizing)
		return E_TOKENIZER_ERROR;
	
	e->istokenizing = TRUE;
	e->iscompiling = TRUE;
	return NO_ERROR;
}

C(f_emit_byte)		/* emit-byte (FCode# --) */
{
	Int fc;
	
	if (!e->istokenizing)
		return E_TOKENIZER_ERROR;
	
	IFCKSP(e, 1, 0);
	POP(e, fc);
	return compile_byte(e, fc);
}

C(f_fload)			/* fload ([filename<cr>] --) */
{
	Byte *file;
	
	if (!e->istokenizing)
		return E_TOKENIZER_ERROR;
	
	file = parse_line(e);
	
	if (file == NULL || *file == 0)
		return E_TOKENIZER_ERROR;
	
	return tokenize_file(e, file + 1);
}

C(f_fcode_version2)	/* fcode-version2 (--) */
{
	if (!e->istokenizing)
		return E_TOKENIZER_ERROR;

	e->headertok = 0xB6;		/* named-token */
	return NO_ERROR;
}

C(f_fcode_end)		/* fcode-end (--) */
{
	if (!e->istokenizing)
		return E_TOKENIZER_ERROR;

	return compile_byte(e, 0x00);		/* end0 */
}

Retcode
load_file(Environ *e, const char *fname)
{
	Int len;
	Byte *buf;
	Retcode ret;
	EC(machine_init_load);
	
	ret = read_file(e, fname, &buf, &len);
	
	if (ret != NO_ERROR)
		return ret;

	if (machine_init_load(e) == NO_ERROR && e->load)
	{
		memcpy(e->load, buf, len);
		e->loadlen = len;
	}

	/* first see if it is fcode */
	ret = interp_fcode(e, buf, MAGIC_FNEXT);

	/* nope, try text */
	if (ret == E_BAD_FCODE_FORMAT)
		ret = interp_text(e, buf, len);

	/* nope, try executable image */
	if (ret != NO_ERROR && e->load && exec_is_exec(e))
	{
		ret = exec_load(e);

		if (ret == NO_ERROR)
		{
			exec_free_symbols(e, e->loadsyms);
			e->loadsyms = exec_load_symbols(e);
		}
	}

	free(buf);
	return ret;
}

C(f_load_file)		/* load-file ("file-name<cr>" --) [non-standard] */
{
	Byte *fname;
	
	fname = parse_line(e);
	
	if (fname == NULL || *fname == 0)
		return E_EMPTY_STRING;
	
	return load_file(e, fname + 1);
}



Retcode
detok_file(Environ *e, const char *fname)
{
	Retcode ret;
	Int len, f, i;
	Int indent = 0;
	Byte *buf, *str;
	Input savinput;
	
	ret = read_file(e, fname, &buf, &len);
	
	if (ret != NO_ERROR)
		return ret;

	push_input(e, &savinput);
	e->in.type = I_FCODE;
	e->in.buf = buf;
	e->in.len = len;

	/* first check the start byte of the header */
	cprintf(e, "%6d: ", e->in.loc);
	f = next_fcode_num(e);

	switch (f)
	{
	case 0xF0:
		cprintf(e, "start0\n");
		break;
	case 0xF1:
		cprintf(e, "start1\n");
		break;
	case 0xF2:
		cprintf(e, "start2\n");
		break;
	case 0xF3:
		cprintf(e, "start3\n");
		break;
	case 0xFD:
		cprintf(e, "version1\n");
		break;
	default:
		cprintf(e, "unknown start byte = 0x%x\n", f);
		pop_input(e);
		return E_BAD_FCODE_FORMAT;
	}

	/* then the format */
	cprintf(e, "%6Cd: ", e->in.loc);
	f = next_fcode_byte(e);
	cprintf(e, "format = %#x\n", f);

	/* the checksum */
	cprintf(e, "%6Cd: ", e->in.loc);
	f = (uByte)next_fcode_byte(e) << BYTE_SIZE;
	f |= (uByte)next_fcode_byte(e);
	cprintf(e, "checksum = %#x\n", f);
	
	/* and finally the actual length */
	cprintf(e, "%6Cd: ", e->in.loc);
	f = (uByte)next_fcode_byte(e);
	f = (f << BYTE_SIZE) | (uByte)next_fcode_byte(e);
	f = (f << BYTE_SIZE) | (uByte)next_fcode_byte(e);
	f = (f << BYTE_SIZE) | (uByte)next_fcode_byte(e);
	cprintf(e, "file-len=%d, fcode-len=%d\n", len, f);
	e->in.len = f;

	while (e->in.loc < e->in.len)
	{
		cprintf(e, "%6Cd: ", e->in.loc);

		for (i = 0; i < indent; i++)
			cprintf(e, "    ");

		f = next_fcode_num(e);

		/* display the name */
		if (f != 0xFC && e->xtoks[f] == e->xtoks[0xFC])	/* ferror - undefined */
			cprintf(e, "%#x", f);
		else
			cprintf(e, "%P [%#x]", e->xtoks[f]->name, f);

		/* have to parse for special commands to show strings, nums, etc */
		switch (f)
		{
		case 0xB6:		/* named-token */
		case 0xCA:		/* external-token */
			next_fcode_string(e, &str);
			cprintf(e, " \"%P\":%d", str, *(uByte*)str);
			/* FALL THROUGH */

		case 0xB5:		/* newtoken */
		case 0x11:		/* b(') */
		case 0xC3:		/* b(to) */
			cprintf(e, " %#x", next_fcode_num(e));
			break;
		
		case 0x12:		/* b(") */
			next_fcode_string(e, &str);
			cprintf(e, " \"%P\":%d", str, *(uByte*)str);
			break;

		case 0x712:		/* b(string) */
			next_fcode_string(e, &str);
			(void)next_fcode_byte(e);
			cprintf(e, " \"%P\":%d", str, *(uByte*)str);
			break;
	
		case 0x10:		/* b(lit) */
			cprintf(e, " %d", next_fcode_num32(e));
			break;
		
#ifdef SF_64BIT
		/* non-standard vendor-specific extension */
		case 0x710:		/* b(lit64) */
			cprintf(e, " %Cd", next_fcode_num64(e));
			break;
#endif

		case 0x13:		/* bbranch */
		case 0x14:		/* b?branch */
		case 0x714:		/* begin-branch */
			f = next_fcode_offset(e);
			cprintf(e, " %d", f);
			indent += (f < 0) ? -1 : 1;
			break;

		case 0x17:		/* b(do) */
		case 0x717:		/* begin-do */
		case 0x18:		/* b(?do) */
		case 0x718:		/* begin-?do */
		case 0x1C:		/* b(of) */
			cprintf(e, " %d", next_fcode_offset(e));
			indent++;
		break;

		case 0x15:		/* b(loop) */
		case 0x16:		/* b(+loop) */
		case 0xC6:		/* b(endof) */
			cprintf(e, " %d", next_fcode_offset(e));
			indent--;
			break;

		case 0xC4:		/* b(case) */
		case 0x704:		/* begin-case */
		case 0xB1:		/* b(<mark) */
		case 0x701:		/* begin-mark */
		case 0xB7:		/* b(:) */
			indent++;
			break;

		case 0xC5:		/* b(endcase) */
		case 0xB2:		/* b(>resolve) */
		case 0xC2:		/* b(;) */
			indent--;
			break;
		}

		cprintf(e, "\n");

#ifdef LIMITED
			/* truncate output for size-limited demo program */
			if (e->in.loc > LIMITED)
			{
				cprintf(e, "Sorry, de-tokenizing truncated in demo version.\n");
				break;
			}
#endif
	}

	pop_input(e);
	free(buf);
	return NO_ERROR;
}


/*	de-tokenize a file producing the contents as Forth
 */
C(f_detok)		/* detok ("< >fcode-file<cr>" --) [non-standard] */
{
	Byte *file;
	Retcode ret;
	
	if (e->istokenizing)
		return E_TOKENIZER_ERROR;
	
	/* don't assume files are null-terminated; buffers may be reused */
	file = lstrdup(parse_line(e), PSTR);
	
	if (file == NULL || *file == 0)
		return E_EMPTY_STRING;

	ret = detok_file(e, file + 1);
	free(file);
	return ret;
}


const Initentry init_tokenizer[] =
{
	{ "tokenize", f_tokenize, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(\"< >source-file< >target-file<cr>\" --)\n" \
					"\ttokenize source file generating FCode target") },
			/* [non-standard] */
	{ "tokenizer[", f_tokenizerb, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) enter tokenizer-escape mode " \
					"allowing manual FCode generation") },
	{ "]tokenizer", f_btokenizer, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) exit tokenizer-escape mode resuming FCode generation") },
	{ "emit-byte", f_emit_byte, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(FCode# --) output given FCode# in tokenizer-escape mode") },
	{ "fload", f_fload, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("([filename<cr>] --) insert specified file at this point") },

	{ "fcode-version2", f_fcode_version2, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) begin tokenizing - must begin source file") },
	{ "fcode-end", f_fcode_end, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) end tokenizing source file") },

	{ "load-file", f_load_file, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"file-name<cr>\" --) load and execute Forth/FCode file") },
			/* [non-standard] */
	
	{ "detok", f_detok, INVALID_FCODE, F_IMMEDIATE | F_PAGINATE, T_FUNC
	HELP("(\"< >fcode-file<cr>\" --) detokenize (disassemble) FCode image") },
			/* [non-standard] */

	{ NULL, NULL }
};
