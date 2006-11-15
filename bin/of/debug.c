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

/* (c) Copyright 1996-1999 by CodeGen, Inc.  All Rights Reserved. */

#include "defs.h"
#include "exe.h"
#include "stdarg.h"

extern void show_table(Environ *e, Package *pkg, Table *t, int type,
		char *leadin);

static void
show_instance(Environ *e, Instance *inst)
{
	cprintf(e, "instance %p:\n", inst);

	if (inst == NULL)
		return;

	cprintf(e, "\tparent=%p\n", inst->parent);
	cprintf(e, "\tnumunits=%d\n", inst->numunits);
	cprintf(e, "\tunit=%#Cx.%Cx.%Cx.%Cx\n",
			inst->unit[0], inst->unit[1], inst->unit[2], inst->unit[3]);
	cprintf(e, "\tprobe=%#Cx.%Cx.%Cx.%Cx\n",
			inst->probe[0], inst->probe[1], inst->probe[2], inst->probe[3]);

	if (inst->args)
		cprintf(e, "\targs=%p \"%P\"\n", inst->args, inst->args);

	cprintf(e, "\tpackage=%p\n", inst->package);
	cprintf(e, "\tdict=%p:\n", inst->dict);
	show_table(e, inst->package, inst->dict, T_NONE, "\t\t");

	cprintf(e, "\tinterposed=%d\n", inst->interposed);
	cprintf(e, "\tself=%p\n", inst->self);
}

static void
show_package(Environ *e, Package *p, char *name, Bool recurse)
{
	int namelen = strlen(name);
	char *cname = &name[namelen];

	cprintf(e, "package %p %s:\n", p, name);
	cprintf(e, "    parent=%p\n", p->parent);
	cprintf(e, "    children=%p\n", p->children);
	cprintf(e, "    link=%p\n", p->link);

	cprintf(e, "    dict=%p:\n", p->dict);
	show_table(e, p, p->dict, T_NONE, "\t");
	cprintf(e, "    props=%p:\n", p->props);
	show_table(e, p, p->props, T_PROPERTY, "\t");

	cprintf(e, "    initinst: ");
	show_instance(e, p->initinst);

	cprintf(e, "    disp=%p\n", p->disp);
	cprintf(e, "    self=%p\n", p->self);

	if (cname[-1] != '/')
	    *cname++ = '/';

	/* show child packages */
	if (recurse)
		for (p = p->children; p; p = p->link)
		{
			Byte *nprop;
			Int nlen;
			Retcode ret = prop_get_str(p->props, "name", CSTR, &nprop, &nlen);

			if (ret != NO_ERROR)
			{
				cprintf(e, "error getting name property: %s (%d)\n",
						err2str(ret), ret);
				strcpy(cname, "?");
			}
			else
			{
				strncpy(cname, nprop, nlen); 
				cname[nlen] = '\0';
			}

			show_package(e, p, name, recurse);
		}

	name[namelen] = '\0';
}

C(f_curr_pkg)				/* curr-pkg (-- phandle) [non-standard] */
{
	IFCKSP(e, 0, 1);
	PUSHP(e, e->currpkg);
	return NO_ERROR;
}

C(f_dump_inst)				/* dump-inst (ihandle --) [non-standard] */
{
	Instance *inst;

	IFCKSP(e, 1, 0);
	POPT(e, inst, Instance*);

	if (inst == NULL)
		return E_NULL_INSTANCE;

	show_instance(e, inst);
	return NO_ERROR;
}

C(f_dump_pkg)				/* dump-pkg (phandle --) [non-standard] */
{
	Package *pkg;
	char buf[STR_SIZE];

	IFCKSP(e, 1, 0);
	POPT(e, pkg, Package*);

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	if (!get_device_name(e, pkg, buf))
	{
		buf[0] = '/';
		buf[1] = '\0';
	}

	show_package(e, pkg, buf, FALSE);
	return NO_ERROR;
}

