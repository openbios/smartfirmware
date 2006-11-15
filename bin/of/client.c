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

/* client interface, updated by extensible client services extensions #215 */

/*#define DEBUG*/

#include "defs.h"


/* 6.3.2.1  Client interface */

C(f_client_test)		/* test (method-cstr -- missing?) */
{
	extern const Initentry client_services_methods[];
	const Initentry *method;
	Byte *name;
	Int ret = FTRUE;
	
	IFCKSP(e, 1, 1);
	POPT(e, name, Byte*);

	if (e->client)		/* new client-services installed */
	{
		if (find_table(e->client->dict, name, CSTR))
			ret = FFALSE;
	}
	else		/* search our init-list for builtin client interface words */
	{
		for (method = client_services_methods; method->name; method++)
			if (compare_strs(name, CSTR, method->name, CSTR))
			{
				ret = FFALSE;
				break;
			}
	}

	PUSH(e, ret);
	return NO_ERROR;
}

/* additional method from proposal/extension #270 */
C(f_client_test_method)		/* test-method (method-cstr phandle -- missing?) */
{
	Package *pkg;
	Byte *name;
	Int ret = FTRUE;
	
	IFCKSP(e, 2, 1);
	POPT(e, pkg, Package*);
	POPT(e, name, Byte*);

	if (pkg && find_table(pkg->dict, name, CSTR))
		ret = FFALSE;

	PUSH(e, ret);
	return NO_ERROR;
}


/* 6.3.2.2  Device tree */

/* defined in packages.c */
EC(f_peer);		/* peer (phandle -- phandle.sibling) 0x23C */
EC(f_child);	/* child (phandle.parent -- phandle.child) 0x23B */

C(f_client_parent)			/* parent (phandle -- parent-phandle) */
{
	Package *pkg;

	IFCKSP(e, 1, 1);
	POPT(e, pkg, Package*);

	if (pkg)
		pkg = pkg->parent;

	PUSHP(e, pkg);
	return NO_ERROR;
}

C(f_client_instance_to_package)	/* instance-to-package (ihandle -- phandle) */
{
	Instance *inst;
	
	IFCKSP(e, 1, 1);
	POPT(e, inst, Instance*);

	if (inst && inst->package)
		PUSHP(e, inst->package);
	else
		PUSH(e, -1);

	return NO_ERROR;
}

C(f_client_getproplen)	/* getproplen (name-cstr phandle -- prop-len) */
{
	Package *pkg;
	Byte *name;
	Entry *ent;

	IFCKSP(e, 2, 1);
	POPT(e, pkg, Package*);
	POPT(e, name, Byte*);
	
	ent = find_table(pkg->props, name, CSTR);

	PUSH(e, (ent) ? ent->len : -1);
	return NO_ERROR;
}

C(f_client_getprop)		/* getprop (len addr name-cstr phandle -- size) */
{
	Package *pkg;
	Byte *name;
	Byte *buf;
	Int buflen;
	Entry *ent;
	
	IFCKSP(e, 4, 1);
	POPT(e, pkg, Package*);
	POPT(e, name, Byte*);
	POPT(e, buf, Byte*);
	POP(e, buflen);
	DPRINTF(("client-getprop: pkg=%p name=%s buf=%p buflen=%d\n", pkg, name, buf, buflen));

	ent = find_table(pkg->props, name, CSTR);
	
	if (ent)
	{
		DPRINTF(("client-getprop: ent=%p array=%p len=%d\n", ent, ent->v.array, ent->len));
		PUSH(e, ent->len);
		memcpy(buf, ent->v.array, (buflen > ent->len) ? ent->len : buflen);

		if (ent->len >= 16)
		{
			DPRINTF(("client-getprop: array %#x %#x %#x %#x\n",
					((uInt *)ent->v.array)[0], ((uInt *)ent->v.array)[1],
					((uInt *)ent->v.array)[2], ((uInt *)ent->v.array)[3]));
		}

		if (buflen >= 16)
		{
			DPRINTF(("client-getprop: buf %#x %#x %#x %#x\n",
					((uInt *)buf)[0], ((uInt *)buf)[1],
					((uInt *)buf)[2], ((uInt *)buf)[3]));
		}
	}
	else
		PUSH(e, -1);

	return NO_ERROR;
}

