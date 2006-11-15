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

/* (c) Copyright 1996-2004 by CodeGen, Inc.  All Rights Reserved. */

#include "defs.h"

/* SUN_OBP_COMPATIBILITY macro is defined three places below.
   It is to make init-program execute the loaded image (whatever it is)
   to be compatible with Sun's OBP.  Define this macro to turn on
   Sun OBP compliance.  Otherwise, it is IEEE-1275 compatible where
   init-program does not automatically execute anything but waits for go.
#define SUN_OBP_COMPATIBILITY
 */

Retcode
push_config_bool(Environ *e, char *param)
{
	IFCKSP(e, 0, 1);
	PUSH(e, get_config_bool(e, param, CSTR));
	return NO_ERROR;
}

Retcode
push_config_int(Environ *e, char *param)
{
	IFCKSP(e, 0, 1);
	PUSH(e, get_config_int(e, param, CSTR));
	return NO_ERROR;
}

Retcode
push_config_str(Environ *e, char *param)
{
	Byte *str;
	Int len;
	Retcode ret;

	IFCKSP(e, 0, 2);
	ret = get_config_val(e, param, CSTR, &str, &len);

	if (ret != NO_ERROR)
		return ret;

	if (str == NULL || len == 0)
	{
		str = "";
		len = 0;
	}

	PUSHP(e, str);
	PUSH(e, len);
	return NO_ERROR;
}

/* Generic functions to push bool/int/string params on the stack.
   These use the top of the return-stack to get the xtok of the word
   currently being executed, then retrieve its name to use to lookup
   the nvram parameter.
 */
CC(f_push_config_bool)
{
	Entry *ent = e->estk ? e->estk->code : NULL;

	if (ent == NULL)
		return E_ABORT;

	return push_config_bool(e, ent->name + 1);
}

CC(f_push_config_int)
{
	Entry *ent = e->estk ? e->estk->code : NULL;

	if (ent == NULL)
		return E_ABORT;

	return push_config_int(e, ent->name + 1);
}

CC(f_push_config_str)
{
	Entry *ent = e->estk ? e->estk->code : NULL;

	if (ent == NULL)
		return E_ABORT;

	return push_config_str(e, ent->name + 1);
}

Int
diagnostic_mode(Environ *e)
{
	Int diag;

	diag = get_config_bool(e, "diag-switch?", CSTR);

	if (diag == FFALSE)
		diag = machine_diag_switch(e) ? FTRUE : FFALSE;

	return diag;
}

C(f_diagnostic_mode)		/* diagnostic-mode? (-- diag?) 0x120 */
{
	IFCKSP(e, 0, 1);
	PUSH(e, diagnostic_mode(e));
	return NO_ERROR;
}

/* diag-switch? (-- diag?) */
/* boot-device (-- dev-str dev-len) */
/* boot-file (-- dev-str dev-len) */
/* diag-device (-- dev-str dev-len) */
/* diag-file (-- dev-str dev-len) */
/* auto-boot? (-- auto?) */
/* auto-boot-timeout (-- secs) [non-standard] */


/* 7.4.4.1  Configuration variables */

C(f_dsetenv)		/* $setenv (data-addr data-len name-str name-len --) */
{
	Byte *name, *data;
	Int nlen, dlen;
	Retcode ret;

	IFCKSP(e, 4, 0);
	POP(e, nlen);
	POPT(e, name, Byte*);
	POP(e, dlen);
	POPT(e, data, Byte*);

#ifndef SUN_COMPATIBILITY
	if (get_config(e, name, nlen) == NULL)
		return E_NO_CONFIG_VAR;
#endif

	if (e->security == SM_NVRAMRC &&
			compare_strs(name, nlen, "security-mode", CSTR))
	{
		cprintf(e, "Sorry - cannot set security-mode in nvramrc.\n");
		return E_ABORT;
	}

	/* do not save password under options */
	if (compare_strs(name, nlen, "security-password", CSTR))
		ret = set_nvram(e, name, nlen, data, dlen);
	else
		ret = save_config(e, name, nlen, data, dlen, TRUE);

	if (ret == NO_ERROR)
		ret = create_prop_alias(e, name, nlen);

	if (compare_strs(name, nlen, "security-mode", CSTR))
	{
		/* assume that if we got this far, user must have access */
		e->got_password = TRUE;
		e->security = security_mode(e);
		save_config(e, "security-#badlogins", CSTR, "0", CSTR, TRUE);
	}

	return ret;
}

C(f_setenv)			/* setenv ("nv-param< >new-value<eol>" --) */
{
	Byte *param, *val;
	Int plen, vlen;
	Retcode ret;

	ret = parse_word_len(e, &param, &plen);

	if (ret != NO_ERROR)
		return ret;

	if (param == NULL || *param == '\0' || plen == 0)
		return E_EMPTY_STRING;

	ret = parse_line_len(e, &val, &vlen);

	if (ret != NO_ERROR)
		return ret;

#ifndef SUN_COMPATIBILITY
	if (get_config(e, param, plen) == NULL)
		return E_NO_CONFIG_VAR;
#endif

	if (val == NULL || *val == '\0' || vlen == 0)
	{
		/* user wants to clear variable */
		cprintf(e, "%S = <null>\n", param, plen);
		val = "\0";
	}
	else
		cprintf(e, "%S = %S\n", param, plen, val, vlen);

	IFCKSP(e, 0, 4);
	PUSHP(e, val);
	PUSH(e, vlen);
	PUSHP(e, param);
	PUSH(e, plen);
	return f_dsetenv(e);
}

static void
print_nvramrc(Environ *e, Byte *str)
{
	int i = LINE_NUM_START;
	Byte *s;

	cprintf(e, "nvramrc\n");

	if (str == NULL)
		return;

	for (s = str; *s; s++)
		if (*s == '\n' || *s == '\r')
		{
			cprintf(e, "%5d: %S\n", i++, str, s - str);
			str = s + 1;
		}

	if (*str)
		cprintf(e, "%3d: %s\n", i, str);
}