C(f_dump_env)				/* dump-env (--) [non-standard] */
{
	Int i;
	Input *in;

	cprintf(e, "Environ = %p\n", e);

	if (e == NULL)
		return NO_ERROR;

	cprintf(e, "\tnames = %p\n", e->names);
	cprintf(e, "\troot = %p\n", e->root);
	cprintf(e, "\taliases = %p\n", e->aliases);
	cprintf(e, "\toptions = %p\n", e->options);
	cprintf(e, "\tchosen = %p\n", e->chosen);
	cprintf(e, "\tpackages = %p\n", e->packages);
	cprintf(e, "\tcurrpkg = %p\n", e->currpkg);
	
	cprintf(e, "\tcurrinst = %#Cx\n", e->currinst);
	cprintf(e, "\tkeyboard = %#Cx\n", e->keyboard);
	cprintf(e, "\tscreen = %#Cx\n", e->screen);
	
	cprintf(e, "\tfmem = %p\n", e->fmem);
	cprintf(e, "\tfbrk = %p\n", e->fbrk);
	cprintf(e, "\tsp = %p (%d)\n", e->sp, e->sp - e->stack);
	cprintf(e, "\trsp = %p (%d)\n", e->rsp, e->rsp - e->rstack);
	cprintf(e, "\testk = %p\n", e->estk);

#ifdef PROPERTY_MEM
	cprintf(e, "\tpropmem = %p\n", e->propmem);
	cprintf(e, "\tproplen = %d\n", e->proplen);
#endif
	
	cprintf(e, "\tradix = %Cd\n", e->radix);
	cprintf(e, "\tnumlen = %d\n", e->numlen);
	cprintf(e, "\tnumptr = %p\n", e->numptr);

	cprintf(e, "\tlinesout = %Cd\n", e->linesout);
	cprintf(e, "\toutcol = %Cd\n", e->outcol);
	cprintf(e, "\tlinesperpage = %Cd\n", e->linesperpage);
	cprintf(e, "\tscrollstep = %Cd\n", e->scrollstep);
	
	cprintf(e, "\txtoks = %p\n", e->xtoks);
	cprintf(e, "\tnumxtoks = %d\n", e->numxtoks);
	cprintf(e, "\tmaxxtoks = %d\n", e->maxxtoks);
	cprintf(e, "\ttokfcodes = %d\n", e->tokfcodes);
	
	for (in = &e->in; in; in = in->link)
	{
		cprintf(e, "\tin.type = %s\n", in->type == I_NONE ? "none" :
			in->type == I_FCODE ? "fcode" : 
			in->type == I_STRING ? "string" : 
			in->type == I_STREAM ? "stream" : "unknown");
		cprintf(e, "\tin.buf = %p\n", in->buf);
		cprintf(e, "\tin.len = %d\n", in->len);
		cprintf(e, "\tin.loc = %Cd\n", in->loc);
		cprintf(e, "\tin.fspread = %d\n", in->fspread);
		cprintf(e, "\tin.fnext = %d\n", in->fnext);
		cprintf(e, "\tin.foffset = %d\n", in->foffset ? 16 : 8);
		cprintf(e, "\tin.link = %p\n", in->link);
	}
	
	cprintf(e, "\texpect_len = %Cd\n", e->expect_len);
	cprintf(e, "\tkeyloc = %d\n", e->keyloc);
	cprintf(e, "\tkeylen = %d\n", e->keylen);

	cprintf(e, "\tno_banner = %s\n", e->no_banner ? "true" : "false");
	cprintf(e, "\tshowstack = %s\n", e->showstack ? "true" : "false");
	cprintf(e, "\tinstance = %s\n", e->instance ? "true" : "false");
	cprintf(e, "\tistokenizing = %s\n", e->istokenizing ? "true" : "false");
	cprintf(e, "\tistemp = %s\n", e->istemp ? "true" : "false");
	cprintf(e, "\tcursor = %s\n", e->cursor ? "true" : "false");
	cprintf(e, "\tfcode32 = %s\n", e->fcode32 ? "true" : "false");

	cprintf(e, "\tinalarm = %s\n", e->inalarm ? "true" : "false");
	cprintf(e, "\tnumalarms = %d\n", e->numalarms);

	cprintf(e, "\tdebug = %d\n", e->debug);
	cprintf(e, "\tsecurity = %d\n", e->security);
	cprintf(e, "\tgot_password = %s\n", e->got_password ? "true" : "false");

	cprintf(e, "\tflush_output = %d\n", e->flush_output);
	cprintf(e, "\tcapture_buf(%d) = %p\n", CAPTURE_BUF_SIZE, e->capture_buf);
	cprintf(e, "\tcapture_head = %d\n", e->capture_head);
	cprintf(e, "\tcapture_tail = %d\n", e->capture_tail);

	cprintf(e, "\tiscompiling = %d\n", e->iscompiling);
	cprintf(e, "\tcomp_state = %Cd\n", e->comp_state);
	
	cprintf(e, "\ttempbrk = %p\n", e->tempbrk);
	cprintf(e, "\ttempsp = %p\n", e->tempsp);
	cprintf(e, "\tnewdef = %p\n", e->newdef);
	
	cprintf(e, "\tload = %p\n", e->load);
	cprintf(e, "\tloadargs = %p\n", e->loadargs);
	cprintf(e, "\tloadentry = %p\n", e->loadentry);
	cprintf(e, "\tentrypoint = %p\n", e->entrypoint);
	cprintf(e, "\tstate_valid = %Cd\n", e->state_valid);
	cprintf(e, "\tgo_running = %s\n", e->go_running ? "true" : "false");

	cprintf(e, "\tloadsyms = %p\n", e->loadsyms);
	cprintf(e, "\toursyms = %p\n", e->oursyms);

	cprintf(e, "\tcallback = %p\n", e->callback);
	cprintf(e, "\tsym2value = %p\n", e->sym2value);
	cprintf(e, "\tvalue2sym = %p\n", e->value2sym);

	cprintf(e, "\tlogo_width = %d\n", e->logo_width);
	cprintf(e, "\tlogo_height = %d\n", e->logo_height);

	cprintf(e, "\tipos_package = %p\n", e->ipos_package);
	cprintf(e, "\tipos_len = %d\n", e->ipos_len);
	cprintf(e, "\tipos_arg = %p \"%s\"\n", e->ipos_arg,
			e->ipos_arg ? e->ipos_arg : "");

	cprintf(e, "\tmask = %#Cx\n", e->mask);

	cprintf(e, "\tcurline = %Cd\n", e->curline);
	cprintf(e, "\tcurcol = %Cd\n", e->curcol);
	cprintf(e, "\tinverse = %s\n", e->inverse ? "true" : "false");
	cprintf(e, "\tinvscreen = %s\n", e->invscreen ? "true" : "false");
	cprintf(e, "\tlines = %Cd\n", e->lines);
	cprintf(e, "\tcols = %Cd\n", e->cols);
	cprintf(e, "\tframebuf = %#Cx\n", e->framebuf);
	cprintf(e, "\tpixsize = %d\n", e->pixsize);
	cprintf(e, "\tfg = %d\n", e->fg);
	cprintf(e, "\tbg = %d\n", e->bg);
	cprintf(e, "\tswidth = %Cd\n", e->swidth);
	cprintf(e, "\tsheight = %Cd\n", e->sheight);
	cprintf(e, "\twtop = %Cd\n", e->wtop);
	cprintf(e, "\twleft = %Cd\n", e->wleft);
	cprintf(e, "\tcwidth = %Cd\n", e->cwidth);
	cprintf(e, "\tcheight = %Cd\n", e->cheight);
	cprintf(e, "\tcascent = %d\n", e->cascent);
	cprintf(e, "\tfontbytes = %Cd\n", e->fontbytes);
	cprintf(e, "\tescape = %Cd\n", e->escape);
	cprintf(e, "\tfont = %p\n", e->font);
	cprintf(e, "\tglyphs = %d\n", e->glyphs);
	cprintf(e, "\tminchar = %d\n", e->minchar);
	cprintf(e, "\temstate = %d\n", e->emstate);
	cprintf(e, "\temval = %s\n", e->emval == S_NONE ? "none" :
			e->emval == S_ESCAPE ? "escape" :
			e->emval == S_NUM1 ? "num1" :
			e->emval == S_NUM2 ? "num2" : "unknown");
	cprintf(e, "\temval2 = %d\n", e->emval2);
	cprintf(e, "\tembold = %d\n", e->embold);

	cprintf(e, "\teditbuf = %p\n", e->editbuf);
	cprintf(e, "\temax = %d\n", e->emax);
	cprintf(e, "\tetop = %d\n", e->etop);
	cprintf(e, "\telines = %d\n", e->elines);

	cprintf(e, "\tself = %p\n", e->self);

	cprintf(e, "\tpkgtop = %d\n", e->pkgtop);
	cprintf(e, "\tpkgstk[%d] = %p:\n", MAX_PKG_STACK_SIZE, e->pkgstk);

	for (i = 0; i <= e->pkgtop; i++)
		cprintf(e, "\t\t%d: %p\n", i, e->pkgstk[i]);

	cprintf(e, "\thcol = %d\n", e->hcol);
	cprintf(e, "\thyankbuf[%d] = %p \"%s\"\n",
			STR_SIZE, e->hyankbuf, e->hyankbuf);
	cprintf(e, "\thistory[%d] = %p:\n", MAX_CMD_HISTORY, e->history);

	for (i = 0; i < MAX_CMD_HISTORY; i++)
		if (e->history[i])
			cprintf(e, "\t\t%d: %s\n", i, e->history[i] + 1);

	cprintf(e, "\tfcodes[%d] = %p\n", NUM_FCODES, e->fcodes);
	cprintf(e, "\tstack[%d] = %p\n", STACK_SIZE, e->stack);
	cprintf(e, "\trstack[%d] = %p\n", RET_STACK_SIZE, e->rstack);
	cprintf(e, "\testack[%d] = %p\n", EXEC_STACK_SIZE, e->estack);
	cprintf(e, "\talarms[%d] = %p\n", MAX_ALARMS, e->alarms);
	cprintf(e, "\tkeybuf[%d] = %p \"%s\"\n", STR_SIZE, e->keybuf, e->keybuf);
	cprintf(e, "\tnumbuf[%d] = %p \"%s\"\n", STR_SIZE, e->numbuf, e->numbuf);

	return NO_ERROR;
}

C(f_dump_all)				/* dump-all (--) [non-standard] */
{
	char name[STR_SIZE];

	f_dump_env(e);

	cprintf(e, "\ne->names:\n");
	show_table(e, NULL, e->names, T_NONE, "");

	strcpy(name, "/");
	show_package(e, e->root, name, TRUE);
	return NO_ERROR;
}

C(f_mem_stats)		/* mem-stats (--) [non-standard] */
{
	size_t allocspc;
	size_t freespc;
	size_t allocblks;
	size_t freeblks;
	void *endarena;
	size_t arenasz;
	extern void *g_stack_bottom;
#ifdef START_DEFINED
	extern char start[];
#endif
#ifdef END_DEFINED
	extern char edata[], end[], etext[];
#endif

	cprintf(e, "g_mem : g_memory=%p g_mem_size=%#x g_mem_used=%#x\n",
			g_machine_memory, g_machine_memory_size, g_machine_memory_used);
	cprintf(e, ".text : ");
#ifdef START_DEFINED
	cprintf(e, "start=%p ", start);
#endif
#ifdef END_DEFINED
	cprintf(e, "etext=%p edata=%p end=%p", etext, edata, end);
#endif
	cprintf(e, "\n");
	cprintf(e, "stack : top=%p bottom=%p size=%#x\n",
			 &arenasz, (char*)g_stack_bottom,
			 (char*)g_stack_bottom - (char*)&arenasz);

	meminfo(&allocspc, &freespc, &allocblks, &freeblks, &endarena, &arenasz);
	cprintf(e, "malloc: allocspc=%#x freespc=%#x allocblks=%#x freeblks=%#x\n",
			allocspc, freespc, allocblks, freeblks);
	cprintf(e, "        startarena=%p endarena=%p arenasz=%#x\n",
			endarena - arenasz, endarena, arenasz);

	cprintf(e, "forth : mem=%p len=%#x here=%p free=%#x used=%#x\n",
			e->fmem, MEM_SIZE, e->fbrk, MEM_SIZE - (e->fbrk - e->fmem), 
			e->fbrk - e->fmem);

#ifdef PROPERTY_MEM
	cprintf(e, "Property-lists: mem=%p len=%d used=%d free=%d\n",
			e->propmem, PROPERTY_MEM, e->proplen, PROPERTY_MEM - e->proplen);
#endif

	return NO_ERROR;
}

#ifdef DEBUG_MALLOC
C(f_dump_heap)		/* dump-heap (--) [non-standard] */
{
	extern void dump_heap();
	dump_heap();
	return NO_ERROR;
}
#endif

C(f_dotrs)			/* .rs (--) [non-standard] */
{
	display_rstack(e);
	return NO_ERROR;
}

C(f_dotes)			/* .es (--) [non-standard] */
{
	display_exec_stack(e);
	return NO_ERROR;
}


/* 7.5.1  Automatic stack display */

C(f_showstack)		/* showstack (--) */
{
	e->showstack = TRUE;
	return NO_ERROR;
}

C(f_noshowstack)		/* noshowstack (--) */
{
	e->showstack = FALSE;
	return NO_ERROR;
}


/* 7.5.2   Serial download */
EC(machine_init_load);

CC(f_dl)				/* dl (--) */
{
	Int c;

	cprintf(e, "Begin serial transmission of Forth - type ^D to end...\n");
	machine_init_load(e);
	e->loadlen = 0;

	while ((c = get_key(e)) != CTRL('D'))
		e->load[e->loadlen++] = c;

	e->load[e->loadlen] = '\0';
	return interp_text(e, e->load, e->loadlen);
}