C(f_client_nextprop)	/* nextprop (buf-addr prev-cstr phandle -- size) */
{
	Package *pkg;
	Byte *name;
	Byte *buf;
	Entry *ent;
	
	IFCKSP(e, 3, 1);
	POPT(e, pkg, Package*);
	POPT(e, name, Byte*);
	POPT(e, buf, Byte*);

	if (pkg == NULL || pkg->props == NULL)
		ent = e->root->props->list;
	else if (name == NULL || *name == '\0')
		ent = pkg->props->list;
	else
		ent = find_table(pkg->props, name, CSTR);
	
	if (ent)
	{
		Int len = 0;

		PUSH(e, (ent->link) ? 1 : 0);

		if (name && *name != '\0')
			ent = ent->link;

		if (ent)
			len = *(uByte*)ent->name;

		/* 32 is max len specified by OF spec */
		if (len > 31)
			len = 31;

		strncpy(buf, ent->name + 1, len);
		buf[len] = '\0';
	}
	else
		PUSH(e, -1);

	return NO_ERROR;
}

C(f_client_setprop)		/* setprop (len addr name-cstr phandle -- size) */
{
	Package *pkg;
	Byte *name;
	Int len;
	Byte *buf, *newbuf;

	IFCKSP(e, 4, 1);
	POPT(e, pkg, Package*);
	POPT(e, name, Byte*);
	POPT(e, buf, Byte*);
	POP(e, len);
	newbuf = prop_alloc(e, len);

	if (newbuf && buf && pkg && pkg->props &&
			add_property(pkg->props, name, CSTR, newbuf, len) == NO_ERROR)
	{
		memcpy(newbuf, buf, len);
		PUSH(e, len);
	}
	else
		PUSH(e, -1);

	return NO_ERROR;
}

C(f_client_canon)		/* canon (len addr device-cstr -- length) */
{
	Byte *device;
	Byte *buf;
	Int len;
	Package *pkg;
	Byte strbuf[STR_SIZE];
	
	IFCKSP(e, 3, 1);
	POPT(e, device, Byte*);
	POPT(e, buf, Byte*);
	POP(e, len);

	pkg = find_device(e, device, CSTR);

	if (pkg != NULL && get_device_name(e, pkg, strbuf))
	{
		strncpy(buf, strbuf, len);
		PUSH(e, strlen(strbuf));
	}
	else
		PUSH(e, -1);

	return NO_ERROR;
}

C(f_client_finddevice)	/* finddevice (device-cstr -- phandle) */
{
	Byte *device;
	Package *pkg;
	
	IFCKSP(e, 1, 1);
	POPT(e, device, Byte*);
	DPRINTF(("client-finddevice: device=%s\n", device));

	pkg = find_device(e, device, CSTR);

	if (pkg)
		PUSHP(e, pkg);
	else
		PUSH(e, -1);

	return NO_ERROR;
}

/* instance-to-path (buf-len buf-addr ihandle -- length) */
C(f_client_instance_to_path)
{
	Instance *inst;
	Byte *buf;
	Int buflen;
	Byte str[STR_SIZE];

	IFCKSP(e, 3, 1);
	POPT(e, inst, Instance*);
	POPT(e, buf, Byte*);
	POP(e, buflen);
	
	if (inst && buf && get_device_name(e, inst->package, str))
	{
		/* append any instance arguments to the path */
		if (inst->args && *inst->args)
		{
			Byte *s = strrchr(str, '/');

			if (s)
				s = strchr(s, ':');

			strcat(str, s ? "," : ":");
			strcat(str, inst->args + 1);
		}

		strncpy(buf, str, buflen);
		PUSH(e, strlen(str));
	}
	else
		PUSH(e, -1);

	return NO_ERROR;
}

/* instance-to-interposed-path (buf-len buf-addr ihandle -- length) */
C(f_client_instance_to_interposed_path)
{
	Instance *inst;
	Byte *buf;
	Int buflen;
	Byte str[STR_SIZE];

	IFCKSP(e, 3, 1);
	POPT(e, inst, Instance*);
	POPT(e, buf, Byte*);
	POP(e, buflen);
	
	if (inst && buf && get_interposed_device_name(e, inst, str))
	{
		strncpy(buf, str, buflen);
		PUSH(e, strlen(str));
	}
	else
		PUSH(e, -1);

	return NO_ERROR;
}

