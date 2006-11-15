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

/* Misc. Fcode and Forth words that are somewhat machine-dependent but do not
   belong in machdep.c. */

#include "defs.h"
#include "machdep.h"


/* Spin the text cursor so user does not think we are hopelessly lost
 */
void
spin_cursor(Environ *e)
{
	static const char cursor[] = { '/', '-', '\\', '|' };
	static unsigned int curidx = 0;

	cprintf(e, "%c\b", cursor[curidx++ % sizeof (cursor)]);
}


/* 5.3.7.1 Peek/poke
   The following code uses the machine_probe_*() routines and should not
   need to be changed.
 */

static Retcode
peek(Environ *e, int size)
{
	Cell addr;
	Cell value;
	Bool ok;
	
	IFCKSP(e, 1, 2);
	POP(e, addr);
	DO_UMASK32(e, addr);
	ok = machine_probe_read(e, addr, &value, size);
	DO_MASK32(e, value);

	if (ok)
	{
		PUSH(e, value);
		PUSH(e, FTRUE);
	}
	else
		PUSH(e, FFALSE);

	return NO_ERROR;
}

static Retcode
poke(Environ *e, int size)
{
	Cell addr;
	Cell value;
	Bool ok;
	
	IFCKSP(e, 2, 1);
	POP(e, addr);
	POP(e, value);
	DO_UMASK32(e, addr);
	DO_MASK32(e, value);
	ok = machine_probe_write(e, addr, value, size);
	PUSH(e, ok ? FTRUE : FFALSE);
	return NO_ERROR;
}

C(f_cpeek)			/* cpeek (addr -- false | byte true) 0x220 */
{
	return peek(e, sizeof (Byte));
}

C(f_wpeek)			/* wpeek (waddr -- false | w true) 0x221 */
{
	return peek(e, sizeof (Word));
}

C(f_lpeek)			/* lpeek (qaddr -- false | quad true) 0x222 */
{
	return peek(e, sizeof (Int));
}

C(f_cpoke)			/* cpoke (byte addr -- okay?) 0x223 */
{
	return poke(e, sizeof (Byte));
}

C(f_wpoke)			/* wpoke (w waddr -- okay?) 0x224 */
{
	return poke(e, sizeof (Word));
}

C(f_lpoke)			/* lpoke (quad qaddr -- okay?) 0x225 */
{
	return poke(e, sizeof (Int));
}


/* 5.3.7.2  Device-register access
   The following code uses the machine_reg_*() routines above and should
   not need to be changed
 */

static Retcode
rget(Environ *e, int size)
{
	Cell addr;
	Cell value = 0;
	
	IFCKSP(e, 1, 1);
	POP(e, addr);
	DO_UMASK32(e, addr);
	machine_reg_read(e, addr, &value, size);
	DO_MASK32(e, value);
	PUSH(e, value);
	return NO_ERROR;
}

static Retcode
rset(Environ *e, int size)
{
	Cell addr;
	uCell value;
	
	IFCKSP(e, 2, 0);
	POP(e, addr);
	POP(e, value);
	DO_UMASK32(e, addr);
	DO_MASK32(e, value);
	machine_reg_write(e, addr, value, size);
	return NO_ERROR;
}

CC(f_rbget)			/* rb@ (addr -- byte) 0x230 */
{
	return rget(e, sizeof (Byte));
}

CC(f_rwget)			/* rw@ (waddr -- w) 0x232 */
{
	return rget(e, sizeof (Word));
}

CC(f_rlget)			/* rl@ (qaddr -- quad) 0x234 */
{
	return rget(e, sizeof (Int));
}

CC(f_rbset)			/* rb! (byte addr --) 0x231 */
{
	return rset(e, sizeof (Byte));
}

CC(f_rwset)			/* rw! (w waddr --) 0x233 */
{
	return rset(e, sizeof (Word));
}

CC(f_rlset)			/* rl! (quad qaddr --) 0x235 */
{
	return rset(e, sizeof (Int));
}


/* 5.3.7.3  Time */

/* Return the current time in milliseconds.  For C funcs.
 */
uLong
get_msecs(Environ *e)
{
	Time_value tv;

	machine_gettime(e, &tv);
	return tv.sec * 1000 + tv.nsec / 1000000;
}