CC(f_dlbin)				/* dlbin (--) [non-standard] */
{
	Int c;
	uInt expectlen = 0;
	Retcode ret;

	cprintf(e, "Begin serial transmission of Fcode or executable...\n");
	machine_init_load(e);
	e->loadlen = 0;

	/* the 256 is a sanity check to give up after a certain point
	   if the image format cannot be identified */
	while (e->loadlen < expectlen || (expectlen == 0 && e->loadlen < 256))
	{
		c = get_key(e);

		/* magic length value that tells us to look for ^D */
		if (expectlen == ~0 && c == CTRL('D'))
			break;

		e->load[e->loadlen++] = c;

		if (expectlen == 0)
			(void)exec_length(e, &expectlen);
	}

	e->load[e->loadlen] = '\0';
	ret = execute_word(e, "init-program");

	if (ret != NO_ERROR)	
		return ret;

	return execute_word(e, "go");
}


/* 7.5.3   Dictionary */

/* 7.5.3.1  Dictionary search */
typedef enum
{
	L_NONE,
	L_XTOK,
	L_STR,
	L_ALL,
	L_CHAIN
} Look_for;

/* global to control displaying of headerless entries in show_name() below */
static Bool g_skipnull = FALSE;


#ifdef VERBOSE_DEBUG

static void
show_name(Environ *e, Package *pkg, Entry *ent, Bool nl)
{
	char name[STR_SIZE];

	if (g_skipnull && ent->name == NULL)
		return;

	if (pkg)
	{
		get_device_name(e, pkg, name);
		cprintf(e, "%s: ", name);
	}

	if (ent->name)
		cprintf(e, "%P [%#x]", ent->name, ent->xtok);
	else
		cprintf(e, "<%#x>", ent->xtok);

	if (nl)
		cprintf(e, "\n");
}