/* package-to-path (buf-len buf-addr phandle -- length) */
C(f_client_package_to_path)
{
	Package *pkg;
	Byte *buf;
	Int buflen;
	Byte str[STR_SIZE];

	IFCKSP(e, 3, 1);
	POPT(e, pkg, Package*);
	POPT(e, buf, Byte*);
	POP(e, buflen);
	
	if (pkg && buf && get_device_name(e, pkg, str))
	{
		strncpy(buf, str, buflen);
		PUSH(e, strlen(str));
	}
	else
		PUSH(e, -1);

	return NO_ERROR;
}

/* call-static-method (... phandle method-cstr -- ... catch-result) */
C(f_client_call_static_method)
{
	Byte *method;
	Package *pkg;
	Retcode ret;
	
	IFCKSP(e, 2, 1);
	POPT(e, method, Byte*);
	POPT(e, pkg, Package*);
	
	if (pkg == NULL)
		return E_NULL_PACKAGE;
	
	ret = execute_static_method_name(e, pkg, method, CSTR);
	
	PUSH(e, ret);
	return NO_ERROR;
}

/* call-method (... ihandle method-cstr -- ... catch-result) */
C(f_client_call_method)
{
	Byte *method;
	Instance *inst;
	Retcode ret;
	
	IFCKSP(e, 2, 1);
	POPT(e, method, Byte*);
	POPT(e, inst, Instance*);
	
	if (inst == NULL)
		return E_NULL_INSTANCE;
	
	ret = execute_method_name(e, inst, method, CSTR);
	
	PUSH(e, ret);
	return NO_ERROR;
}


/* 6.3.2.3  Device I/O */

C(f_client_open)		/* open (device-cstr -- ihandle) */
{
	Byte *device;
	CC(f_open_dev);

	IFCKSP(e, 1, 1);
	device = (Byte*)(uPtr)TOP(e);
	PUSH(e, strlen(device));
	return f_open_dev(e);
}

/* defined in device.c */
EC(f_close_dev);		/* close-dev (ihandle --) */

static Retcode
do_method(Environ *e, Byte *method)
{
	Instance *inst;
	Byte *addr;
	Int len;
	Retcode ret;

	IFCKSP(e, 3, 1);
	POPT(e, inst, Instance*);
	POPT(e, addr, Byte*);
	POP(e, len);
	DPRINTF(("client: %s inst=%p addr=%p len=%d\n", method, inst, addr, len));

	PUSHP(e, addr);
	PUSH(e, len);
	ret = execute_method_name(e, inst, method, CSTR);

	if (ret == E_USER_ABORT)	
		ret = execute_word(e, "interact");

	return ret;
}

C(f_client_read)		/* read (len addr ihandle -- actual) */
{
	return do_method(e, "read");
}

C(f_client_write)		/* write (len addr ihandle -- actual) */
{
	return do_method(e, "write");
}

C(f_client_seek)		/* seek (pos.lo pos.hi ihandle -- status) */
{
	Instance *inst;

	IFCKSP(e, 3, 1);
	POPT(e, inst, Instance*);

	return execute_method_name(e, inst, "seek", CSTR);
}


/* 6.3.2.4  Memory */

static Retcode
do_mem_method(Environ *e, Byte *prop, Byte *method)
{
	Int inst;
	
	if (prop_get_int(e->chosen->props, prop, CSTR, &inst) != NO_ERROR ||
			inst == 0)
		return E_NULL_INSTANCE;
		
	return execute_method_name(e, (Instance*)(uPtr)inst, method, CSTR);
}

C(f_client_claim)		/* claim (align size virt -- baseaddr) */
{
	Cell virt;
	Cell base;
	Int size, align;
	Int mode = 0xC;
	Retcode ret;

	IFCKSP(e, 3, 4);
	POP(e, virt);
	POP(e, size);
	POP(e, align);

	DPRINTF(("client_claim: virt 0x%X size %d align %d\n", virt, size, align));

	/* claim size byte of physical memory */
	PUSH(e, size);
	PUSH(e, align >= 4096 ? align : 4096);
	ret = do_mem_method(e, "memory", "claim");
	POP(e, base);

	DPRINTF(("client_claim: base 0x%X\n", base));

	/* claim size bytes of address space */

	if (align == 0)
		PUSH(e, virt);

	PUSH(e, size);
	PUSH(e, align);
	ret = do_mem_method(e, "mmu", "claim");
	POP(e, virt);

	DPRINTF(("client_claim: new virt 0x%X\n", virt));

	prop_get_int(e->mmu->props, "claim-mode", CSTR, &mode);

	PUSH(e, base);
	PUSH(e, virt);
	PUSH(e, size);
	PUSH(e, mode);
	ret = do_mem_method(e, "mmu", "map");
	DPRINTF(("client_claim: space mapped\n"));

	PUSH(e, virt);
	return ret;
}