C(f_printenv)		/* printenv ("{param-name}<eol>" --) */
{
	Byte *param;
	Int plen;
	Byte *str, *def;
	Entry *ent;
	Retcode ret;
	Byte name[STR_SIZE];
	
	ret = parse_line_len(e, &param, &plen);

	/* kill trailing white-space */
	while (param && plen > 0 && isspace(param[plen - 1]))
		plen--;

	if (param && *param && plen)
	{
		str = get_config(e, param, plen);

		if (str == NULL)
			return E_NO_CONFIG_VAR;

		def = get_nvram_default(e, param, plen);

		if (compare_strs(param, plen, "security-password", CSTR))
			cprintf(e, "%-21S ********\n", param, plen);
		else if (compare_strs(param, plen, "nvramrc", CSTR))
			print_nvramrc(e, str);
		else if (e->cols < 70)
			cprintf(e, "%-21S %s\n", param, plen, str);
		else
			cprintf(e, "%-21S %-27s (%s)\n", param, plen, str, def ? def : "");
	}
	else
	{
		if (e->cols < 70)
		{
			cprintf(e, "%-21s %-27s\n", "Variable", "Value");
			cprintf(e, "--------------------------------------------------\n");
		}
		else
		{
			cprintf(e, "%-21s %-27s (%s)\n", "Variable", "Value", "Default Value");
			cprintf(e, "-------------------------------------------------------"
					"-----------------------\n");
		}

		for (ent = e->options->props->list; ent; ent = ent->link)
		{
			if (compare_strs(ent->name, PSTR, "name", CSTR))
				continue;

			str = get_config(e, ent->name, PSTR);
			def = get_nvram_default(e, ent->name, PSTR);
			memcpy(name, ent->name + 1, *(uByte*)ent->name);
			name[*(uByte*)ent->name] = '\0';

			if (compare_strs(name, CSTR, "security-password", CSTR))
				cprintf(e, "%-21s ********\n", name);
			else if (compare_strs(name, CSTR, "nvramrc", CSTR))
				print_nvramrc(e, str);
			else if (e->cols < 70)
				cprintf(e, "%-21s %s\n", name, str);
			else
				cprintf(e, "%-21s %-27s (%s)\n", name, str, def ? def : "");
		}
	}
	
	return NO_ERROR;
}

C(f_set_default)	/* set-default ("param-name<eol>" --) */
{
	Byte *param, *val;
	Int plen, vlen;
	Retcode ret;

	ret = parse_word_len(e, &param, &plen);

	if (ret == NO_ERROR)
		ret = parse_line_len(e, &val, &vlen);

	if (param == NULL || *param == 0 || plen == 0)
		return E_EMPTY_STRING;

	if (compare_strs(param, plen, "security-password", CSTR) ||
			compare_strs(param, plen, "security-mode", CSTR) ||
			compare_strs(param, plen, "security-#badlogins", CSTR))
		return E_SECURE_CONFIG_VAR;

	val = get_nvram_default(e, param, plen);

	if (val == NULL)
		return E_NO_CONFIG_VAR;

	save_config(e, param, plen, val, CSTR, TRUE);
	return NO_ERROR;
}

C(f_set_defaults)		/* set-defaults (--) */
{
	Entry *ent;
	Byte *val;

	for (ent = e->options->props->list; ent; ent = ent->link)
	{
		if (compare_strs(ent->name, PSTR, "security-password", CSTR) ||
				compare_strs(ent->name, PSTR, "security-mode", CSTR) ||
				compare_strs(ent->name, PSTR, "security-#badlogins", CSTR))
			continue;

		val = get_nvram_default(e, ent->name, PSTR);

		if (val)
			save_config(e, ent->name, PSTR, val, CSTR, FALSE);
	}

	reset_nvram(e);
	return NO_ERROR;
}

/* create an alias word that points to a property */
Retcode
create_prop_alias(Environ *e, Byte *name, Int len)
{
	Entry *ent = find_table(e->names, name, len);

	if (ent)
		return NO_ERROR;

	ent = new_entry();

	if (ent == NULL)
		return E_OUT_OF_MEMORY;

	setstrlen(&name, &len);
	ent->name = lstrdup(name, len);

	if (ent->name == NULL)
		return E_OUT_OF_MEMORY;

	set_xtok(e, ent);			/* init the fcode value */
	ent->type = T_ALIAS;
	ent->v.alias = find_table(e->options->props, name, len);

	if (ent->v.alias == NULL)	/* shouldn't happen */
		return E_ABORT;

	/* and finally stick it in the table */
	if (!insert_table(e->names, ent))
		return E_TABLE_ERROR;

	return NO_ERROR;
}

/* nodefault-bytes (maxlen "new-name< >" --) (E: -- addr len) */
C(f_nodefault_bytes)
{
	Byte *name;
	Int maxlen;
	Byte *val;
	Retcode ret;

	IFCKSP(e, 1, 2);
	POP(e, maxlen);
	name = parse_word(e);

	if (*name == 0)
		return E_ABORT;

	/* get the NVRAM value for this new word and copy it in */
	val = new_nvram(e, name, PSTR, maxlen);

	/* add it to /options */
	ret = set_property(e->options->props, name, PSTR, val, maxlen);

	if (ret != NO_ERROR)
		return ret;

	return create_prop_alias(e, name, PSTR);
}


/* 7.4.4.2  The script */

C(f_nvramrc)		/* nvramrc (-- data-addr data-len) */
{
	Entry *ent = find_table(e->options->props, "nvramrc", CSTR);
	Byte *str;

	IFCKSP(e, 0, 2);

	if (ent == NULL)
		str = get_nvram(e, "nvramrc", CSTR);
	else
		str = ent->v.array;

	if (str == NULL)
		str = "";

	PUSHP(e, str);
	PUSH(e, strlen(str));
	return NO_ERROR;
}

/* use-nvramrc? (-- enabled?) */


/* 7.4.5 I/O control */

/* input-device (-- dev-str dev-len) */
/* output-device (-- dev-str dev-len) */

/* stdin and stdout are T_ADDR words pointing to e->keyboard and
	e->screen respectively */

/* screen-#columns (-- n) */
/* screen-#rows (-- n) */