static Bool
look_for_xt(Environ *e, Entry *ent, Look_for what, Int xtok)
{
#ifdef SF_64BIT
	Cell ff;
#endif
	Input savinput;
	Int f, i;
	Int indent = 0;
	Byte *str;
	Bool ret = FALSE;

	push_input(e, &savinput);
	e->in.type = I_FCODE;
	e->in.buf = (ent->type == T_CREATE) ?
			*(Byte**)ent->v.fcode : ent->v.fcode;
	e->in.len = ent->len;

	while (e->in.loc < e->in.len)
	{
		i = e->in.loc;
		f = next_fcode_num(e);

		if (f == 0x00 || f == 0xFF)		/* end0 or end1 */
			break;
		else if (f == xtok)
		{
			ret = TRUE;
			break;
		}

		if (what == L_ALL)
		{
			cprintf(e, "%6d: ", i);

			for (i = 0; i < indent; i++)
				cprintf(e, "    ");

			show_name(e, NULL, e->xtoks[f], FALSE);
		}

		/* have to parse for special commands to skip strings, nums, etc */
		switch (f)
		{
		case 0xB6:		/* named-token */
		case 0xCA:		/* external-token */
			next_fcode_string(e, &str);
			
			if (what == L_ALL)
				cprintf(e, " \"%P\"", str);

			/* FALL THROUGH */

		case 0xB5:		/* newtoken */
		case 0x11:		/* b(') */
		case 0xC3:		/* b(to) */
			f = next_fcode_num(e);

			if (what == L_ALL)
				cprintf(e, " %#x", f);

			break;

		case 0x12:		/* b(") */
			next_fcode_string(e, &str);
			
			if (what == L_ALL)
				cprintf(e, " \"%P\":%d", str, *(uByte*)str);

			break;

		case 0x712:		/* b(string) */
			next_fcode_string(e, &str);
			(void)next_fcode_byte(e);
			
			if (what == L_ALL)
				cprintf(e, " \"%P\":%d", str, *(uByte*)str);

			break;
		
		case 0x10:		/* b(lit) */
			f = next_fcode_num32(e);

			if (what == L_ALL)
				cprintf(e, " %d", f);

			break;

#ifdef SF_64BIT
		case 0x710:		/* b(lit64) */
			ff = next_fcode_num64(e);

			if (what == L_ALL)
				cprintf(e, " %Cd", ff);

			break;
#endif
		
		case 0x13:		/* bbranch */
		case 0x14:		/* b?branch */
		case 0x714:		/* begin-branch */
			f = next_fcode_offset(e);

			if (what == L_ALL)
				cprintf(e, " %d", f);

			indent += (f < 0) ? -1 : 1;
			break;

		case 0x17:		/* b(do) */
		case 0x717:		/* begin-do */
		case 0x18:		/* b(?do) */
		case 0x718:		/* begin-?do */
		case 0x1C:		/* b(of) */
			f = next_fcode_offset(e);

			if (what == L_ALL)
				cprintf(e, " %d", f);

			indent++;
			break;

		case 0x15:		/* b(loop) */
		case 0x16:		/* b(+loop) */
		case 0xC6:		/* b(endof) */
			f = next_fcode_offset(e);

			if (what == L_ALL)
				cprintf(e, " %d", f);

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

		if (what == L_ALL)
			cprintf(e, "\n");
	}

	pop_input(e);
	return ret;
}

#else	/* !VERBOSE_DEBUG */

static int indent;
static Bool indentnext;

static void
do_indent(Environ *e)
{
	int i = indent;

	if (e->outcol == i * 4 || indentnext)
		return;

	if (e->outcol > 0)
		cprintf(e, "\n");

	for (i -= e->outcol / 4; i > 0; i--)
		cprintf(e, "    ");
}

static void
prtf(Environ *e, const char *fmt, ...)
{
	va_list args;
	char buf[STR_SIZE];
	extern int vbprintf(char *, const char *, va_list);

	va_start(args, fmt);
	vbprintf(buf, fmt, args);
	va_end(args);

	if (indentnext)
	{
		indentnext = FALSE;
		do_indent(e);
	}
	else if (e->outcol > 0 &&
			strlen(buf) + e->outcol >= (e->cols > 0 ? e->cols : 80) - 1)
		do_indent(e);

	cprintf(e, "%s", buf);
}

static void
show_name(Environ *e, Package *pkg, Entry *ent, Bool nl)
{
	char name[STR_SIZE];
	char buf[STR_SIZE];

	if (g_skipnull && ent->name == NULL)
		return;

	/* hide built-in magic words */
	if ((ent->xtok & FCODE_MASK & ~0xFF) == 0x700)
		return;

	if (pkg && pkg != e->currpkg)
	{
		get_device_name(e, pkg, name);
		bprintf(buf, "%s:", name);
	}
	else
		buf[0] = '\0';

	if (ent->name)
		bprintf(name, "%P", ent->name, ent->xtok);
	else
		bprintf(name, "(%#x)", ent->xtok);

	strcat(buf, name);
	prtf(e, "%s", buf);

	if (nl)
		cprintf(e, "\n");
}

static Bool
look_for_xt(Environ *e, Entry *ent, Look_for what, Int xtok)
{
#ifdef SF_64BIT
	Cell ff;
#endif
	Input savinput;
	Int f, i;
	Byte *str;
	Bool ret = FALSE;

	push_input(e, &savinput);
	e->in.type = I_FCODE;
	e->in.buf = (ent->type == T_CREATE) ?
			*(Byte**)ent->v.fcode : ent->v.fcode;
	e->in.len = ent->len;
	indentnext = FALSE;

	if (what == L_ALL)
		do_indent(e);

	while (e->in.loc < e->in.len)
	{
		i = e->in.loc;
		f = next_fcode_num(e);

		if (f == 0x00 || f == 0xFF)		/* end0 or end1 */
			break;
		else if (f == xtok)
		{
			ret = TRUE;
			break;
		}

		/* have to parse for special commands to skip strings, nums, etc */
		switch (f)
		{
		case 0xB6:		/* named-token */
		case 0xCA:		/* external-token */
			next_fcode_string(e, &str);
			
			if (what == L_ALL && *str)
				prtf(e, "%P ", str);

			/* FALL THROUGH */

		case 0xB5:		/* newtoken */
			f = next_fcode_num(e);
			break;

		case 0xC3:		/* b(to) */
		case 0x11:		/* b(') */
			i = next_fcode_num(e);

			if (what == L_ALL && e->xtoks[i])
			{
				if (e->xtoks[f]->name)
					prtf(e, "%s %P ", f == 0xC3 ? "to" : "'",
							e->xtoks[i]->name);
				else
					prtf(e, "(%#x) ", i);

				if (f == 0xC3)
					indentnext = TRUE;
			}

			break;

		case 0x12:		/* b(") */
			next_fcode_string(e, &str);
			
			if (what == L_ALL)
			{
				if ((uByte)e->in.buf[e->in.loc] == 0x90)	/* type */
				{
					e->in.loc++;
					prtf(e, ".\" %P\" ", str);
				}
				else
					prtf(e, "\" %P\" ", str);
			}

			break;

		case 0x712:		/* b(string) */
			next_fcode_string(e, &str);
			(void)next_fcode_byte(e);
			
			if (what == L_ALL)
			{
				if ((uByte)e->in.buf[e->in.loc] == 0x90)	/* type */
				{
					e->in.loc++;
					prtf(e, ".\" %P\" ", str);
				}
				else
					prtf(e, "\" %P\" ", str);
			}

			break;
		
		case 0x10:		/* b(lit) */
			f = next_fcode_num32(e);

			if (what == L_ALL)
			{
				if (indentnext)
				{
					do_indent(e);
					indentnext = FALSE;
				}
				else if (e->outcol + 8 >= (e->cols > 0 ? e->cols : 80) - 1)
					do_indent(e);

				PUSH(e, f);
				display_number(e, e->radix, (e->radix == 10), FALSE);
			}

			break;

#ifdef SF_64BIT
		case 0x710:		/* b(lit64) */
			ff = next_fcode_num64(e);

			if (what == L_ALL)
			{
				if (indentnext)
				{
					do_indent(e);
					indentnext = FALSE;
				}
				else if (e->outcol + 8 >= (e->cols > 0 ? e->cols : 80) - 1)
					do_indent(e);

				PUSH(e, ff);
				display_number(e, e->radix, (e->radix == 10), FALSE);
			}

			break;
#endif
		
		case 0x14:		/* b?branch */
		case 0x714:		/* begin-branch */
			i = next_fcode_offset(e);

			if (what == L_ALL)
			{
				if (i < 0)
				{
					--indent;
					do_indent(e);
					prtf(e, "until ");
				}
				else
				{
					i += e->in.loc - 6;

					/* begin..if..again..then is really begin..while..repeat */
					if ((uByte)e->in.buf[i] == 0x13 && e->in.buf[i + 1] < 0)
						prtf(e, "while ");
					else
					{
						prtf(e, "if ");
						indent++;
					}
				}

				indentnext = TRUE;
			}

			break;

		case 0x13:		/* bbranch */
			f = next_fcode_offset(e);

			if (what == L_ALL)
			{
				i = e->in.loc;

				if (f < 0)
				{
					--indent;
					do_indent(e);

					/* while-repeat becomes begin..if..again..then */
					if ((uByte)e->in.buf[i] == 0xB2)
					{
						prtf(e, "repeat ");
						e->in.loc++;
					}
					else
						prtf(e, "again ");
				}
				else
				{
					--indent;
					do_indent(e);
					prtf(e, "else ");
					++indent;
				}

				indentnext = TRUE;
			}

			break;

		case 0xB2:		/* b(>resolve) */
			/* an "else" clause also has a b(>resolve) for the "if" branch */
			if (what == L_ALL &&
					((uByte)e->in.buf[i - 3] != 0x13 || e->in.buf[i - 2] < 0))
			{
				--indent;
				do_indent(e);
				prtf(e, "then ");
				indentnext = TRUE;
			}

			break;

		case 0x17:		/* b(do) */
		case 0x717:		/* begin-do */
			i = next_fcode_offset(e);

			if (what == L_ALL)
			{
				prtf(e, "do ");
				++indent;
				indentnext = TRUE;
			}

			break;

		case 0x18:		/* b(?do) */
		case 0x718:		/* begin-?do */
			i = next_fcode_offset(e);

			if (what == L_ALL)
			{
				prtf(e, "?do ");
				++indent;
				indentnext = TRUE;
			}

			break;

		case 0x1C:		/* b(of) */
			f = next_fcode_offset(e);

			if (what == L_ALL)
			{
				prtf(e, "of ");
				++indent;
				indentnext = TRUE;
			}

			break;

		case 0x15:		/* b(loop) */
			f = next_fcode_offset(e);

			if (what == L_ALL)
			{
				--indent;
				do_indent(e);
				prtf(e, "loop ");
				indentnext = TRUE;
			}

			break;

		case 0x16:		/* b(+loop) */
			f = next_fcode_offset(e);

			if (what == L_ALL)
			{
				--indent;
				do_indent(e);
				prtf(e, "+loop ");
				indentnext = TRUE;
			}

			break;

		case 0xC6:		/* b(endof) */
			f = next_fcode_offset(e);

			if (what == L_ALL)
			{
				--indent;
				do_indent(e);
				prtf(e, "endof ");
				indentnext = TRUE;
			}

			break;

		case 0xC4:		/* b(case) */
		case 0x704:		/* begin-case */
			if (what == L_ALL)
			{
				prtf(e, "case ");
				++indent;
				indentnext = TRUE;
			}

			break;

		case 0xB1:		/* b(<mark) */
		case 0x701:		/* begin-mark */
			if (what == L_ALL)
			{
				do_indent(e);
				prtf(e, "begin ");
				++indent;
				indentnext = TRUE;
			}

			break;

		case 0xB7:		/* b(:) */
			if (what == L_ALL)
			{
				++indent;
				do_indent(e);
				prtf(e, ": ");
			}

			break;

		case 0xC5:		/* b(endcase) */
			if (what == L_ALL)
			{
				--indent;
				do_indent(e);
				prtf(e, "endcase ");
				indentnext = TRUE;
			}

			break;

		case 0xC2:		/* b(;) */
			if (what == L_ALL)
			{
				--indent;
				do_indent(e);
				prtf(e, "; ");
				indentnext = TRUE;
			}

			break;

		default:
			if (what == L_ALL)
			{
				show_name(e, NULL, e->xtoks[f], FALSE);

				if (compare_strs(ent->name, PSTR, e->xtoks[f]->name, PSTR) &&
						ent->xtok != f)
					cprintf(e, "[%#x]", f);

				if (!g_skipnull || ent->name)
					cprintf(e, " ");
			}

			break;
		}
	}

	if (what == L_ALL && e->outcol > 0)
		cprintf(e, "\n");

	pop_input(e);
	return ret;
}

#endif	/* !VERBOSE_DEBUG */

static Bool
match_str(Environ *e, Byte *s1, Int l1, Byte *s2, Int l2)
{
	while (l1 >= l2)
	{
		if (compare_strs(s1, l2, s2, l2))
			return TRUE;
		
		s1++;
		l1--;
	}

	return FALSE;
}

static void
look_for_entry(Environ *e, Package *pkg, Entry *ent,
		Look_for what, Byte *str, Int len)
{
	if (what == L_ALL)
	{
		show_name(e, pkg, ent, FALSE);

		if (ent->name)
			cprintf(e, " ");

		return;
	}

	if (what == L_STR && ent->name)
	{
		setstrlen(&str, &len);

		if (match_str(e, ent->name + 1, (uByte)ent->name[0], str, len))
		{
			show_name(e, pkg, ent, FALSE);
			cprintf(e, " ");
			return;
		}
	}

	switch (ent->type)
	{
	case T_CREATE:
	case T_FCODE:
		if (what == L_XTOK && look_for_xt(e, ent, what, len))
			show_name(e, pkg, ent, TRUE);

		break;

	case T_EXECTOKEN:
	case T_DEFER:
		if (what == L_XTOK && ent->v.xtok == len)
			show_name(e, pkg, ent, TRUE);

		break;

	case T_FORTH:
		if (match_str(e, ent->v.fcode, strlen(ent->v.fcode), str, len))
			show_name(e, pkg, ent, TRUE);

		break;
	}
}

static void
look_for_table(Environ *e, Package *pkg, Table *tab,
		Look_for what, Byte *str, Int len)
{
	Entry *ent;

	if (tab)
		for (ent = tab->list; ent; ent = ent->link)
			look_for_entry(e, pkg, ent, what, str, len);
}

static void
look_for_pkg(Environ *e, Package *pkg, Look_for what, Byte *str, Int len)
{
	if (pkg == NULL)
	{
		look_for_table(e, NULL, e->names, what, str, len);
		pkg = e->root;
	}

	look_for_table(e, pkg, pkg->dict, what, str, len);

	if (what == L_ALL)
		return;

	if (pkg->initinst)
		look_for_table(e, pkg, pkg->initinst->dict, what, str, len);

	for (pkg = pkg->children; pkg; pkg = pkg->link)
		look_for_pkg(e, pkg, what, str, len);
}

static void
look_for(Environ *e, Package *pkg, Look_for what, Byte *str, Int len)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Byte buf[STR_SIZE];

	if (str)
	{
		setstrlen(&str, &len);
		memcpy(buf, str, len);
		str = buf;
	}

	indent = 0;

	if (inst && inst->dict)
		look_for_table(e, inst->package, inst->dict, what, str, len);

	look_for_pkg(e, pkg, what, str, len);
}

static void
look_for2(Environ *e, Package *pkg, Look_for what, Byte *str, Int len)
{
	Byte buf[STR_SIZE];

	if (str)
	{
		setstrlen(&str, &len);
		memcpy(buf, str, len);
		str = buf;
	}

	indent = 0;
	g_skipnull = TRUE;
	look_for_pkg(e, pkg, what, str, len);
	g_skipnull = FALSE;
}