C(f_client_release)		/* release (size virt --) */
{
	Cell virt;
	Int size;

	IFCKSP(e, 2, 0);
	POP(e, virt);
	POP(e, size);

	/* mmu unmap */
	/* mmu release */
	/* memory release */

	PUSH(e, virt);
	PUSH(e, size);
	return do_mem_method(e, "mmu", "release");
}


/* 6.3.2.5  Control transfer */

C(f_client_boot)		/* boot (bootspec-cstr --) */
{
	Byte *dev;
	Byte *dargs;
	Int dlen, dalen;
	
	IFCKSP(e, 1, 0);
	POPT(e, dev, Byte*);
	dargs = dev;
	
	while (*dargs && !isspace(*dargs))
		dargs++;
	
	dlen = dargs - dev;
	
	while (*dargs && isspace(*dargs))
		dargs++;
	
	dalen = strlen(dargs);
	return boot_load(e, dev, dlen, dargs, dalen);
}

/* enter (--)  ==  interpret */

C(f_client_exit)		/* exit (--) */
{
	cprintf(e, "Program terminated!\n");
	machine_reset_all(e);
	return E_QUIT;		/* should not get here, but silence the compiler */
}

C(f_client_restart)		/* restart (cstr --) */
{
	Byte *cmd;
	IFCKSP(e, 1, 0);
	POPT(e, cmd, Byte*);

	/* save away command so that it will be executed upon restarting */
	/* XXX: do nothing for now. */

	cprintf(e, "Restarting...\n");
	machine_reset_all(e);
	return E_QUIT;		/* should not get here, but silence the compiler */
}

C(f_client_chain)	/* chain (args-len args-addr entry size virt --) */
{
	Retcode ret;
	Byte *args;
	Int alen;
	Byte abuf[STR_SIZE];

	IFCKSP(e, 5, 0);
	ret = f_client_release(e);

	if (ret != NO_ERROR)
		return ret;

	POPT(e, e->load, Byte*);
	POPT(e, args, Byte*);
	POP(e, alen);

	strncpy(abuf, args, alen);
	e->loadargs = abuf;
	ret = machine_init_program(e);

	if (ret != NO_ERROR)
		return ret;

	return machine_go(e);
}


/* 6.3.2.6  User Interface */

C(f_client_interpret)	/* interpret (... cmd-cstr -- ... catch-result) */
{
	Byte *cmd;
	Retcode ret;
	
	IFCKSP(e, 1, 1);
	POPT(e, cmd, Byte*);

	ret = interp_text(e, cmd, CSTR);

	PUSH(e, ret);
	return NO_ERROR;
}

C(f_client_set_callback)	/* set-callback (newfunc -- oldfunc) */
{
	Callback *func;

	IFCKSP(e, 1, 1);
	POPT(e, func, Callback*);
	PUSHP(e, e->callback);
	e->callback = func;
	return NO_ERROR;
}

/* set-symbol-lookup (value-to-sym sym-to-value --) */
C(f_client_set_symbol_lookup)
{
	IFCKSP(e, 2, 0);
	POPT(e, e->sym2value, Callback*);
	POPT(e, e->value2sym, Callback*);
	set_defer(e, "sym>value", "callback sym>value");
	set_defer(e, "value>sym", "callback value>sym");
	return NO_ERROR;
}

/* 6.3.2.7  Time */

/* milliseconds == get-msecs */
EC(f_getmsecs);		/* in other.c */