static Retcode
do_console(Environ *e, char *method, Cell *device, Bool doabort,
		char *prop)
{
	Package *pkg;
	Int len;
	Retcode ret;
	Instance *inst;
	Byte dev[STR_SIZE];
	
	/* copy the device string into our temp buf in case it is
		overwritten later */
	IFCKSP(e, 2, 1);
	POP(e, len);
	strncpy(dev, (char*)(uPtr)TOP(e), len);
	dev[len] = '\0';
	DROP(e);
	
	/* first see if the device exists */
	pkg = find_device(e, dev, len);
	
	if (pkg == NULL)
		return E_NO_DEVICE;

	/* make sure it has the desired method */
	if (find_table(pkg->dict, method, CSTR) == NULL)
		return E_NO_METHOD;
	
	/* now call "open-dev" to actually open this device */
	PUSHP(e, dev);
	PUSH(e, len);
	ret = execute_word(e, "open-dev");
	
	if (ret != NO_ERROR)
		return ret;

	POPT(e, inst, Instance*);
	
	if (inst == NULL)
		return E_NO_DEVICE;
	
	if (doabort)
	{
		Package *savepkg = e->currpkg;

		e->currpkg = inst->package;
		execute_method_name(e, inst, "install-abort", CSTR);
		e->currpkg = savepkg;
	}
	
	if (*device)
	{
		/* close up the old device */
		if (doabort)
		{
			Package *savepkg = e->currpkg;
			Instance *i = (Instance*)(uPtr)*device;

			e->currpkg = i->package,
			execute_method_name(e, i, "remove-abort", CSTR);
			e->currpkg = savepkg;
		}
		
		PUSHP(e, *device);
		*device = (uPtr)inst;	/* so debug output will still work */
		(void)execute_word(e, "close-dev");
	}
	
	/* return the device instance */
	*device = (uPtr)inst;
	
	/* set the /chosen property name to this instance */
	return prop_set_ptr(e->chosen->props, prop, CSTR, (uPtr)inst);
}

C(f_input)			/* input (dev-str dev-len --) */
{
	return do_console(e, "read", &e->keyboard, TRUE, "stdin");
}

C(f_output)			/* output (dev-str dev-len --) */
{
	return do_console(e, "write", &e->screen, FALSE, "stdout");
}

C(f_io)				/* io (dev-str dev-len --) */
{
	Int len;
	Retcode ret;
	Byte dev[STR_SIZE];
	
	len = STACK(e, 0);
	strncpy(dev, (char*)(uPtr)STACK(e, 1), len);
	dev[len] = '\0';
	ret = execute_word(e, "output");

	if (ret != NO_ERROR)
		return ret;

	PUSHP(e, dev);
	PUSH(e, len);
	return execute_word(e, "input");
}

/* look for the first device of type "display" probed
 */
static Package *
find_node(Environ *e, Package *pkg, Byte *devtype)
{
	Package *p, *p2;
	Package *r = NULL;
	Retcode ret;
	Byte *type;
	Int tlen;

	if (pkg->props)
	{
		ret = prop_get_str(pkg->props, "device_type", CSTR, &type, &tlen);

		if (ret == NO_ERROR && compare_strs(type, tlen, devtype, CSTR))
			return pkg;
	}

	for (p = pkg->children; p; p = p->link)
	{
		p2 = find_node(e, p, devtype);

		if (p2)
			r = p2;
	}

	return r;
}

CC(f_install_console)	/* install-console (--) */
{
    Retcode ret;
    Byte str[STR_SIZE];
    Package *pkg;
    Package *savepkg = e->currpkg;

    DPRINTF(("f_install_console\n"));

    /* try to create a "screen" alias for the first device of type
       "display" we can find if there is no such alias */
    if (find_table(e->aliases->props, "screen", CSTR) == NULL)
    {
        DPRINTF(("f_install_console: no screen alias found\n"));
        pkg = find_node(e, e->root, "display");

        if (pkg && get_device_name(e, pkg, str))
        {
            DPRINTF(("f_install_console: found display device\n"));
            ret = make_devalias(e, "screen", CSTR, str, CSTR);

            if (ret != NO_ERROR)
                return ret;
        }
    }

    /* try to create a "keyboard" alias for the first device of type
       "keyboard" we can find if there is no such alias [non-standard] */
    if (find_table(e->aliases->props, "keyboard", CSTR) == NULL)
    {
        DPRINTF(("f_install_console: no keyboard alias found\n"));
        pkg = find_node(e, e->root, "keyboard");

        if (pkg && get_device_name(e, pkg, str))
        {
            DPRINTF(("f_install_console: found keyboard device\n"));
            ret = make_devalias(e, "keyboard", CSTR, str, CSTR);

            if (ret != NO_ERROR)
                return ret;
        }
    }

    DPRINTF(("f_install_console: setting output\n"));
    (void)execute_word(e, "output-device");
    ret = execute_word(e, "output");

    if (ret == NO_ERROR)
    {
        DPRINTF(("f_install_console: setting input\n"));
        (void)execute_word(e, "input-device");
        ret = execute_word(e, "input");
    }

    e->currpkg = savepkg;
    DPRINTF(("f_install_console: returning: %s (%d)\n", err2str(ret), ret));
    return ret;
}

/* 7.4.6 Security */
C(f_password)				/* password (--) */
{
	Byte pass1[STR_SIZE];
	Byte pass2[STR_SIZE];
	Int len1, len2;

	cprintf(e, "New password (up to 8 characters): ");
	get_password(e, pass1, STR_SIZE, &len1);
	cprintf(e, "Please type new password again: ");
	get_password(e, pass2, STR_SIZE, &len2);

	if (compare_strs(pass1, len1, pass2, len2))
	{
		cprintf(e, "security-password changed\n");
		save_config(e, "security-#badlogins", CSTR, "0", CSTR, TRUE);
		return set_nvram(e, "security-password", CSTR, pass1, len1);
	}
	else
		cprintf(e, "Passwords do not match - security-password unchanged\n");

	return NO_ERROR;
}

enum Security_mode
security_mode(Environ *e)
{
	Byte *str = get_config(e, "security-mode", CSTR);

	if (str)
		switch (*str)
		{
		case 'f':			/* full */
		case 'F':
			return SM_FULL;

		case 'c':			/* command */
		case 'C':
			return SM_COMMAND;

		case 'n':			/* none */
		case 'N':
		default:
			break;
		}

	return SM_NONE;
}

C(f_security_mode)			/* security-mode (-- n) */
{
	PUSH(e, security_mode(e));
	return NO_ERROR;
}

C(f_security_password)		/* security-password (-- passwd-str passwd-len) */
{
	Byte *str;

	IFCKSP(e, 0, 2);

	/* this is never stored under /options so read it straight from NVRAM */
	str = get_nvram(e, "security-password", CSTR);

	if (str == NULL)
		str = "";

	PUSHP(e, str);
	PUSH(e, strlen(str));
	return NO_ERROR;
}

/* security-#badlogins (-- n) */


/* 7.4.7  Reset */

C(f_reset_all)		/* reset-all (--) */
{
	return machine_reset_all(e);
}


/* 7.4.9  Client program callback */