C(f_dotcalls)		/* .calls (xt --) */
{
	Int xtok;

	IFCKSP(e, 1, 0);
	POP(e, xtok);

	if (xtok <= 0 || xtok >= e->numxtoks)
		return E_ILLEGAL_XTOK;

	look_for(e, NULL, L_XTOK, NULL, xtok);
	return NO_ERROR;
}

C(f_dsift)			/* $sift (text-addr text-len --) */
{
	Byte *str;
	Int len;

	IFCKSP(e, 2, 0);
	POP(e, len);
	POPT(e, str, Byte*);
	look_for(e, NULL, L_STR, str, len);

	if (e->outcol > 0)
		cprintf(e, "\n");

	return NO_ERROR;
}

C(f_sifting)		/* sifting ("text< >" --) */
{
	Byte *word = parse_word(e);

	if (word == NULL || *word == 0)
		return E_EMPTY_STRING;
	
	look_for(e, NULL, L_STR, word + 1, (uByte)word[0]);

	if (e->outcol > 0)
		cprintf(e, "\n");

	return NO_ERROR;
}

C(f_words)			/* words (--) */
{
	look_for2(e, e->currpkg, L_ALL, NULL, 0);

	if (e->outcol > 0)
		cprintf(e, "\n");

	return NO_ERROR;
}


/* 7.5.3.2  Decompiler */

extern void show_entry(Environ *e, Package *pkg, Entry *ent, char *leadin,
		Bool tag);
static void look_and_mark(Environ *e, Entry *ent, Byte *mark);

static void
see_entry(Environ *e, Entry *ent, Look_for what)
{
	if (ent->type == T_FCODE || ent->type == T_CREATE)
	{
		if (what == L_CHAIN && e->numxtoks > NUM_FCODES)
		{
			Int len = e->numxtoks - NUM_FCODES;
			Byte *mark = (Byte*)malloc(len);

			if (mark == NULL)
			{
				cprintf(e, "error: out of memory\n");
				return;
			}

			memset(mark, 0, len);
			look_and_mark(e, ent, mark);
			free(mark);
		}
		else
		{
			if (ent->name && *ent->name)
				cprintf(e, ": %P ( %#x )\n", ent->name, ent->xtok);
			else
				cprintf(e, ": (%#x)\n", ent->xtok);

			indent = 1;
			indentnext = TRUE;
			look_for_xt(e, ent, L_ALL, -1);
			cprintf(e, ";\n");
		}
	}
	else
	{
		show_entry(e, NULL, ent, "", TRUE);
		cprintf(e, "\n");
	}
}

static void
look_and_mark(Environ *e, Entry *ent, Byte *mark)
{
#ifdef SF_64BIT
	Cell ff;
#endif
	Input savinput;
	Int f, i;
	Byte *str;

	if (ent->type != T_FCODE && ent->type != T_CREATE)
		return;

	push_input(e, &savinput);
	e->in.type = I_FCODE;
	e->in.buf = (ent->type == T_CREATE) ?
			*(Byte**)ent->v.fcode : ent->v.fcode;
	e->in.len = ent->len;

	while (e->in.loc < e->in.len)
	{
		i = e->in.loc;
		f = next_fcode_num(e);

		if (f == 0x00 || f == 0xFF)		/* end0 or end1 */
			break;

		/* have to parse for special commands to skip strings, nums, etc */
		switch (f)
		{
		case 0xB6:		/* named-token */
		case 0xCA:		/* external-token */
			next_fcode_string(e, &str);
			/* FALL THROUGH */

		case 0xB5:		/* newtoken */
		case 0xC3:		/* b(to) */
		case 0x11:		/* b(') */
			i = next_fcode_num(e);
			break;

		case 0x12:		/* b(") */
			next_fcode_string(e, &str);
			break;

		case 0x712:		/* b(string) */
			next_fcode_string(e, &str);
			(void)next_fcode_byte(e);
			break;

		case 0x10:		/* b(lit) */
			f = next_fcode_num32(e);
			break;

#ifdef SF_64BIT
		case 0x710:		/* b(lit64) */
			ff = next_fcode_num64(e);
			break;
#endif

		case 0x14:		/* b?branch */
		case 0x714:		/* begin-branch */
		case 0x13:		/* bbranch */
		case 0x17:		/* b(do) */
		case 0x717:		/* begin-do */
		case 0x18:		/* b(?do) */
		case 0x718:		/* begin-?do */
		case 0x1C:		/* b(of) */
		case 0x15:		/* b(loop) */
		case 0x16:		/* b(+loop) */
		case 0xC6:		/* b(endof) */
			f = next_fcode_offset(e);
			break;

		case 0xB2:		/* b(>resolve) */
		case 0xC4:		/* b(case) */
		case 0x704:		/* begin-case */
		case 0xB1:		/* b(<mark) */
		case 0x701:		/* begin-mark */
		case 0xB7:		/* b(:) */
		case 0xC5:		/* b(endcase) */
		case 0xC2:		/* b(;) */
			break;

		default:
			if (f >= NUM_FCODES && mark[f - NUM_FCODES] == 0)
				mark[f - NUM_FCODES] = 1;

			break;
		}
	}

	pop_input(e);
	see_entry(e, ent, L_ALL);

	for (i = 0; i < e->numxtoks - NUM_FCODES; i++)
		if (mark[i] == 1)
		{
			mark[i] = 2;
			look_and_mark(e, e->xtoks[NUM_FCODES + i], mark);
		}
}

static Entry *
try_number(Environ *e, Byte *word)
{
	Cell num, err;
	Byte *str = word;
	Int len = PSTR;
	
	/* if this is a numeric value, assume it is an xtok */
	parse_number(e->radix, &str, &len, &num, &err, TRUE);

	if (err)
	{
		/* try parsing it as a C-style hex constant */
		if (word[1] == '0' && (word[2] == 'x' || word[2] == 'X'))
		{
			str = word + 3;
			len = CSTR;
			parse_number(16, &str, &len, &num, &err, TRUE);
		}
	}

	if (err || num < 0 || num >= e->numxtoks)
		return NULL;

	return e->xtoks[num];
}

static Retcode
do_see(Environ *e, Look_for what)
{
	Byte *word = parse_word(e);
	Entry *ent = lookup_sym(e, word, PSTR);

	if (ent == NULL)
		ent = try_number(e, word);

	if (ent == NULL)
		return E_BAD_SYMBOL;

	see_entry(e, ent, what);
	return NO_ERROR;
}

C(f_see)				/* see ("old-name< >" --) */
{
	return do_see(e, L_ALL);
}

C(f_see_chain)			/* see ("old-name< >" --) [non-standard] */
{
	return do_see(e, L_CHAIN);
}

static Retcode
do_pseep(Environ *e, Look_for what)
{
	Int xtok;

	IFCKSP(e, 1, 0);
	POP(e, xtok);

	if (xtok < 0 || xtok >= e->numxtoks)
		return E_ILLEGAL_XTOK;
	
	see_entry(e, e->xtoks[xtok], what);
	return NO_ERROR;
}

C(f_pseep)				/* (see) (xt --) */
{
	return do_pseep(e, L_ALL);
}

C(f_psee_chainp)		/* (see-chain) (xt --) [non-standard] */
{
	return do_pseep(e, L_CHAIN);
}


/* patch a sequence of compiled xtok bytes by re-compiling it while
   substituting the replacement for the first occurrence of fromxtok
 */