CC(f_getmsecs)		/* get-msecs (-- n) 0x125 */
{
	IFCKSP(e, 0, 1);
	PUSH(e, get_msecs(e));
	return NO_ERROR;
}

/* Pause for "n" milliseconds using a CPU-intensive loop, then return.
 */
CC(f_ms)			/* ms (n --) 0x126 */
{
	uLong ms;

	IFCKSP(e, 1, 0);
	POP(e, ms);

	for (ms += get_msecs(e); get_msecs(e) < ms; )
		;

	return NO_ERROR;
}

/* Set an alarm so that execution token "xt" is executed every "n" milliseconds
   (or so).  This is implemented in eval() in exec.c as a simple (lame) polling
   loop instead of a proper hardware timer.  OpenFirmware has no specific
   requirements for timer accuracy, and since SmartFirmware is not designed to
   be interruptible, running this alarm as a real interrupt task could have
   unexpected side-effects (ie crashes).  Making SmartFirmware interrupt-safe
   would be very difficult, and seems to be pretty pointless for booting up.
 */
CC(f_alarm)			/* alarm (xt n --) 0x213 */
{
	Int xt;
	uLong ms;
	Int i;
	
	IFCKSP(e, 2, 0);
	POP(e, ms);
	POP(e, xt);

	if (xt < 0 || xt >= e->numxtoks)
		return E_ILLEGAL_XTOK;
	
	for (i = 0; i < MAX_ALARMS; i++)
	{
		if (ms == 0)
		{
			/* shut off an alarm */
			if (e->alarms[i].inst == (Instance*)(uPtr)e->currinst &&
					e->alarms[i].xtok == xt)
			{
				e->alarms[i].inst = NULL;
				e->alarms[i].xtok = 0;
				e->alarms[i].ms = 0;
				e->alarms[i].lastms = 0;
				e->numalarms--;
				return NO_ERROR;
			}
		}
		else if (e->alarms[i].inst == NULL &&
				e->alarms[i].xtok == 0 && e->alarms[i].ms == 0)
		{
			/* xt called every n milliseconds by eval() in exec.c */
			e->alarms[i].inst = (Instance*)(uPtr)e->currinst;
			e->alarms[i].xtok = xt;
			e->alarms[i].ms = ms;
			e->alarms[i].lastms = get_msecs(e);
			e->numalarms++;
			return NO_ERROR;
		}
	}
	
	cprintf(e, "alarm: out of space for new alarms!\n");
	return E_OUT_OF_MEMORY;
}

/* This is used to abort a timer task and kick back to a prompt.
   If a program is running then run interact to force it to a prompt.
 */
C(f_user_abort)		/* user-abort (... --) (R: ... --) 0x219 */
{
	Input *i = &e->in;

	/* search for the most recent text input stream */
	while (i->type == I_FCODE)
		i = i->link;

	if (e->go_running)
		return execute_word(e, "interact");

	if (i && i->type == I_STREAM)
		return execute_word(e, "abort");

	if (!e->inalarm)
		return execute_word(e, "interact");

	return execute_word(e, "abort");
}


/* 5.3.7.4  System information
   These routines use macros in machdep.h, and should not need to be changed
 */

C(f_fcode_rev)		/* fcode-revision (-- n) 0x87 */
{
	IFCKSP(e, 0, 1);
	PUSH(e, STANDARD_REV);
	return NO_ERROR;
}

Bool
parse_mac_addr(Environ *e, Byte **str, uByte mac_addr[], const char *errstr)
{
	Int i;
	Cell error, val;
	Int len = strlen(*str);
	Byte *s = *str;
	
	for (i = 0; i < 6; i++)
	{
		parse_number(16, &s, &len, &val, &error, FALSE);

		if (!error || (i < 5 && *s == ':') || (i == 5 && *s == '\0'))
			mac_addr[i] = (uByte)val;
		else
		{
			cprintf(e, "%s bad MAC Address format \"%s\"\n", errstr, s);
			return FALSE;
		}

		if (*s == ':')		/* increment pointer past colon */
			s++;
	}

	DPRINTF(("parse_mac_addr: %u:%u:%u:%u:%u:%u\n", mac_addr[0], mac_addr[1], 
				mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]));

	*str = s;
	return TRUE;
}

CC(f_mac_address)	/* mac-address (-- str len) 0x1A4 */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Retcode ret = NO_ERROR;

	IFCKSP(e, 0, 2);

	/* see if the active instance has a local-mac-address property */