/* list of builtin extensible client methods */
const Initentry client_services_methods[] =
{
	{ "test", f_client_test, INVALID_FCODE },
	{ "test-method", f_client_test_method, INVALID_FCODE },

	{ "peer", f_peer, INVALID_FCODE },
	{ "child", f_child, INVALID_FCODE },
	{ "parent", f_client_parent, INVALID_FCODE },
	{ "instance-to-package", f_client_instance_to_package, INVALID_FCODE },
	{ "getproplen", f_client_getproplen, INVALID_FCODE },
	{ "getprop", f_client_getprop, INVALID_FCODE },
	{ "nextprop", f_client_nextprop, INVALID_FCODE },
	{ "setprop", f_client_setprop, INVALID_FCODE },
	{ "canon", f_client_canon, INVALID_FCODE },
	{ "finddevice", f_client_finddevice, INVALID_FCODE },
	{ "instance-to-path", f_client_instance_to_path, INVALID_FCODE },
	{ "instance-to-interposed-path", f_client_instance_to_interposed_path,
			INVALID_FCODE },
	{ "package-to-path", f_client_package_to_path, INVALID_FCODE },
	{ "call-static-method", f_client_call_static_method, INVALID_FCODE },
	{ "call-method", f_client_call_method, INVALID_FCODE },

	{ "open", f_client_open, INVALID_FCODE },
	{ "close", f_close_dev, INVALID_FCODE },
	{ "read", f_client_read, INVALID_FCODE },
	{ "write", f_client_write, INVALID_FCODE },
	{ "seek", f_client_seek, INVALID_FCODE },

	{ "claim", f_client_claim, INVALID_FCODE },
	{ "release", f_client_release, INVALID_FCODE },

	{ "boot", f_client_boot, INVALID_FCODE },
	{ "enter", interpret, INVALID_FCODE },
	{ "exit", f_client_exit, INVALID_FCODE },
	{ "chain", f_client_chain, INVALID_FCODE },
	{ "restart", f_client_restart, INVALID_FCODE },

	{ "interpret", f_client_interpret, INVALID_FCODE },
	{ "set-callback", f_client_set_callback, INVALID_FCODE },
	{ "set-symbol-lookup", f_client_set_symbol_lookup, INVALID_FCODE },

	{ "milliseconds", f_getmsecs, INVALID_FCODE },

	{ NULL,             NULL },
};


CC(install_client_services)
{
	Package *pkg;
	Retcode ret;

	pkg = find_package(e->root, "openprom", CSTR);
	DPRINTF(("install_client_services: pkg=%p\n", pkg));

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	e->client = new_pkg_name(pkg, "client-services");

	if (e->client == NULL)
		return E_NULL_PACKAGE;

	ret = init_entries(e, e->client->dict, client_services_methods);

	DPRINTF(("install_client_services: return %s (%d)\n", err2str(ret), ret));
	return ret;
}


/* client-interface call-back hook from outside SmartFirmware */
Int
client_interface(Cell array[])
{
	const Initentry *method;
	char *name;
	Int nargs, nreturns, i;
	Cell *args, *returns;
	Cell *sp = g_e->sp;		/* save stack pointer */
	Cell *rsp = g_e->rsp;	/* save return stack pointer */
	Retcode ret;

	name = (char*)(uPtr)array[0];
	nargs = array[1];
	nreturns = array[2];
	args = (nargs) ? array + 3 : NULL;
	returns = (nreturns) ? array + 3 + nargs : NULL;

	DPRINTF(("client_interface: %s %d %d\n", name, nargs, nreturns));

	if (CKSP(g_e, 0, nargs))	/* make sure we have room for the args */
		return -1;

	/* move args to Forth stack, call Forth word, and pop return values */
	for (i = nargs - 1; i >= 0; i--)
		PUSH(g_e, args[i]);

	if (g_e->client)				/* new services are installed */
		ret = execute_static_method_name(g_e, g_e->client, name, CSTR);
	else
	{
		/* search our init list and call function directly */
		for (method = client_services_methods; method->name; method++)
			if (compare_strs(name, CSTR, method->name, CSTR))
				break;

		if (method->name == NULL)
			return -1;

		ret = method->func(g_e);	/* call function directly */
	}

	/* make sure enough stuff is returned on the stack */
	if (ret != NO_ERROR || CKSP(g_e, nreturns, 0))
	{
		g_e->sp = sp;		/* restore stack */
		g_e->rsp = rsp;		/* restore return stack */
		return -1;
	}

	/* pop of any return values, taking care not to underflow the stack */
	for (i = 0; i < nreturns && g_e->sp > sp; i++)
		POP(g_e, returns[i]);

	g_e->sp = sp;		/* restore stack in case of error */
	g_e->rsp = rsp;		/* restore return stack in case of error */
	DPRINTF(("client_interface: return 0\n"));
	return 0;
}