/* $callback (argn ... arg1 nargs addr len -- retn ... ret2 Nreturns-1) */
C(f_dcallback)
{
	Byte *service;
	Int slen, i;
	Int nargs, nreturns;
	Cell *array, *returns;
	Retcode ret;

	DPRINTF(("$callback\n"));
	
	if (e->callback == NULL)
		return E_NO_CALLBACK;
		
	IFCKSP(e, 3, 1);
	POP(e, slen);
	POPT(e, service, Byte*);
	setstrlen(&service, &slen);
	POP(e, nargs);
	array = (Cell*)malloc((nargs + 20) * sizeof (Cell));

	DPRINTF(("$callback: service=%S array=%p nargs=%d\n",
			service, slen, array, nargs));
	
	if (array == NULL)
		return E_OUT_OF_MEMORY;
	
	array[0] = (uPtr)service;
	array[1] = nargs;
	array[2] = 15;		/* nreturns available for callback */
	
	for (i = 0; i < nargs; i++)
		POP(e, array[3 + i]);
	
	returns = array + 3 + nargs;
	DPRINTF(("$callback: about to call e->callback (%p) returns=%p\n",
			e->callback, returns));
	i = machine_callback(e, e->callback, array);
	DPRINTF(("$callback - e->callback returns %d\n", i));
	nreturns = array[2];
	ret = (Retcode)returns[0];

	DPRINTF(("$callback: service=%S array=%p nargs=%d nreturns=%d ret=%d\n",
			service, slen, array, nargs, nreturns, ret));
	
	if (ret == 0)
	{
		for (i = nreturns - 1; i >= 1; i--)
			PUSH(e, returns[i]);

		PUSH(e, nreturns - 1);
	}
	
	free(array);
	return ret;
}

C(f_callback)		/* callback ("service-name< >" "arguments<eol>" --) */
{
	Retcode ret;
	Int nret;
	Byte *service, *args;
	Int alen, slen;
		
	IFCKSP(e, 3, 2);
	ret = parse_word_len(e, &service, &slen);
	
	if (*service == 0)
		return E_EMPTY_STRING;

	if (ret == NO_ERROR)
		ret = parse_line_len(e, &args, &alen);

	if (ret != NO_ERROR)
		return ret;

	PUSHP(e, args);
	PUSH(e, alen);			/* nargs */
	PUSHP(e, service);
	PUSH(e, slen);
	ret = execute_word(e, "$callback");
	
	if (ret != NO_ERROR)
		return ret;
	
	POP(e, nret);
	
	if (nret != 1)
		return E_CALLBACK_ERROR;
	
	POPT(e, ret, Retcode);		/* return value */
	return ret;
}

C(f_sync)
{
	Int nret;
	Retcode ret;
	
	IFCKSP(e, 3, 2);
	PUSH(e, 0);		/* nargs */
	PUSH(e, "sync");
	PUSH(e, 4);
	ret = execute_word(e, "$callback");
	
	if (ret != NO_ERROR)
		return ret;
	
	POP(e, nret);
	
	if (nret != 1)
		return E_CALLBACK_ERROR;
	
	POPT(e, ret, Retcode);		/* return value */
	return ret;
}


/* 7.4.10  Banner */

Retcode
display_copyright(Environ *e, Int leading)
{
#ifdef CUSTOM_COPYRIGHT
	if (leading)
		cprintf(e, "\033[%dC", leading);

	cprintf(e, CUSTOM_COPYRIGHT);
#endif

	/* THE CODEGEN COPYRIGHT LINES MAY NOT BE DELETED FROM ANY FINAL IMAGE. */
	if (leading)
		cprintf(e, "\033[%dC", leading);

	cprintf(e, "SmartFirmware(tm) Copyright 1996-2001 by CodeGen, Inc.\n");

	if (leading)
		cprintf(e, "\033[%dC", leading);

#ifdef INCLUDE_BSD_COPYRIGHT
	cprintf(e, "All Rights Reserved.  Portions derived from code Copyright\n");

	if (leading)
		cprintf(e, "\033[%dC", leading);

	cprintf(e, "1992, 1993 by The Regents of the University of California.\n");
#else
	cprintf(e, "All Rights Reserved.\n");
#endif

	return NO_ERROR;
}

C(f_copyright)		/* copyright (--) [non-standard] */
{
	return display_copyright(e, 0);
}

CC(f_banner)		/* banner (--) */
{
	Instance *screen = (Instance*)(uPtr)e->screen;
	Bool oemlogo = FALSE;
	Byte *oem;
	Int res, i;
	Int leading = 0;
	Int width = e->logo_width;
	Int height = e->logo_height;
	Bool logo = FALSE;
	Byte *str;
	Int slen;

	e->no_banner = TRUE;	/* suppress default script */

	if (screen && screen->package &&
			prop_get_str(screen->package->props, "device_type",
					CSTR, &str, &slen) == NO_ERROR &&
			compare_strs(str, slen, "display", CSTR))
	{
		oemlogo = get_config_bool(e, "oem-logo?", CSTR);

		if (oemlogo)
		{
			width = 64;
			height = 64;
		}

		logo = width > 0 && height > 0;
		
		/* indent following text lines */
		if (logo)
#ifdef SUN_COMPATIBILITY
			leading = 128 / e->cwidth + 1;
#else
			leading = width / e->cwidth + 1;
#endif
	}

	if (leading)
		cprintf(e, "\033[%dC", leading);

	e->linesout = 0;
	res = get_config_bool(e, "oem-banner?", CSTR);

	if (res == FTRUE)
	{
		oem = get_config(e, "oem-banner", CSTR);

		if (oem)
			cprintf(e, "%s\n", oem);
	}
	else
	{
#ifdef MAIN_BANNER
		MAIN_BANNER
#else
#ifdef FIRMWARE_BUILD
		cprintf(e, "Welcome to SmartFirmware(tm) for %s %s version %s (%s)\n",
				MANUFACTURER, MACHINE_TYPE, FIRMWARE_REV, FIRMWARE_BUILD);
#else
		cprintf(e, "Welcome to SmartFirmware(tm) for %s %s version %s\n",
				MANUFACTURER, MACHINE_TYPE, FIRMWARE_REV);
#endif
#endif
	}

#ifndef NO_BANNER_COPYRIGHT
	(void)display_copyright(e, leading);
#endif

	if (logo)
	{
		extern Byte logo_pixmap[];

#ifdef SUN_COMPATIBILITY
		for (i = 128 / e->cheight + 1 - e->linesout; i > 0; i--)
#else
		for (i = height / e->cheight + 1 - e->linesout; i > 0; i--)
#endif
			display_char(e, '\n');

		IFCKSP(e, 0, 4);
		PUSH(e, e->curline - e->linesout);	/* previous line position */
		oem = logo_pixmap;

		if (oemlogo)
			(void)get_config_val(e, "oem-logo", CSTR, &oem, &i);

		PUSHP(e, oem);		/* addr */
		PUSH(e, width);		/* width */
		PUSH(e, height);	/* height */
		execute_method_name(e, screen, "draw-logo", CSTR);
	}

	return NO_ERROR;
}