#ifdef SUN_COMPATIBILITY
	if (inst && get_config_bool(e, "local-mac-address?", CSTR))
#else
	if (inst)
#endif
	{
		Entry *ent = NULL;

		while ((ent == NULL || ent->len != 6) && inst != NULL)
		{
			ent = find_table(inst->package->props, "local-mac-address", CSTR);
			inst = inst->parent;
		}

		if (ent && ent->len == 6)
		{
			PUSH(e, (uPtr)ent->v.array);
			PUSH(e, 6);
			return NO_ERROR;
		}
	}

#if defined EVAL_MAC_ADDRESS	/* arbitrary custom code */
	EVAL_MAC_ADDRESS
#else
	{
		/* check the NVRAM to see if "mac-address" param exists [non-standard] */
		Byte *str = get_config(e, "mac-address", CSTR);
		static uByte macaddr[6];

		/* parse the value */
		if (str != NULL && parse_mac_addr(e, &str, macaddr, "mac-address:\n"))
			PUSHP(e, macaddr);
		else
		{
		#if defined MAC_ADDRESS			/* default MAC address */
			PUSHP(e, MAC_ADDRESS);		/* 6-byte string or array */
		#else							/* just fake an address */
			memset(macaddr, 0, sizeof macaddr);
			macaddr[0] = 8;
			PUSHP(e, macaddr);
		#endif /* !MAC_ADDRESS */
		}
	}
#endif	/* !EVAL_MAC_ADDRESS */

	PUSH(e, 6);
	return ret;
}


/* 5.3.7.5  FCode self-test */

C(f_display_status)	/* display-status (n --) 0x121 */
{
	Int n;
	IFCKSP(e, 1, 0);
	POP(e, n);
	machine_led_write(e, n);
	return NO_ERROR;
}

CC(f_memory_test)	/* memory-test-suite (addr len -- fail?) 0x122 */
{
	Bool ok;
	Cell addr;
	Cell len;

	IFCKSP(e, 2, 1);
	POP(e, len);
	POP(e, addr);
	ok = machine_memory_test(e, addr, len, e->mask, diagnostic_mode(e));

	if (ok)
		PUSH(e, FFALSE);
	else
	{
		cprintf(e, "memory-test-suite failed for addr=%#Cx len=%Cd\n",
				addr, len);
		PUSH(e, FTRUE);
	}

	return NO_ERROR;
}

/* mask (-- addr) 0x124: T_ADDR to e->mask */


/* Additional machine-dependent Forth words.
   These use various machine_*() routines should not need to be changed.
 */

static Retcode
unalign_get(Environ *e, int size)
{
	Cell addr;
	Cell value;
	
	IFCKSP(e, 1, 1);
	DO_UMASK32(e, e->sp[0]);	/* not part of Fcode64/32 extensions */
	POP(e, addr);
	machine_unalign_read(e, addr, &value, size);
	PUSH(e, value);
	return NO_ERROR;
}

static Retcode
unalign_set(Environ *e, int size)
{
	Cell addr;
	Cell value;
	
	IFCKSP(e, 2, 0);
	DO_UMASK32(e, e->sp[0]);	/* not part of Fcode64/32 extensions */
	POP(e, addr);
	POP(e, value);
	machine_unalign_write(e, addr, value, size);
	return NO_ERROR;
}

C(f_unalign_wget)	/* unaligned-w@ (waddr -- w) */
{
	return unalign_get(e, sizeof (Doublet));
}

C(f_unalign_wset)	/* unaligned-w! (w waddr --) */
{
	return unalign_set(e, sizeof (Doublet));
}

C(f_unalign_lget)	/* unaligned-l@ (qaddr -- quad) */
{
	return unalign_get(e, sizeof (Quadlet));
}

C(f_unalign_lset)	/* unaligned-l! (quad qaddr --) */
{
	return unalign_set(e, sizeof (Quadlet));
}


/* 7.3.9.3  Assembler
	SmartFirmware uses FCode internally and so these default words only
	support explicit compilation via l, w, etc unless a machine-dependent
	assembler is defined and supported.
 */

