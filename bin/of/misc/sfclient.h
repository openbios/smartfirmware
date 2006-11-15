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

/* (c) Copyright 1999-2000 by CodeGen, Inc.  All Rights Reserved. */

/* Library of C routines for using the SmartFirmware client-interface.
 */

#ifndef __SFCLIENT_H__
#define __SFCLIENT_H__

#ifndef NULL
#	define NULL 0
#endif

#ifndef FALSE
#	define FALSE 0
#endif
#ifndef TRUE
#	define TRUE 1
#endif

typedef struct instance Instance;
typedef struct package Package;
typedef int Retcode;

typedef char Bool;
typedef signed short Short;
typedef signed int Int;
typedef signed long Long;
typedef unsigned char uByte;
typedef unsigned short uShort;
typedef unsigned int uInt;
typedef unsigned long uLong;

#ifdef INT64
	typedef INT64 Cell;		/* a Forth cell - at least 64-bits, also must
								hold any pointer */
	typedef unsigned INT64 uPtr;	/* must be able to hold any pointer */
	typedef INT64 Ptr;	/* must be able to hold any pointer */
#else
	typedef int Cell;	/* a Forth cell - must be able to hold any pointer */
	typedef unsigned int uPtr;	/* must be able to hold any pointer */
	typedef int Ptr;	/* must be able to hold any pointer */
#endif

typedef int (*Callback)(Cell *array);

#define R_OK		0
#define R_ERR		-1
#define R_NO_DATA	-2


/* initialization stuff - must be done before client-interface will work */

/* 
	addr of C client-interface is passed in argc after terminating NULL
	   - it is also passed as a hex string in envv as "client_interface"
	   - the addr of the asm interface is in a machine-dependent register

	sf_init MUST be called from main before doing anything else to
	initialize its pointer to the C client-interface.

	int
	main(int argc, const char *argv[], const char *envv[])
	{
		sf_init( (void*)argv[argc + 1] );
		...
	}

 */
Retcode sf_init( Retcode (*client_interface)(Cell array[]) );


/* call client-interface directly */
Retcode sf_call_firmware(const char *name, Int nargs, Int nreturns,
		Cell args[], Cell returns[]);


/* Basic client-interface routines as specified in OpenFirmware standard.
   These are simply C wrappers around standard client-services.
 */

/* 6.3.2.1  Client interface */

Retcode sf_test(const char *name);
	/* return 0 if service "name" exists, and -1 if it does not */
Retcode sf_test_method(Package *pkg, const char *name);
	/* return 0 if method "name" exists in "pkg", and -1 if it does not */


/* 6.3.2.2  Device tree */

Package *sf_peer(Package *p);
	/* Return the next sibling of the package "p", or the root if NULL */
Package *sf_child(Package *p);
	/* Return the first child of the Package. */
Package *sf_parent(Package *p);
	/* Return the parent of the package. */
Package *sf_instance_to_package(Instance *inst);
	/* Return the package of the instance */

int sf_getproplen(Package *pkg, const char *prop);
	/* Get the length of a property, 0 if no value, or -1 on any error */
int sf_getprop(Package *pkg, const char *propname,
	char *propbuf, int propbuflen);
	/* Get a property's value, returning -1 on error and the actual */
	/* size of the property loaded into propbuf */
int sf_nextprop(Package *pkg, const char *prevprop, char *propbuf);
	/* Return the property name of the property following "prevprop" */
	/* of the Package.  propbuf must be at least 32 chars big. */
	/* Returns -1 on error, 0 if no more properties exist after */
	/* "prevprop", or 1 otherwise. */
int sf_setprop(Package *pkg, const char *prop, char *value, int len);
	/* set the property's value, creating the property if it doesn't exist */
	/* - return -1 on error or the actual size of the new property */

int sf_canon(const char *devspec, char *buf, int buflen);
	/* convert a possibly ambiguous device-specifier to a fully-qualified */
	/* path - return -1 on error or the length of the fully-qualified name */

Package *sf_finddevice(const char *devpath);
	/* see if a device-path exists - returns the Package of the device */
	/* if it exists and NULL on any error */

int sf_instance_to_path(Instance *inst, char *buf, int buflen);
	/* Return the fully qualified device path of the instance, -1 on error */
int sf_instance_to_interposed_path(Instance *inst, char *buf, int buflen);
	/* Return the actual interposed device path of the instance, -1 on error */

int sf_package_to_path(Package *pkg, char *buf, int buflen);
	/* Return the fully qualified device path of the package, -1 on error */


/* similar to sf_call_firmware, but uses "call-method" service */
Retcode sf_call_method(const char *method, Instance *inst,
		Int nargs, Int nreturns, Cell args[], Cell returns[]);


/* 6.3.2.3  Device I/O */

Instance *sf_open(const char *devpath);
	/* open a device and return its instance, or NULL on error */
void sf_close(Instance *inst);
	/* close a previously opened instance */
int sf_read(Instance *inst, char *buf, int len);
	/* read data from an opened device - returns the actual length read, */
	/* -1 on error, or -2 for no data pending for the read - (non-blocking) */
int sf_write(Instance *inst, const char *buf, int len);
	/* write data to an opened device - return the actual length written */
	/* or -1 on error */
Retcode sf_seek(Instance *inst, int poshi, int poslo);
	/* seek device to the location specified by poshi.poslo - returns */
	/* -1 on error or if the device cannot seek */


/* 6.3.2.4  Memory */
/* - note - these words may not function in all implementations */

void *sf_claim(void *virt, int size, int align);
	/* allocate requested size of memory, beginning at "virtaddr" if "align" */
	/* is 0, or at any addr if "align" is any other value ("virt" is ignored) */
	/* - returns the baseaddr of the memory, or -1 on any error */
void sf_release(void *virt, int size);
	/* release memory allocated by "claim" */


/* 6.3.2.5  Control transfer */
/* - note - these words may not function in all implementations */

void sf_boot(const char *bootspec);
	/* exit the client program, reset the system, and reboot with the */
	/* given "bootspec" argument passed to the "boot" command */
void sf_enter(void);
	/* enter the command-interpreter ("ok" prompt) */
void sf_exit(void);
	/* exit the command-interpreter */
void sf_chain(void *virt, int size, void *entry, char *argstr, int arglen);
	/* free size bytes at virt, then load another client program at entry */
	/* - pass argstr/arglen to the new program and launch it */


/* 6.3.2.6  User Interface */

Retcode sf_interpret(const char *cmd, Int nargs, Int nreturns,
		Cell args[], Cell returns[]);
	/* similar to sf_call_firmware, but uses "interpret" service to */
	/* execute arbitrary Forth commands */
Callback sf_set_callback(Callback newfunc);
	/* define routine for handling "callback" and "sync" Forth words */
	/* - old entry point is returned */
void sf_set_symbol_lookup(Callback sym_to_value, Callback value_to_sym);
	/* define "defer" words "sym>value" and "value>sym" to look up symbols  */


/* 6.3.2.7  Time */

unsigned long sf_milliseconds(void);
	/* return number of milliseconds, typically since bootup */



/* convenient routines built upon the above standard calls */

Package *sf_getroot(void);
	/* Return the root node of the device tree */

/* simple console I/O */
int sf_getchar(void);				/* read a character from the console. */
int sf_gets(char *str, int len);	/* read a line from the console. */
void sf_putchar(int);				/* write a character to the console. */
void sf_puts(const char *str);		/* write a string to the console. */


#endif /* __SFCLIENT_H__ */