C(f_suppress_banner)	/* suppress-banner (--) */
{
	e->no_banner = TRUE;
	return NO_ERROR;
}

/* oem-logo? (-- custom?) */

C(f_oem_logo)		/* oem-logo (-- logo-addr logo-len) */
{
	Entry *ent = find_table(e->options->props, "oem-logo", CSTR);
	Byte *str;

	IFCKSP(e, 0, 2);

	if (ent == NULL)
		str = get_nvram(e, "oem-logo", CSTR);
	else
		str = ent->v.array;

	PUSHP(e, str);
	PUSH(e, 512);
	return NO_ERROR;
}

/* oem-banner? (-- custom?) */
/* oem-banner (-- text-addr text-len) */


/* 7.4.11.3  Device probing */

CC(f_probe_all)		/* probe-all (--) */
{
	Retcode ret = machine_probe_all(e);
	
	/* no pre-selected package or instance */	
	e->currinst = (uPtr)NULL;
	e->currpkg = NULL;
	return ret;
}


/* 7.4.8  Self-test */

static Retcode
do_selftest(Environ *e, Byte *dev, Int len)
{
	Cell saveinst = e->currinst;
	Cell *saversp = e->rsp;
	Cell *savesp = e->sp;
	Retcode ret;
	Int val = FFALSE;
	Byte dbuf[STR_SIZE];

	/* could look for the device first, and make sure it has a "selftest"
	   method, but since exec_dev_method() has to do this anyway, may as
	   well let it do everything */
	machine_test_begin(e);

	/* make a temp copy for safety */
	setstrlen(&dev, &len);
	memcpy(dbuf, dev, len);
	dbuf[len] = '\0';
	dev = dbuf;

	PUSHP(e, dev);
	PUSH(e, len);
	PUSHP(e, "selftest");
	PUSH(e, 8);
	ret = execute_word(e, "execute-device-method");
	
	/* failure to exec or some other error */
	if (ret != NO_ERROR)
		goto done;

	/* get exec's return value */
	POP(e, val);
	
	if (val == FFALSE)
	{
		machine_test_fail(e);
		ret = E_NO_SELFTEST;
		goto done;
	}

	POP(e, val);		/* actual selftest's return code */

	if (val == NO_ERROR)
		machine_test_pass(e);
	else
#ifdef SUN_COMPATIBILITY
		cprintf(e, " Selftest failed. Return code = %d\n", val);
#else
		machine_test_fail(e);
#endif

done:
	if (ret != NO_ERROR)
	{
		e->sp = savesp;
		e->rsp = saversp;
		e->currinst = saveinst;
	}

	return (ret == E_USER_ABORT) ? ret : (Retcode)val;
}

C(f_test)			/* test ("device-specifier<eol>" --) */
{
	return do_selftest(e, parse_line(e), PSTR);
}

Retcode
do_testall(Environ *e, Package *node, Bool diag)
{
	Byte dev[STR_SIZE];
	Package *pkg;
	Retcode r;
	
	if (node == NULL)
		return NO_ERROR;
	
	/* get the name for this device, and run its "selftest" method, if any */
	if (!get_device_name(e, node, dev))
		return E_NO_SELFTEST;
	
	/* if it does not have a "selftest" node, do not bother with it */
#ifdef SUN_COMPATIBILITY
	if (find_table(node->dict, "selftest", CSTR) &&
			find_table(node->props, "reg", CSTR))
#else
	if (find_table(node->dict, "selftest", CSTR))
#endif
	{
#ifdef SUN_COMPATIBILITY
		cprintf(e, "Testing %s...\n", dev ? dev : "");

		r = do_selftest(e, dev, CSTR);

		if (r != NO_ERROR && r != E_NO_SELFTEST)
			return r;
#else
		if (diag)
			cprintf(e, "testing %s...\n", dev ? dev : "");
		else
			spin_cursor(e);

		r = do_selftest(e, dev, CSTR);
	
		if (r != NO_ERROR)
			return r;

		if (diag)
			cprintf(e, "successful!\n");
		else
			display_text(e, "\033[K", CSTR);
#endif
	}

	/* then test all its children that have selftest methods */
	for (pkg = node->children; pkg; pkg = pkg->link)
	{
		r = do_testall(e, pkg, diag);
		
#ifdef SUN_COMPATIBILITY
		if (r == E_USER_ABORT || r == R_END)
			return r;
#else
		if (r != NO_ERROR && r != E_NO_SELFTEST)
			return r;
#endif
	}
	
	return NO_ERROR;
}

C(f_test_all)		/* test-all ("{device-specifier}<eol>" --) */
{
	Byte *dev = parse_line(e);
	Int len = PSTR;
	Package *pkg;
	
	/* if there is something here, it must be a device */
	if (dev == NULL || *dev == 0)
	{
		dev = "/";
		len = 1;
	}
	
	pkg = find_device(e, dev, len);
	
	if (pkg == NULL)
		return E_NO_DEVICE;
	
	return do_testall(e, pkg, diagnostic_mode(e));
}

/* secondary-diag? (-- on?) [non-standard] */

CC(f_secondary_diag)	/* secondary-diag (--) [non-standard] */
{
	return machine_secondary_diag(e);
}


/* stuff to actually try to boot */

C(f_init_program)	/* init-program (--) */
{
	Retcode ret;
	
	e->state_valid = FFALSE;

	if (e->load == NULL)			/* sanity check */
		return E_BAD_LOAD_IMAGE;

	ret = machine_init_program(e);

	if (ret == NO_ERROR)
		e->state_valid = FTRUE;

#ifdef SUN_OBP_COMPATIBILITY
	if (!e->state_valid)
		return E_BAD_LOAD_IMAGE;

	if (ret == NO_ERROR)
	{
		e->go_running = TRUE;
		ret = machine_go(e);
		e->go_running = FALSE;
	}
#endif

	return ret;
}