C(f_code)			/* code ("new-name< >" -- code-sys) (E: ... -- ?) */
{
	Byte *word = parse_word(e);

	if (e->iscompiling || e->istokenizing)
		return E_COMPILATION_ERROR;

	if (word == NULL || *word == '\0')
		return E_EMPTY_STRING;

	e->newdef = new_entry();

	if (e->newdef == NULL)
		return E_OUT_OF_MEMORY;

	e->newdef->name = pstrdup(word);
	set_xtok(e, e->newdef);		/* init the fcode value */
	e->newdef->type = T_CODE;

	if (((Ptr)e->fbrk % ALIGNMENT) &&
			falloc(e, ALIGNMENT - ((Ptr)e->fbrk % ALIGNMENT)) == NULL)
		return E_OUT_OF_MEMORY;

	e->newdef->v.addr = e->fbrk;
	e->assemble = TRUE;
	e->comp_state = FTRUE;
	e->iscompiling = TRUE;
	e->tempsp = e->sp;
	return NO_ERROR;
}

C(f_label)			/* label ("new-name< >" -- code-sys) (E: -- addr) */
{
	Retcode ret = f_code(e);

	if (ret == NO_ERROR)
		e->newdef->type = T_LABEL;

	return ret;
}

C(f_csemicolon)		/* c; (code-sys --) */
{
	if (!e->iscompiling || !e->assemble)
		return E_COMPILATION_ERROR;

#ifdef MACHINE_CODE
	{
		/* compile the machine-dependent return code sequence */
		Retcode ret = machine_compile_return(e);
	
		if (ret != NO_ERROR)
			return ret;
	}
#endif

	e->newdef->len = e->fbrk - e->newdef->v.fcode;
	e->comp_state = FFALSE;
	e->iscompiling = FALSE;
	e->assemble = FALSE;
	return create_newdef(e);
}

C(f_endcode)		/* end-code (code-sys --) */
{
	if (!e->iscompiling || !e->assemble)
		return E_COMPILATION_ERROR;

	e->newdef->len = e->fbrk - e->newdef->v.fcode;
	e->comp_state = FFALSE;
	e->iscompiling = FALSE;
	e->assemble = FALSE;
	return create_newdef(e);
}