static Retcode
do_patch(Environ *e, Entry *ent, Bool fromisnum, Cell fromxtok,
		Bool isnum, Cell xtok)
{
	Input savinput;
	Int f;
	Retcode ret = NO_ERROR;
	Byte *newfcode = e->fbrk;

	DPRINTF(("do_patch: ent=%p fromisnum=%d fromxtok=%Cx isnum=%d xtok=%Cx\n",
			ent, fromisnum, fromxtok, isnum, xtok));

	if (e->iscompiling)
		return E_ABORT;
	
	/* push this entry's compiled fcode/xtok sequence for parsing */
	push_input(e, &savinput);
	e->in.type = I_FCODE;
	e->in.buf = (ent->type == T_CREATE) ?
			*(Byte**)ent->v.fcode : ent->v.fcode;
	e->in.len = ent->len;
	e->iscompiling = TRUE;
	
	while (e->in.loc < e->in.len)
	{
		f = next_fcode_num(e);

		if (fromisnum)
		{
			if (f == 0x10)			/* b(lit) */
			{
				f = next_fcode_num32(e);
				
				if (fromxtok != INVALID_FCODE && f == fromxtok)
				{
					if (isnum)
					{
#ifdef SF_64BIT
						compile_num64(e, xtok);		/* numeric constant */
#else
						compile_fcode(e, 0x10);		/* b(lit) */
						compile_num32(e, xtok);		/* 32-bit constant */
#endif
					}
					else
						compile_fcode(e, xtok);		/* replacement xtok */

					fromxtok = INVALID_FCODE;		/* do only one */
				}
				else
				{
					/* not a matching number - just compile this value */
					compile_fcode(e, 0x10);			/* b(lit) */
					compile_num32(e, f);			/* original 32-bit value */
				}
				
				continue;
			}
#ifdef SF_64BIT
			else if (f == 0x710)	/* b(lit64) */
			{
				Cell ff = next_fcode_num64(e);
				
				if (fromxtok != INVALID_FCODE && ff == fromxtok)
				{
					if (isnum)
						compile_num64(e, xtok);		/* numeric constant */
					else
						compile_fcode(e, xtok);		/* replacement xtok */

					fromxtok = INVALID_FCODE;
				}
				else
				{
					/* not a matching number - just compile this value */
					compile_num64(e, ff);			/* original value */
				}
				
				continue;
			}
#endif
		}
		
		if (fromxtok != INVALID_FCODE && f == fromxtok)
		{
			if (isnum)
			{
#ifdef SF_64BIT
				compile_num64(e, xtok);		/* numeric constant */
#else
				compile_fcode(e, 0x10);		/* b(lit) */
				compile_num32(e, xtok);		/* 32-bit constant */
#endif
			}
			else
				compile_fcode(e, xtok);		/* replacement xtok */
			
			fromxtok = INVALID_FCODE;
			continue;
		}
		
		/* convert no-op markers back to original values before compilation */
		if ((f & 0xFF00) == 0x700 &&	/* special no-op position marker */
				f != 0x712)				/* b(string) is special */
			f &= 0xFF;		/* now the original fcode again */
		
		ret = execute_xtok(e, f);		/* really compiles this xtok */

		if (ret != NO_ERROR || f == 0x00 || f == 0xFF)		/* end0 or end1 */
			break;
	}

	e->iscompiling = FALSE;
	pop_input(e);
	ent->v.fcode = newfcode;
	ent->len = e->fbrk - newfcode;
	return ret;
}

C(f_ppatchp)		/* (patch) (new-n1 num1? old-n2 num2? xt --) */
{
	Cell newn1, oldn2;
	Int isnum1, isnum2, xt;
	
	IFCKSP(e, 5, 0);
	POP(e, xt);
	POP(e, isnum2);
	POP(e, oldn2);
	POP(e, isnum1);
	POP(e, newn1);
	
	if (xt < 0 || xt >= e->numxtoks || e->xtoks[xt] == NULL)
		return E_ILLEGAL_FCODE;
	
	return do_patch(e, e->xtoks[xt], isnum2, oldn2, isnum1, newn1);
}

C(f_patch)			/* patch ("new-name< >old-name< >word-to-patch< >" --) */
{
	Byte newname[STR_SIZE];
	Byte oldname[STR_SIZE];
	Byte word[STR_SIZE];
	Cell err;
	Byte *str;
	Int len;
	Entry *wordent, *newent, *oldent;
	Cell newxtok, oldxtok;

	newname[0] = 0;
	oldname[0] = 0;
	word[0] = 0;
	pstrcpy(newname, parse_word(e));
	pstrcpy(oldname, parse_word(e));
	pstrcpy(word, parse_word(e));
	
	if (*newname == 0 || *oldname == 0 || *word == 0)
		return E_EMPTY_STRING;
	
	wordent = lookup_sym(e, word, PSTR);

	if (wordent == NULL)
		wordent = try_number(e, word);
	
	if (wordent == NULL)
		return E_BAD_SYMBOL;
	
	oldent = lookup_sym(e, oldname, PSTR);
	
	if (oldent == NULL)
	{
		str = oldname;
		len = PSTR;
		parse_number(e->radix, &str, &len, &oldxtok, &err, TRUE);	
		
		if (err)
			return E_BAD_SYMBOL;
	}
	else
		oldxtok = oldent->xtok;
	
	newent = lookup_sym(e, newname, PSTR);
	
	if (newent == NULL)
	{
		str = newname;
		len = PSTR;
		parse_number(e->radix, &str, &len, &newxtok, &err, TRUE);	
		
		if (err)
			return E_BAD_SYMBOL;
	}
	else
		newxtok = newent->xtok;
	
	return do_patch(e, wordent, oldent == NULL, oldxtok,
			newent == NULL, newxtok);
}

CC(f_interact)		/* interact (--) [non-standard Sun-compatible] */
{
	enum Debug_mode savedebug = e->debug;
	Bool saveiscompiling = e->iscompiling;
	Input in;
	Retcode r;
	Byte buf[STR_SIZE];

	e->debug = DB_NONE;
	e->iscompiling = FALSE;
	cprintf(e, "Starting Forth interpreter - type \"resume\" to continue.\n");
	push_input(e, &in);
	e->in.type = I_STREAM;
	e->in.buf = buf;

	r = interpret(e);

	pop_input(e);
	e->debug = savedebug;
	e->iscompiling = saveiscompiling;
	return (r == R_END) ? NO_ERROR : r;
}

/* 7.5.3.4 Forth source-level debugger */

/* debug hook routine - called from evaluator */
Retcode
do_debug(Environ *e, Byte *name, Int nlen, Entry *code)
{
	Entry *ent = NULL;
	Int key;
	Int savelpp = e->linesperpage;
	Byte ename[STR_SIZE];
	Byte cname[STR_SIZE];
	
	if (e->debug == DB_NONE)
		return NO_ERROR;
	
	/* what are we trying to execute? */
	if (code)
	{
		if (code->name && *code->name)
			pstrcpy(cname, code->name);
		else
			bprintf(cname + 1, "[%#x]", code->xtok);
	}
	else if (name && *name && nlen)
		lstrcpy(cname, name, nlen);
	else
		strcpy(cname + 1, "<unknown>");

	/* the current Entry being executed should be on top of the estk */
	if (e->estk)
	{
		ent = e->estk->code;

		if (ent->name)
			pstrcpy(ename, ent->name);
		else
			bprintf(ename + 1, "[%#x]", ent->xtok);
	}
	else
		strcpy(ename + 1, "<unknown>");

	/* ent should be marked for debug or we will continue executing */
	if (e->debug != DB_TRACE_ALL && e->debug != DB_STEP_ALL &&
			(ent == NULL || !(ent->flags & F_DEBUG)))
		return NO_ERROR;

	if (e->outcol > 0)
		cprintf(e, "\n");

loop:
#ifdef VERBOSE_DEBUG
	if (e->iscompiling)
		cprintf(e, "COMPILE ");
	else if (e->debug == DB_TRACE || e->debug == DB_TRACE_ALL)
		cprintf(e, "TRACE ");
	else
		cprintf(e, "DEBUG ");

	show_entry(e, NULL, ent, "", TRUE);
	cprintf(e, ": ");
	show_entry(e, NULL, code, "", TRUE);
#else
	if (e->iscompiling)
		cprintf(e, "COMPILE %s: %s", ename + 1, cname + 1);
	else if (e->debug == DB_TRACE || e->debug == DB_TRACE_ALL)
		cprintf(e, "TRACE %s: %s", ename + 1, cname + 1);
	else
		cprintf(e, "%s: %s", ename + 1, cname + 1);
#endif

	if (e->fcode32)
		cprintf(e, " [32]");

	/* one-line stack display in hex for debugging convenience */
	if (e->sp > e->stack)
	{
		Int i = 4;
		Cell *sp = e->sp;

		if (sp - i > e->stack)
			cprintf(e, " (... ");
		else
			cprintf(e, " (");

		for (; i >= 0; i--)
			if (sp - i > e->stack)
				cprintf(e, "%#Cx ", sp[-i]);

		cprintf(e, "\b)");
	}

	if (e->debug == DB_STEP || e->debug == DB_STEP_ALL)
		cprintf(e, ": ");
	else
		cprintf(e, "\n");

	if (e->debug == DB_TRACE || e->debug == DB_TRACE_ALL)
		return NO_ERROR;

	e->linesperpage = e->lines + 1;		/* shut off pagination */
	key = get_key(e);
	cprintf(e, "\n");
	
	switch (key)
	{
	case ' ':
	case '\r':
	case '\n':
		break;
	
	case 'd':
	case 'D':
		/* mark this word for debugging if it is a word */
		if (code)
			code->flags |= F_DEBUG;

#ifdef SUN_COMPATIBILITY
		/* unmark the current entry so only one debug word is ever turned on */
		if (ent)
			ent->flags &= ~F_DEBUG;
#endif

		break;
	
	case 'u':
	case 'U':
		/* unmark the current entry */
		if (ent)
			ent->flags &= ~F_DEBUG;

		/* and mark our caller for debugging */
		if (e->estk && e->estk->prev && e->estk->prev->code)
		{
			e->estk->prev->code->flags |= F_DEBUG;
			break;
		}

		break;
	
	case 'c':
	case 'C':
		e->debug = (e->debug == DB_STEP_ALL) ? DB_TRACE_ALL : DB_TRACE;
		break;
	
	case 'f':
	case 'F':
		(void)f_interact(e);
		goto loop;
	
	case 'Q':
	case 'q':
		e->linesperpage = savelpp;		/* restore pagination */
		e->debug = DB_NONE;
		return E_QUIT;

	/* non-standard extensions */
	case 'n':
	case 'N':
		/* turn debugging off for this node */
		if (ent)
			ent->flags &= ~F_DEBUG;

		break;

	case 't':
	case 'T':
		/* show stacks */
		display_stack(e);
		display_rstack(e);
		display_exec_stack(e);
		goto loop;

	case 'a':
	case 'A':
		/* turn on step-all - subsequent continue makes it trace-all */
		e->debug = DB_STEP_ALL;
		goto loop;

	/* Sun OBP compatible extensions */
	case 's':
	case 'S':
		if (ent)
			see_entry(e, ent, L_ALL);

		goto loop;

	case '$':
		if (!CKSP(e, 2, 0) && TOP(e) > 0 && TOP(e) < STR_SIZE)
			cprintf(e, "top-of-stack as string: \"%S\"\n",
					(Byte*)(uPtr)STACK(e, 1), (Int)TOP(e));
		else
			cprintf(e, "not enough args or bad args on top of stack\n");

		goto loop;

	case 'h':
	case 'H':
	case '?':
	default:
		cprintf(e, "<SP>  execute next word\n");
		cprintf(e, "d     down - mark this word for debugging\n");
		cprintf(e, "u     up - unmark word and mark caller for debugging\n");
		cprintf(e, "n     no debug - turn off debugging for word\n");
		cprintf(e, "c     continue with tracing on\n");
		cprintf(e, "f     Forth interpreter - type \"resume\" to continue\n");
		cprintf(e, "q     quit - abort execution of word and all callers\n");
		cprintf(e, "s     see the word currently being debugged\n");
		cprintf(e, "$     display <addr,len> on top of stack as string\n");
		cprintf(e, "t     display both data and return stacks\n");
		cprintf(e, "a     change mode to step-all - continue will trace-all\n");
		goto loop;
	}
	
	e->linesperpage = savelpp;		/* restore pagination */
	return NO_ERROR;
}