C(f_go)				/* go (--) */
{
#ifdef SUN_OBP_COMPATIBILITY
	return f_init_program(e);
#else
	Retcode ret;

	if (!e->state_valid)
		return E_BAD_LOAD_IMAGE;

	e->go_running = TRUE;
	ret = machine_go(e);		/* don't expect this to return */
	e->go_running = FALSE;
	return ret;
#endif
}

/* called repeatedly by do_load below to do the open-dev and so on
 */
static Retcode
try_load(Environ *e, Int diag, Byte *dev, Int dlen,
		Byte *args, Int alen)
{
	Int len = 0;
	Instance *inst;
	Retcode ret;
	Byte str[STR_SIZE];

	DPRINTF(("try_load: dev=\"%s\" dlen=%d args=\"%s\" alen=%d\n",
			dev, dlen, args, alen));

	if (!diag)
		spin_cursor(e);

	/* setup loadargs for convenience of "load" and other C methods */
	e->loadargs = args;

	/* try to open this device */
	PUSHP(e, dev);
	PUSH(e, dlen);
	ret = execute_word(e, "open-dev");
	
	if (ret != NO_ERROR)
		return ret;

	POPT(e, inst, Instance*);
	
	if (inst == NULL)
		return E_BOOT_ERROR;

	/* set the /chosen properties to the selected values */
	if (get_device_name(e, inst->package, str))
	{
	#ifdef SUN_COMPATIBILITY
		/* need to append unit number and instance arguments to the
		   bootpath for Sun bootloaders
		 */
		Byte *s = strrchr(str, '/');

		if (s)
		{
			s = strchr(s, '@');

			/* no '@' in last node returned by get_device_name,
			   so append unit number of instance to device name
			 */
			if (s == NULL)
			{
				Int n;
				Byte buf[STR_SIZE];

				strcat(str, "@");

				/* first push the instance unit numbers on the stack */
				for (n = 0; n < inst->numunits; n++)
					PUSH(e, inst->unit[n]);

				/* then try the parent's encode-unit method */
				if (execute_static_method_name(e, inst->parent->package,
						"encode-unit", CSTR) == NO_ERROR)
				{
					/* success - pop off and append the encoded string */
					POP(e, n);
					POPT(e, s, Byte*);
					strncat(str, s, n);
				}
				else
				{
					n = inst->numunits;
					DROPN(e, n);			/* drop our pushed numbers */
					bprintf(buf, "%Cx", inst->unit[--n]);
					strcat(str, buf);

					while (n--)
					{
						bprintf(buf, ",%Cx", inst->unit[n]);
						strcat(str, buf);
					}
				}
			}
		}

		/* append any instance arguments to bootpath */
		if (inst->args && *inst->args)
		{
			s = strrchr(s, '/');

			if (s)
				s = strchr(s, ':');

			strcat(str, s ? "," : ":");
			strcat(str, inst->args + 1);
		}

		DPRINTF(("try_load: dev=\"%s\" args=\"%s\" str=\"%s\"\n",
				dev, args, str));

	#endif	/* SUN_COMPATIBILITY */

		ret = prop_set_str(e->chosen->props, "bootpath", CSTR, str, CSTR);

		if (diag)
			cprintf(e, "Boot path: %s", str);
	}
	else
	{
		ret = prop_set_str(e->chosen->props, "bootpath", CSTR, dev, dlen);

		if (diag)
			cprintf(e, "Boot path: %S", dev, dlen);
	}
	
	if (ret == NO_ERROR)
		ret = prop_set_str(e->chosen->props, "bootargs", CSTR, args, alen);
	
	if (diag)
		cprintf(e, "  Boot args: %S\n", args, alen);
	else
		spin_cursor(e);

	/* this should initialize e->load to some reasonable address */
	ret = machine_init_load(e);

	if (ret != NO_ERROR)
		return ret;

	if (e->load == NULL)
		return E_OUT_OF_MEMORY;

	e->state_valid = FFALSE;
	PUSHP(e, e->load);		/* try to load it at this magic addr */
	ret = execute_method_name(e, inst, "load", CSTR);
	e->loadargs = NULL;

	if (ret == NO_ERROR)
		POP(e, len);		/* length of code loaded into memory */

	/* and close the device */
	PUSHP(e, inst);
	(void)execute_word(e, "close-dev");

	/* now return the load failure, if any */
	if (ret != NO_ERROR)
		return ret;

	if (len)
		e->loadlen = len;
	else
		ret = E_BOOT_ERROR;

	/* clear the spinning cursor */
	if (!diag)
		cprintf(e, " \b");

#ifndef SUN_OBP_COMPATIBILITY
	if (ret == NO_ERROR)
		ret = execute_word(e, "init-program");
#endif

	return ret;
}

Retcode
do_load(Environ *e, Byte *dev, Int dlen, Byte *args, Int alen)
{
	Int len;
	Int diag = diagnostic_mode(e);
	Retcode ret;
	Byte *s;
	Byte alstr[STR_SIZE], str[STR_SIZE], astr[STR_SIZE], dstr[STR_SIZE];

	setstrlen(&dev, &dlen);
	setstrlen(&args, &alen);
	DPRINTF(("do_load: dev=\"%S\" dlen=%d args=\"%S\" alen=%d\n",
			dev, dlen, dlen, args, alen, alen));
	memcpy(alstr, dev, dlen);
	alstr[dlen] = '\0';
	s = strchr(alstr, ':');

	if (s)
		*s = '\0';

	ret = prop_get_str(e->aliases->props, alstr, strlen(alstr), &s, &len);
	memcpy(alstr + 1, s, len);
	alstr[len + 1] = '\0';
	*(uByte*)alstr = len;
	DPRINTF(("do_load: alstr=\"%P\"\n", (ret == NO_ERROR) ? alstr : "\0"));

	if (!diag)
		spin_cursor(e);

	/* if the device does not start with a / and is not an alias,
		use the default device */
	if (!*dev || (*dev != '/' && (ret != NO_ERROR || *alstr == 0)))
	{
		/* prepend device to the front of args */
		strncpy(str, dev, dlen);

		if (alen)
		{
			str[dlen] = ' ';
			strncpy(str + dlen + 1, args, alen);
			alen += dlen + 1;
			str[alen] = '\0';
		}
		else
		{
			str[dlen] = '\0';
			alen = dlen;
		}

		args = str;

		/* now get the default boot device */
		dev = get_config(e, diag ? "diag-device" : "boot-device", CSTR);
		dlen = CSTR;
	}

	/* make copies of dev that does not use temp bufs */
	setstrlen(&dev, &dlen);
	memcpy(dstr, dev, dlen);
	dstr[dlen] = '\0';
	dev = dstr;
	
	/* skip whitespace so we can see if there is anything there */
	for (; alen > 0 && isblank(*args); alen--)
		args++;

	/* no args - use the defaults */
	if (alen == 0)
	{
		args = get_config(e, diag ? "diag-file" : "boot-file", CSTR);
		alen = CSTR;
	}

	/* make copies of args that does not use temp bufs */
	setstrlen(&args, &alen);
	memcpy(astr, args, alen);
	astr[alen] = '\0';
	args = astr;

	if (!diag)
		spin_cursor(e);

	len = 0;

	/* nvram string could contain multiple devices, so try them all
	   one by one - note there is no way to have multiple devices on
	   the command-line, but if there were, this should still work
	 */
	do
	{
		/* skip leading blanks */
		for (; isblank(*dev) && dlen > 0; dev++)
			dlen--;

		s = dev;
		len = dlen;

		/* collect non-blanks */
		for (; !isblank(*s) && len > 0; s++)
			len--;

		ret = try_load(e, diag, dev, dlen - len, args, alen);

		if (ret == NO_ERROR)
			break;

		/* try the next one */
		dev = s;
		dlen = len;
	} while (dlen > 0);

	return ret;
}