/* this is used to initialize the builtin help */
static const struct help_data
{
	char *name;
	char *help;
} g_help[] = {
	{ "",
"help category/Forth-word\n"
"	where category is one of:\n"
"	booting devices nvram nv-vars testing " MACHDEP_HELP_TITLE "\n"
	},
	{ "nvram",
"How to change non-volatile memory (nvram) parameter settings:\n"
"	(use \"help nv-vars\" for info about specific variables)\n"
"printenv [var]\n"
"	print the value (and the default) of var, or of all vars in nvram\n"
"	parameters if none is specified\n"
"setenv var string value\n"
"	set var to the specified \"string value\"\n"
"set-defaults\n"
"	set all vars to their default values - fast way to fix mistakes\n"
"set-default var\n"
"	set just var to its default value\n"
"nvedit\n"
"	edit the \"nvramrc\" script in non-volatile RAM with a basic emacs-style\n"
"	screen editor - type ^C (control-C) to exit the editor - changes are\n"
"	only written to nvram when the \"nvstore\" command is run (below)\n"
"nvstore\n"
"	write the current contents of the \"nvedit\" buffer to nvram\n"
	},
	{ "nv-vars",
"What each of the non-volatile memory (nvram) variables does:\n"
"	(use \"help nvram\" for info on how to see or set parameter values)\n"
"diag-switch?  values: true or false\n"
"	switch on/off extended diagnostics and verbose output for selftests\"\n"
"use-nvramrc?  values: true or false\n"
"	if true, run \"nvramrc\" (below) at bootup\n"
"nvramrc  value: string-script\n"
"	contents of nvram bootup script usually modified using \"nvedit\"\n"
"	- should include \"probe-all install-console banner\" (in that order)\n"
"input-device  values: input-device\n"
"output-device  values: output-device\n"
"	devices for console I/O - are usually aliases (see \"help devices\")\n"
"screen-#columns  value: number\n"
"screen-#rows  value: number\n"
"	number of rows and columns for console - maximum possible if 0\n"
"oem-banner?  values: true or false\n"
"	if true, display contents of \"oem-banner\" when \"banner\" is run\n"
"oem-banner  value: string\n"
"	string to display instead of the default system banner\n"
"oem-logo?  values: true or false\n"
"	if true, display bitmap of \"oem-logo\" when \"banner\" is run\n"
"oem-logo  value: 64x64x1 bitmap (512 bytes)\n"
"	bitmap to display instead of the default bitmap or picture\n"
	},
	{ "devices",
"A device path is like a Unix file path: /device@address:options/dev2/...\n"
"The address and options may be omitted to select the best matching device.\n"
"Paths may be relative to the currently open device. \"/\" is the root.\n"
"Getting around the OpenFirmware device tree:\n"
"pwd\n"
"	display the currently open device, if any\n"
"cd device\n"
"	open the specified device and make it the current device, or if the\n"
"	device is \"..\", open the parent of the current device\n"
"ls\n"
"	list devices at the current spot in the device tree\n"
"show-devs\n"
"	list all devices probed at bootup (and their types)\n"
"devalias [alias value]\n"
"	create a device alias with the specified value, or print all aliases\n"
"	- can be useful to select another console in the \"nvramrc\" script\n"
".properties\n"
"	display the property list for the currently open device\n"
"(see \"help testing\" to see how to run device selftests)\n"
	},
	{ "testing",
"If the nvram variable \"diag-switch?\" is true, the selftests generate\n"
"much more verbose output, and more extended tests are also turned on.\n"
"It may be useful to run non-verbose startup tests in the \"nvramrc\" script.\n"
"(See other help categories for more details.)\n"
"To run the device selftests:\n"
"test [device]\n"
"	test the specified device or currently open device by running\n"
"	its \"selftest\" method - may also turn on test LEDs or other indicators\n"
"test-all [device]\n"
"	recursively run tests on all devices at and below the specified device\n"
"	(or \"/\" if none is specified) (ie. on all the children recursively)\n"
	},
	{ "booting",
"Bootup uses some nvram parameters.  If there are no vars with these names,\n"
"the system may not completely support booting. (See other help for details.)\n"
"auto-boot?  value: true or false\n"
"	if true, will try to run the contents of \"boot-command\" at startup\n"
"boot-command  value: string-command\n"
"	command to run when the \"auto-boot?\" is set to true - usually \"boot\"\n"
"boot-device  value: string-device-name\n"
"	device to use as the boot device - usually a hard disk or network card\n"
"boot-file  value: string-file-name\n"
"	device-specific file-name (or args) to load and boot from \"boot-device\"\n"
"diag-device  value: string-device-name\n"
"diag-file  value: string-file-name\n"
"	used instead of boot-file and boot-device if \"diag-switch?\" is true\n"
"Booting also may be performed manually:\n"
"boot [device] [args]\n"
"	load and go from the specified device with the device args, or\n"
"	from the parameter settings if no device is specified\n"
	},

	{ MACHDEP_HELP_TITLE, MACHDEP_HELP },

	{ NULL }		/* MUST BE NULL TERMINATED */
};


C(f_help)			/* help ("{name}<eol>" --) */
{
	Byte *name = parse_line(e);
	const struct help_data *h;
#ifdef DETAILED_HELP
	Entry *ent;
#endif

	/* kill trailing white-space */
	while (name[0] && isspace(name[(uByte)name[0]]))
		name[0]--;

#ifdef DETAILED_HELP
	ent = lookup_sym(e, name, PSTR);

	if (ent)
	{
		switch (ent->type)
		{
		case T_FUNC:
		case T_FORTH:
		case T_ADDR:
		case T_ADDRVAL:
		case T_CONST:
		case T_DEFER:
		case T_REGISTER:
			if (ent->len)
			{
				cprintf(e, "%P %s\n", ent->name, (Byte*)(uPtr)ent->len);
				break;
			}

			/* FALL THROUGH */

		default:
			cprintf(e, "Sorry - no help for %P\n", ent->name);
			break;
		}
	}
	else
#endif
	if (name && *name)
	{
		for (h = g_help; h->name; h++)
			if (compare_strs(h->name, CSTR, name, PSTR))
				break;

		if (h->name == NULL)
			h = g_help;

		cprintf(e, "%s", h->help);
	}
	else
		/* 1st entry is generic/basic entry */
		cprintf(e, "%s", g_help->help);

	return NO_ERROR;
}