C(f_debug)			/* debug ("old-name< >" --) */
{
	Byte *word = parse_word(e);
	Entry *ent = lookup_sym(e, word, PSTR);

	if (ent == NULL)
		ent = try_number(e, word);
	
	if (ent == NULL)
		return E_BAD_SYMBOL;
	
	ent->flags |= F_DEBUG;
	e->debug = DB_STEP;
	cprintf(e, "Debug keys: <SP> Down Up Continue Forth Quit See $string ?\n");
	return NO_ERROR;
}

C(f_nodebug)			/* nodebug ("old-name< >" --) [non-standard] */
{
	Byte *word = parse_word(e);
	Entry *ent = lookup_sym(e, word, PSTR);

	if (ent == NULL)
		ent = try_number(e, word);
	
	if (ent == NULL)
		return E_BAD_SYMBOL;
	
	ent->flags &= ~F_DEBUG;
	return NO_ERROR;
}

C(f_parendebug)		/* (debug (xt --) */
{
	Entry *ent;
	Int xtok;
	
	IFCKSP(e, 1, 0);
	POP(e, xtok);
	
	if (xtok < 0 || xtok >= e->numxtoks)
		return E_ILLEGAL_XTOK;
	
	ent = e->xtoks[xtok];
	
	if (ent == NULL)
		return E_BAD_SYMBOL;
	
	ent->flags |= F_DEBUG;
	e->debug = DB_STEP;
	cprintf(e, "Debug keys: <SP> Down Up Continue Forth Quit See $string ?\n");
	return NO_ERROR;
}

C(f_parennodebug)		/* (nodebug (xt --) [non-standard] */
{
	Entry *ent;
	Int xtok;
	
	IFCKSP(e, 1, 0);
	POP(e, xtok);
	
	if (xtok < 0 || xtok >= e->numxtoks)
		return E_ILLEGAL_XTOK;
	
	ent = e->xtoks[xtok];
	
	if (ent == NULL)
		return E_BAD_SYMBOL;
	
	ent->flags &= ~F_DEBUG;
	return NO_ERROR;
}

C(f_tracing)		/* tracing (--) */
{
	e->debug = (e->debug == DB_STEP_ALL) ? DB_TRACE_ALL : DB_TRACE;
	return NO_ERROR;
}

C(f_stepping)		/* stepping (--) */
{
	e->debug = (e->debug == DB_TRACE_ALL) ? DB_STEP_ALL : DB_STEP;
	return NO_ERROR;
}

C(f_debug_off)		/* debug-off (--) */
{
	e->debug = DB_NONE;
	return NO_ERROR;
}

C(f_trace_all)		/* trace-all (--) [non-standard] */
{
	e->debug = DB_TRACE_ALL;
	return NO_ERROR;
}

C(f_step_all)		/* step-all (--) [non-standard] */
{
	e->debug = DB_STEP_ALL;
	return NO_ERROR;
}

C(f_resume)			/* resume (--) */
{
	return R_END;
}

/* 7.6.6  Symbolic debugging - only to support client interface */

CC(f_sym2value)			/* (addr len -- addr len false | n true) */
{
	Byte *name;
	Int len;
	Cell array[10];
	
	IFCKSP(e, 2, 3);
	POP(e, len);
	POPT(e, name, Byte*);

	/* no callback defined - use the builtin code in exe */
	if (e->sym2value == NULL)
	{
		Sym_ent *sym = NULL;

		if (e->loadsyms)
			sym = exec_sym2addr(e, e->loadsyms, name, len);

		if (sym == NULL)
			sym = exec_sym2addr(e, e->oursyms, name, len);

		if (sym == NULL)
		{
			PUSHP(e, name);
			PUSH(e, len);
			PUSH(e, FFALSE);
		}
		else
		{
			PUSH(e, sym->addr);
			PUSH(e, FTRUE);
		}

		return NO_ERROR;
	}

	array[0] = (Cell)"sym-to-value";
	array[1] = 1;
	array[2] = 2;
	array[3] = (uPtr)name;
	machine_callback(e, e->sym2value, array);
	
	if (array[2] == 2 && array[4] == 0)
	{
		PUSH(e, array[5]);
		PUSH(e, FTRUE);
	}
	else
	{
		PUSHP(e, name);
		PUSH(e, len);
		PUSH(e, FFALSE);
	}
	
	return NO_ERROR;
}

CC(f_value2sym)			/* (n1 -- n1 false | n2 addr len true */
{
	Int value;
	Cell array[10];
	
	IFCKSP(e, 1, 4);
	POP(e, value);

	/* no callback defined - use the builtin code in exe */
	if (e->value2sym == NULL)
	{
		Sym_ent *sym = NULL;

		if (e->loadsyms)
			sym = exec_addr2sym(e, e->loadsyms, value);

		if (sym == NULL)
			sym = exec_addr2sym(e, e->oursyms, value);

		if (sym == NULL)
		{
			PUSH(e, value);
			PUSH(e, FFALSE);
		}
		else
		{
			PUSH(e, sym->addr);
			PUSHP(e, sym->name + 1);
			PUSH(e, *(uByte*)sym->name);
			PUSH(e, FTRUE);
		}

		return NO_ERROR;
	}

	array[0] = (Cell)"value-to-sym";
	array[1] = 1;
	array[2] = 2;
	array[3] = value;
	machine_callback(e, e->value2sym, array);
	
	if (array[2] == 2 && array[4] >= 0)
	{
		PUSH(e, array[4]);
		PUSH(e, array[5]);
		PUSH(e, strlen((char*)(uPtr)array[5]));
		PUSH(e, FTRUE);
	}
	else
	{
		PUSH(e, value);
		PUSH(e, FFALSE);
	}
	
	return NO_ERROR;
}

C(f_sym)			/* sym ("name< >" -- n) */
{
	Byte *name = parse_word(e);
	Int r;
	Retcode ret;

	IFCKSP(e, 0, 2);
	PUSHP(e, name + 1);
	PUSH(e, *name);
	ret = interp_text(e, "sym>value", CSTR);

	if (ret == E_INVALID_XTOKEN || ret == E_NO_CALLBACK)
		ret = f_sym2value(e);

	if (ret != NO_ERROR)
		return ret;

	POP(e, r);

	if (r == FFALSE)
	{
		DROPN(e, 2);	/* name+len removed from stack */
		return E_ABORT;
	}

	/* n is left on the stack */
	return NO_ERROR;
}

C(f_dotadr)			/* .adr (addr --) */
{
	Cell addr, symaddr;
	Byte *name;
	Int len;
	Int r;
	Retcode ret;

	IFCKSP(e, 1, 0);
	addr = TOP(e);
	ret = interp_text(e, "value>sym", CSTR);

	if (ret == E_INVALID_XTOKEN || ret == E_NO_CALLBACK)
		ret = f_value2sym(e);

	if (ret != NO_ERROR)
	{
		cprintf(e, "%#Cx\n", addr);
		return NO_ERROR;
	}

	POP(e, r);

	if (r == FFALSE)
	{
		DROP(e);			/* addr removed from stack */
		cprintf(e, "%#Cx\n", addr);
		return NO_ERROR;
	}

	POP(e, len);
	POPT(e, name, Byte*);
	POP(e, symaddr);
	cprintf(e, "%#Cx  %S+%#Cx\n", addr, name, len, addr - symaddr);
	return NO_ERROR;
}