Retcode
boot_load(Environ *e, Byte *dev, Int dlen, Byte *args, Int alen)
{
	Retcode ret;
	
	ret = do_load(e, dev, dlen, args, alen);

	return ret == NO_ERROR ? execute_word(e, "go") : ret;
}

C(f_load)			/* load ("{param-text}<eol>" --) */
{
	Int dlen = PSTR;
	Int alen = PSTR;
	Byte *dev = parse_word(e);
	Byte *args = "\0";

	/* if there is something here, there may be more after it */
	if (dev && *dev)
		args = parse_line(e);
	else
		dev = args;

	/* command mode only allows boot without any arguments - rest
	   are handled by interpreter loop in exec.c */
	if (dev && *dev && e->security == SM_COMMAND &&
			!e->got_password && !check_password(e))
		return E_ABORT;

	return do_load(e, dev, dlen, args, alen);
}

/* 7.4.3.5  User commands for booting */

C(f_boot)			/* boot ("{param-text}<cr>" --) */
{
	Retcode ret;
	
	if ((ret = execute_word(e, "load")) != NO_ERROR)
		return ret;

	return execute_word(e, "go");
}

static Retcode
do_set_mac_address(Environ *e, Byte *dev, Int dlen, Byte *args, Int alen)
{
	extern Bool parse_mac_addr(Environ *e, Byte **str, uByte mac_addr[],
			const char *errstr);
	Retcode ret;
	Retcode ret2;
#define MAC_ADDR_SIZE 6
	uByte mac_addr[MAC_ADDR_SIZE];
	Instance *inst;
	Instance *saveinst = (Instance *)(uPtr)e->currinst;
	extern Bool parse_mac_addr(Environ *e, Byte **str, uByte mac_addr[], const char *errstr);

	setstrlen(&dev, &dlen);
	setstrlen(&args, &alen);

	IFCKSP(e, 0, 2);
	PUSHP(e, dev);
	PUSH(e, dlen);
	DPRINTF(("do_set_mac_address: open-dev\n"));

	ret = execute_word(e, "open-dev");

	DPRINTF(("do_set_mac_address: open-dev: ret = %d\n", ret));
	IFCKSP(e, 1, 0);
	POPT(e, inst, Instance*);
	
	if ((inst == NULL || ret != NO_ERROR) && alen == 0)
	{
		args = dev;
		alen = dlen;

		PUSH(e, "net");
		PUSH(e, 3);
		DPRINTF(("do_set_mac_address: open-dev\n"));

		ret = execute_word(e, "open-dev");

		DPRINTF(("do_set_mac_address: open-dev: ret = %d\n", ret));
		IFCKSP(e, 1, 0);
		POPT(e, inst, Instance*);
	}

	DPRINTF(("do_set_mac_address: inst = %p\n", inst));

	if (ret != NO_ERROR)
	{
		cprintf(e, "Unable to open device %s\n", dev);
		return ret;
	}

	if (find_table(inst->package->dict, "set-mac-addr", CSTR) == NULL)
	{
		cprintf(e, "Device %s does not support setting the mac address\n", dev);
		cprintf(e, "Try setting the NVRAM variable mac-address with the setenv command\n");
		return E_NO_METHOD;
	}

	if (!parse_mac_addr(e, &args, mac_addr, "set-mac-address:\n"))
	{
		cprintf(e, "Unable to parse the MAC address, %s\n", args);
		return E_BAD_ARGUMENT;
	}

	e->currinst = (uPtr)inst;
	PUSHP(e, mac_addr);
	PUSH(e, MAC_ADDR_SIZE);
	ret = execute_method_name(e, inst, "set-mac-addr", CSTR);
	e->currinst = (uPtr)saveinst;

	PUSHP(e, inst);
	ret2 = execute_word(e, "close-dev");
	return ret != NO_ERROR ? ret : ret2;
}

/* CodeGen non-standard */
C(f_set_mac_address)		/* set-mac-address ("{device} {mac-address}<eol> [non-standard]" --) */
{
	Int dlen = PSTR;
	Int alen = PSTR;
	Byte *dev = parse_word(e);
	Byte *args = "\0";

	/* if there is something here, there may be more after it */
	if (dev == NULL || *dev == 0)
		return E_EMPTY_STRING;
	
	args = parse_line(e);
	return do_set_mac_address(e, dev, dlen, args, alen);
}