const Initentry init_other[] =
{
	{ "cpeek", f_cpeek, 0x220, F_NONE, T_FUNC
			HELP("(addr -- false | byte true) try to fetch byte at addr") },
	{ "wpeek", f_wpeek, 0x221, F_NONE, T_FUNC
			HELP("(waddr -- false | w true) try to fetch doublet at waddr") },
	{ "lpeek", f_lpeek, 0x222, F_NONE, T_FUNC
		HELP("(qaddr -- false | quad true) try to fetch quadlet at qaddr") },
	{ "cpoke", f_cpoke, 0x223, F_NONE, T_FUNC
			HELP("(byte addr -- okay?) try to store byte at addr") },
	{ "wpoke", f_wpoke, 0x224, F_NONE, T_FUNC
			HELP("(w waddr -- okay?) try to store doublet at waddr") },
	{ "lpoke", f_lpoke, 0x225, F_NONE, T_FUNC
			HELP("(quad qaddr -- okay?) try to store quadlet at qaddr") },

	{ "rb@", f_rbget, 0x230, F_NONE, T_FUNC
			HELP("(addr -- byte) fetch byte from device register at addr") },
	{ "rw@", f_rwget, 0x232, F_NONE, T_FUNC
			HELP("(waddr -- w) fetch doublet from device register at waddr") },
	{ "rl@", f_rlget, 0x234, F_NONE, T_FUNC
		HELP("(qaddr -- quad) fetch quadlet from device register at qaddr") },
	{ "rb!", f_rbset, 0x231, F_NONE, T_FUNC
			HELP("(byte addr --) store byte to device register at addr") },
	{ "rw!", f_rwset, 0x233, F_NONE, T_FUNC
			HELP("(w waddr --) store doublet to device register at waddr") },
	{ "rl!", f_rlset, 0x235, F_NONE, T_FUNC
			HELP("(quad qaddr --) store quadlet to device register at qaddr") },

	{ "get-msecs", f_getmsecs, 0x125, F_NONE, T_FUNC
			HELP("(-- n) return elapsed time in milliseconds") },
	{ "ms", f_ms, 0x126, F_NONE, T_FUNC
			HELP("(n --) delay for at least n milliseconds") },
	{ "alarm", f_alarm, 0x213, F_NONE, T_FUNC
			HELP("(xt n --) execute xt repeatedly every n milliseconds") },
	{ "user-abort", f_user_abort, 0x219, F_NONE, T_FUNC
			HELP("(... --) (R: ... --) after alarm routine finishes, abort") },

	{ "fcode-revision", f_fcode_rev, 0x87, F_NONE, T_FUNC
			HELP("(-- n) return revision level of FCode interface") },
	{ "mac-address", f_mac_address, 0x1A4, F_NONE, T_FUNC
			HELP("(-- mac-str mac-len)\n" \
					"\treturn sequence of bytes containing network address") },

	{ "display-status", f_display_status, 0x121, F_NONE, T_FUNC
			HELP("(n --) display the results of a device self-test") },
	{ "memory-test-suite", f_memory_test, 0x122, F_NONE, T_FUNC
			HELP("(addr len -- fail?) test len bytes of memory at addr") },
	{ "mask", (Command)offsetof(Environ, mask), 0x124, F_NONE, T_ADDR
			HELP("(-- a-addr) variable to control bits " \
					"tested with memory-test-suite") },

	{ "unaligned-w@", f_unalign_wget, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(waddr -- w) fetch doublet w from addr at any alignment") },
	{ "unaligned-w!", f_unalign_wset, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(w addr --) store doublet w to addr at any alignment") },
	{ "unaligned-l@", f_unalign_lget, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(waddr -- quad) fetch quadlet from addr at any alignment") },
	{ "unaligned-l!", f_unalign_lset, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(quad addr --) store quadlet to addr at any alignment") },

	{ "code", f_code, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(\"new-name< >\" -- code-sys) (E: ... -- ?)\n" \
					"\tbegin creation of machine-code command") },
	{ "label", f_label, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(\"new-name< >\" -- code-sys) (E: ... -- addr)\n" \
			"\tbegin creation of machine-code command - leave addr on stack") },
	{ "c;", f_csemicolon, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("end of machine-code command - return to caller") },
	{ "end-code", f_endcode, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("end of machine-code command - do not return to caller") },

	{ "help", f_help, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"{name}<eol>\" --) display help for name, if any") },

	{ NULL, NULL }
};