/* search-mem (addr len pattern plen -- false | addr true)
   - not standard OpenFirmware function, but very useful to find
   random memory-trashing bugs
 */
C(f_search_mem)
{
	uByte *addr, *pattern;
	Int i, len, plen;

	IFCKSP(e, 4, 2);
	POP(e, plen);
	POPT(e, pattern, uByte*);
	POP(e, len);
	POPT(e, addr, uByte*);

	for (i = 0; i < len; i++)
	{
		if (memcmp(addr + i, pattern, plen) == 0)
		{
			PUSHP(e, addr + i);
			PUSH(e, FTRUE);
			return NO_ERROR;
		}
	}

	PUSH(e, FFALSE);
	return NO_ERROR;
}

/* ftrace (--) [non-standard]
   - Sun-compatible debug word - not standard OpenFirmware
   - show saved return stack after an exception
 */
C(f_ftrace)
{
	display_ftrace(e);
	return NO_ERROR;
}

/* clear-ftrace (--) [non-standard] 
	- clear previous ftrace to catch a new exception
 */
C(f_clear_ftrace)
{
	e->estack[0] = NULL;
	return NO_ERROR;
}

C(f_ldump)				/* ldump (addr len --) [non-standard] */
{
	#define LINELEN	16
	int i, l;
	uByte *addr, *saddr;
	Int len;
	
	IFCKSP(e, 2, 0);
	POP(e, len);
	POPT(e, saddr, uByte*);

	for (; (uPtr)saddr & (QALIGNMENT - 1); saddr--)
		len++;

	for (addr = saddr; (uPtr)addr & 0xF; addr--)
		len++;

	cprintf(e, "%-*s           0        4        8        C  "
			"0123456789ABCDEF\n", sizeof addr * 2, "");

	while (len > 0)
	{
		l = len < LINELEN ? len : LINELEN;
		cprintf(e, "%0*p  ", sizeof addr * 2, addr);

		for (i = 0; i < l; i += sizeof (uInt))
			if (addr + i < saddr)
				cprintf(e, "%08x ", *(uInt*)(addr + i));
			else
				cprintf(e, "\033[7m%08x\033[0m ", *(uInt*)(addr + i));

		for (; i < LINELEN; i += sizeof (uInt))
			cprintf(e, "%08x ", *(uInt*)(addr + i));

		cprintf(e, " ");

		for (i = 0; i < LINELEN; i++)
			if (addr + i < saddr || i >= l)
				cprintf(e, "%c", isprint(addr[i]) ? addr[i] : '.');
			else
				cprintf(e, "\033[7m%c\033[0m",
						isprint(addr[i]) ? addr[i] : '.');

		for (; i < LINELEN; i += sizeof (uInt))
			cprintf(e, "%c", isprint(addr[i]) ? addr[i] : '.');

		cprintf(e, "\n");
		len -= l;
		addr += l;
	}

	return NO_ERROR;
}

const Initentry init_debug[] =
{
	{ "dump-env", f_dump_env, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(--) display SmartFirmware g_e Environ struct") },
			/* [non-standard] */
	{ "dump-all", f_dump_all, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(--) display SmartFirmware g_e, e->names, and packages") },
			/* [non-standard] */
	{ "curr-pkg", f_curr_pkg, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- phandle) SmartFirmware g_e->currpkg pointer") },
			/* [non-standard] */
	{ "dump-pkg", f_dump_pkg, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(phandle --) display SmartFirmware Package") },
			/* [non-standard] */
	{ "dump-inst", f_dump_inst, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(ihandle --) display SmartFirmware Instance") },
			/* [non-standard] */
	{ "dump-curr-pkg", (Command)"curr-pkg dump-pkg",
			INVALID_FCODE, F_PAGINATE, T_FORTH
			HELP("(--) display SmartFirmware g_e->currpkg Package") },
			/* [non-standard] */
	{ "dump-curr-inst", (Command)"my-self dump-inst",
			INVALID_FCODE, F_PAGINATE, T_FORTH
			HELP("(--) display SmartFirmware g_e->currinst Instance") },
			/* [non-standard] */
	{ "mem-stats", f_mem_stats, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) display current Forth and malloc memory usage") },
			/* [non-standard] */
#ifdef DEBUG_MALLOC
	{ "dump-heap", f_dump_heap, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) display free blocks in the heap") },
			/* [non-standard] */
#endif
	{ ".rs", f_dotrs, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(--) display Forth return stack") },
			/* [non-standard] */
	{ ".es", f_dotes, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(--) display execution call stack") },
			/* [non-standard] */

	{ "showstack", f_showstack, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) turn on automatic stack display") },
	{ "noshowstack", f_noshowstack, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) turn off automatic stack display") },
	{ "dl", f_dl, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) download and execute Forth; end with ^D") },
	{ "dlbin", f_dlbin, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) download and execute FCode or executable image") },
			/* [non-standard] */

	{ ".calls", f_dotcalls, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(xt --) display all commands using execution token xt") },
	{ "$sift", f_dsift, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(text-addr text-len --) " \
					"display all commands containing text-string") },
	{ "sifting", f_sifting, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(\"text< >\" --) display all commands containing text") },
	{ "words", f_words, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(--) display all names of methods or commands") },

	{ "see", f_see, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(\"name< >\" --) decompile Forth-command name") },
	{ "see-chain", f_see_chain, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(\"name< >\" --) recursive decompile Forth-command name") },
			/* [non-standard] */
	{ "(see)", f_pseep, INVALID_FCODE, F_PAGINATE, T_FUNC
		HELP("(xt --) decompile Forth command whose execution token is xt") },
	{ "(see-chain)", f_psee_chainp, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(xt --) recursive decompile Forth execution token xt") },
			/* [non-standard] */
	{ "patch", f_patch, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"new-name< >old-name< >word-to-patch< >\" --)\n" \
		"\tchange all occurrences of old-name to new-name in word-to-patch") },
	{ "(patch)", f_ppatchp, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(new-n1 num1? old-n2 num2? xt --)\n" \
					"\tchange contents of command indicated by xt") },

	{ "interact", f_interact, INVALID_FCODE, F_NONE, T_FUNC
		HELP("(--) launch an interactive Forth shell - resume to continue") },
		/* [non-standard] */

	{ "debug", f_debug, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"name< >\" --) mark command name for debugging") },
	{ "nodebug", f_nodebug, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"name< >\" --) turn debugging off for command name") },
	{ "(debug", f_parendebug, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(xt --) mark command indicated by xt for debugging") },
	{ "(nodebug", f_parennodebug, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(xt --) turn debugging off for command indicated by xt") },
	{ "tracing", f_tracing, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) set trace mode for Forth source-level debugging") },
	{ "stepping", f_stepping, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) set step mode (default) for Forth source debugging") },
	{ "trace-all", f_trace_all, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) trace all Forth/FCode commands") },
			/* [non-standard] */
	{ "step-all", f_step_all, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) step through all Forth/FCode commands") },
			/* [non-standard] */
	{ "debug-off", f_debug_off, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) turn off the Forth source-level debugger") },
	{ "resume", f_resume, INVALID_FCODE, F_NONE, T_FUNC
			HELP("exit from a subroutine interpreter back to the stepper") },

	{ "sym>value", (Command)INVALID_FCODE, INVALID_FCODE, F_NONE, T_DEFER
			HELP("(addr len -- addr len false | n true)\n" \
					"\tdefer word to resolve symbol names") },
	{ "value>sym", (Command)INVALID_FCODE, INVALID_FCODE, F_NONE, T_DEFER
			HELP("(n1 -- n1 false | n2 addr len true)\n" \
					"\tdefer word to resolve symbol values") },
	{ "callback sym>value", f_sym2value, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(addr len -- addr len false | n true)\n" \
					"\tcallback hook to resolve symbol names") },
	{ "callback value>sym", f_value2sym, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(n1 -- n1 false | n2 addr len true)\n" \
					"\tcallback hook to resolve symbol values") },
	{ "sym", f_sym, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"name< >\" -- n) return value of client program name") },
	{ ".adr", f_dotadr, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(addr --) display symbol name + offset of addr") },

	{ "search-mem", f_search_mem, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(addr len pattern plen -- false | addr true)\n" \
					"\tsearch memory at addr for arbitrary byte pattern") },
			/* [non-standard] */
	{ "ftrace", f_ftrace, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(--) show return stack that caused the last exception") },
			/* [non-standard] */
	{ "clear-ftrace", f_clear_ftrace, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) clear previous ftrace to catch a new exception") },
			/* [non-standard] */

	{ "ldump", f_ldump, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(addr len --) dump memory in word-sized chunks") },
			/* [non-standard] */

	{ NULL, NULL }
};