const Initentry init_admin[] =
{
	{ "diagnostic-mode?", f_diagnostic_mode, 0x120, F_NONE, T_FUNC
			HELP("(-- diag?)\n" \
			"\tif true, boot from diag sources and perform longer selftests") },

	{ "diag-switch?", f_push_config_bool, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- diag?) if true, diagnostic-mode? returns true") },
	{ "boot-device", f_push_config_str, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- dev-str dev-len)\n" \
					"\tdefault boot device-name (diagnostic-mode? false)") },
	{ "boot-file", f_push_config_str, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- dev-str dev-len) default boot arguments " \
					"(diagnostic-mode? false)") },
	{ "diag-device", f_push_config_str, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- dev-str dev-len)\n" \
					"\tdefault boot device-name (diagnostic-mode? true)") },
	{ "diag-file", f_push_config_str, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- dev-str dev-len) default boot arguments " \
					"(diagnostic-mode? true)") },
	{ "boot-command", f_push_config_str, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- auto?)\n" \
			"\tthe command to execute after power-up if auto-boot? is set") },
	{ "auto-boot?", f_push_config_bool, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- auto?)\n" \
			"\tif true, automatically execute boot-command after power-up") },
	{ "auto-boot-timeout", f_push_config_int, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- secs)\n" \
		"\tnumber of seconds to wait for a key before auto-boot at power-up") },
			/* [non-standard] */

	{ "setenv", f_setenv, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"nv-param< >new-value<eol>\"\n" \
				"\tset configuration variable nv-param to the new-value") },
	{ "$setenv", f_dsetenv, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(data-addr data-len name-str name-len --)\n" \
					"\tset named configuration variable to the new data") },
	{ "printenv", f_printenv, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(\"{param-name}<eol>\" --)\n" \
			"\tdisplay current and default values of param-name (or all)") },
	{ "set-default", f_set_default, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"param-name<eol>\" --)\n" \
					"\tset confuration variable to its default value") },
	{ "set-defaults", f_set_defaults, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) reset more configuration variables " \
					"to their default values") },
	{ "nodefault-bytes", f_nodefault_bytes, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(maxlen \"new-name< >\" --) (E: -- addr len)\n" \
					"\tcreate custom configuration variable of size maxlen") },

	{ "nvramrc", f_nvramrc, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- data-addr data-len) contents of the NVRAM script") },
	{ "use-nvramrc?", f_push_config_bool, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- enabled?) if true, nvramrc is evaluated at start-up") },

	{ "input-device", f_push_config_str, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- dev-str dev-len) default console input device") },
	{ "output-device", f_push_config_str, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- dev-str dev-len) default console output device") },
	{ "stdin", (Command)offsetof(Environ, keyboard),
			INVALID_FCODE, F_NONE, T_ADDR
			HELP("(-- a-addr) " \
					"variable containing ihandle of console input device") },
	{ "stdout", (Command)offsetof(Environ, screen),
			INVALID_FCODE, F_NONE, T_ADDR
			HELP("(-- a-addr) " \
					"variable containing ihandle of console output device") },
	{ "screen-#columns", f_push_config_int, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- n) number of columns on console output device") },
	{ "screen-#rows", f_push_config_int, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- n) number of rows on console output device") },
	{ "lines/page", (Command)offsetof(Environ, linesperpage), INVALID_FCODE,
			F_NONE, T_ADDRVAL
			HELP("(-- #lines)\n" \
					"\tnumber of lines per page for auto-pagination\n" \
					"\tmay be negative which is then added to #lines") },

	{ "input", f_input, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(dev-str dev-len --) select device for console input") },
	{ "output", f_output, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(dev-str dev-len --) select device for console output") },
	{ "io", f_io, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(dev-str dev-len --) select device for console " \
					"input and output") },
	{ "install-console", f_install_console, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) select and activate console input and output devices") },

	{ "password", f_password, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) prompt user to set security password") },
	{ "security-mode", f_security_mode, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- n) current level of security access protection") },
	{ "security-password", f_security_password, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- passwd-str passwd-len) current password string") },
	{ "security-#badlogins", f_push_config_int, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- n) count of invalid security access attempts") },

	{ "reset", f_reset_all, INVALID_FCODE, F_NONE, T_FUNC
		HELP("(--) historic alias to reset-all") },	/* synonym */
	{ "reset-all", f_reset_all, INVALID_FCODE, F_NONE, T_FUNC
		HELP("(--) reset the machine as if a power-on reset had occurred") },

	{ "callback", f_callback, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"service-name< >\" \"arguments<eol>\" --)\n"
					"\texecute specified client program callback routine") },
	{ "$callback", f_dcallback, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(argn...arg1 nargs addr len -- retn...ret2 Nreturns-1)\n" \
					"\texecute specified client program callback routine") },
	{ "sync", f_sync, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) flush system file buffers after a program interrupt") },

	{ "copyright", f_copyright, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) display system copyright information") },
	{ "banner", f_banner, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) display system power-on banner") },
	{ "suppress-banner", f_suppress_banner, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) do not display banner after running nvramrc script") },
	{ "oem-logo?", f_push_config_bool, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- custom?) if true, banner displays " \
					"custom logo in oem-logo") },
	{ "oem-logo", f_oem_logo, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- logo-addr logo-len) custom logo for banner, " \
					"enabled by oem-logo? ") },
	{ "oem-banner?", f_push_config_bool, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- custom?) if true, banner displays custom " \
					"message in oem-banner") },
	{ "oem-banner", f_push_config_str, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- text-addr text-len) custom banner text, " \
					"enabled by oem-banner?") },

	{ "probe-all", f_probe_all, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) probe for all available plug-in devices") },

	{ "test", f_test, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"device-specifier<eol>\" --)\n" \
					"\tinvoke the selftest method for the specified device") },
	{ "test-all", f_test_all, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"{device-specifier}<eol>\" --)\n" \
					"\tinvoke selftest methods at and below specified node") },
	{ "secondary-diag?", f_push_config_bool, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- on?) if true, secondary-diag is turned on at power-up") },
			/* [non-standard] */
	{ "secondary-diag", f_secondary_diag, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) invoke secondary diagnostics") },
			/* [non-standard] */

	{ "state-valid", (Command)offsetof(Environ, state_valid),
			INVALID_FCODE, F_NONE, T_ADDR
			HELP("(-- a-addr)\n" \
			"\tvariable containing true if saved-program-state is valid ") },
	{ "init-program", f_init_program, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) initialize saved-program-state") },
	{ "go", f_go, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) execute or resume execution of a program in memory") },
	{ "load", f_load, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"{param-text}<eol>\" --) " \
					"load a program specified by params") },
	{ "boot", f_boot, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"{param-text}<cr>\" --) " \
					"load and execute a program specified by params") },
	{ "set-mac-address", f_set_mac_address, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"{device} {mac-address}<eol> [non-standard]\" --)\n" \
					"\tSet the interface specific network address") },

	{ "status", (Command)0x7B /* noop */, INVALID_FCODE, F_NONE, T_DEFER
			HELP("(--) defer word to modify user-interface prompt") },

	{ NULL, NULL }
};
