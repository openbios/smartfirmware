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

/* Library of C routines for using the SmartFirmware C client-interface.
   Similar code would be used for a machine-dependent asm client-interface,
   and only sf_call_firmware() below would need to be altered.
 */

#include "sfclient.h"

#include <stdarg.h>

/* max size of args/returns array - OpenFirmware requires at least 6 */
#ifndef MAX_ARR
#	define MAX_ARR	64		/* overkill */
#endif

static Retcode (*g_client_interface)(Cell array[]) = NULL;
static Instance *g_input = NULL;
static Instance *g_output = NULL;


/* lowest-level C routine to call the client-interface
  - asm interface is machine-dependent but generally follows this outline
 */
Retcode
sf_call_firmware(const char *name, Int nargs, Int nreturns,
		Cell args[], Cell returns[])
{
	Int i;
	Retcode ret;
	Cell array[MAX_ARR];

	if (nargs + 3 >= MAX_ARR)
		return R_ERR;

	array[0] = (uPtr)name;
	array[1] = nargs;
	array[2] = nreturns;

	/* copy args for the client-interface */
	for (i = 0; i < nargs; i++)
		array[3 + i] = args[i];

	ret = g_client_interface(array);

	/* copy returns for the C caller */
	if (ret == R_OK)
		for (i = 0; i < nreturns; i++)
			returns[i] = array[3 + nargs + i];

	return ret;
}


/* C wrappers of standard IEEE-1275 client services: */

/* 6.3.2.1  Client interface */

/* return 0 if service "name" exists, and -1 if it does not
 */
Retcode
sf_test(const char *name)
{
	Cell args[1], returns[1];

	args[0] = (uPtr)name;
	
	if (sf_call_firmware("test", 1, 1, args, returns) != R_OK)
		return R_ERR;
	
	return returns[0];
}


/* return 0 if method "name" exists in the Package, and -1 if it does not
   - this is an additional method from proposal/extension #270
 */
Retcode
sf_test_method(Package *pkg, const char *name)
{
	Cell args[2], returns[1];

	args[0] = (uPtr)pkg;
	args[0] = (uPtr)name;

	if (sf_call_firmware("test-method", 1, 1, args, returns) != R_OK)
		return R_ERR;

	return returns[0];
}


/* 6.3.2.2  Device tree */

static Package *
sf_do_node(Package *p, const char *cmd)
{
	Cell args[1], returns[1];

	args[0] = (uPtr)p;
	
	if (sf_call_firmware(cmd, 1, 1, args, returns) != R_OK)
		return NULL;
	
	return (Package*)(uPtr)returns[0];
}

/* Return the next sibling in the tree.
 */
Package *
sf_peer(Package *p)
{
	return sf_do_node(p, "peer");
}

/* Return the first child of the package.
 */
Package *
sf_child(Package *p)
{
	return sf_do_node(p, "child");
}

/* Return the parent of the package.
 */
Package *
sf_parent(Package *p)
{
	return sf_do_node(p, "parent");
}

/* Return the package of the instance
 */
Package *
sf_instance_to_package(Instance *inst)
{
	Cell args[1], returns[1];

	args[0] = (uPtr)inst;
	
	if (sf_call_firmware("instance-to-package", 1, 1, args, returns) != R_OK)
		return NULL;
	
	return (Package*)(uPtr)returns[0];
}


/* Get the length of a property, 0 if the property has no value,
   or -1 on any error
 */
int
sf_getproplen(Package *pkg, const char *prop)
{
	Cell args[2], returns[1];
	Retcode ret;

	args[0] = (uPtr)pkg;
	args[1] = (uPtr)prop;

	ret = sf_call_firmware("getproplen", 2, 1, args, returns);

	if (ret < R_OK)
		return ret;
	
	return returns[0];
}

/* Get a property's value, returning any error code, or the actual
   length of the property
 */
int
sf_getprop(Package *pkg, const char *propname, char *propbuf, int propbuflen)
{
	Cell args[4], returns[1];
	Retcode ret;

	args[0] = (uPtr)pkg;
	args[1] = (uPtr)propname;
	args[2] = (uPtr)propbuf;
	args[3] = propbuflen;

	ret = sf_call_firmware("getprop", 4, 1, args, returns);

	if (ret < R_OK)
		return ret;
	
	return returns[0];
}

/* Return the property name of the property following "prevprop"
   of the Package.  propbuf must be at least 32 chars big.
   Returns -1 on error, 0 if no more properties exist after
   "prevprop", or 1 otherwise.
 */
int
sf_nextprop(Package *pkg, const char *prevprop, char *propbuf)
{
	Cell args[3], returns[1];
	Retcode ret;

	args[0] = (uPtr)pkg;
	args[1] = (uPtr)prevprop;
	args[2] = (uPtr)propbuf;

	ret = sf_call_firmware("nextprop", 3, 1, args, returns);

	if (ret < R_OK)
		return ret;
	
	return returns[0];
}

/* set the property's value, creating the property if it doesn't exist
   - return -1 on error or the actual size of the new property
 */
int
sf_setprop(Package *pkg, const char *prop, char *value, int len)
{
	Cell args[4], returns[1];
	Retcode ret;

	args[0] = (uPtr)pkg;
	args[1] = (uPtr)prop;
	args[2] = (uPtr)value;
	args[3] = len;

	ret = sf_call_firmware("setprop", 4, 1, args, returns);

	if (ret != R_OK)
		return ret;
	
	return returns[0];
}


/* convert a possibly ambiguous device-specifier to a fully-qualified path
   - return -1 on error or the length of the fully-qualified name
 */
int
sf_canon(const char *devspec, char *buf, int buflen)
{
	Cell args[3], returns[1];
	Retcode ret;

	args[0] = (uPtr)devspec;
	args[1] = (uPtr)buf;
	args[2] = buflen;

	ret = sf_call_firmware("canon", 3, 1, args, returns);

	if (ret < R_OK)
		return ret;
	
	return returns[0];
}

/* see if a device-path exists - returns the Package of the device
   if it exists and NULL on any error
 */
Package *
sf_finddevice(const char *devpath)
{
	Cell args[1], returns[1];

	args[0] = (uPtr)devpath;
	
	if (sf_call_firmware("finddevice", 1, 1, args, returns) != R_OK)
		return NULL;
	
	return (Package*)(uPtr)returns[0];
}

/* Return the fully qualified device path of the instance, -1 on error
 */
int
sf_instance_to_path(Instance *inst, char *buf, int buflen)
{
	Cell args[3], returns[1];
	Retcode ret;

	args[0] = (uPtr)inst;
	args[1] = (uPtr)buf;
	args[2] = buflen;

	ret = sf_call_firmware("instance-to-path", 3, 1, args, returns);

	if (ret < R_OK)
		return ret;
	
	return returns[0];
}

/* Return the actual interposed device path of the instance, -1 on error
 */
int
sf_instance_to_interposed_path(Instance *inst, char *buf, int buflen)
{
	Cell args[3], returns[1];
	Retcode ret;

	args[0] = (uPtr)inst;
	args[1] = (uPtr)buf;
	args[2] = buflen;

	ret = sf_call_firmware("instance-to-interposed-path", 3, 1, args, returns);

	if (ret < R_OK)
		return ret;
	
	return returns[0];
}

/* Return the fully qualified device path of the package, -1 on error
 */
int
sf_package_to_path(Package *pkg, char *buf, int buflen)
{
	Cell args[3], returns[1];
	Retcode ret;

	args[0] = (uPtr)pkg;
	args[1] = (uPtr)buf;
	args[2] = buflen;

	ret = sf_call_firmware("package-to-path", 3, 1, args, returns);

	if (ret < R_OK)
		return ret;
	
	return returns[0];
}

/* similar to sf_call_firmware, but uses "call-method" service
 */
Retcode
sf_call_method(const char *method, Instance *inst, Int nargs, Int nreturns,
		Cell args[], Cell returns[])
{
	Int i;
	Cell array[MAX_ARR];

	if (nargs + 2 >= MAX_ARR)
		return R_ERR;

	array[0] = (uPtr)method;
	array[1] = (uPtr)inst;

	for (i = 0; i < nargs; i++)
		array[2 + i] = args[i];

	return sf_call_firmware("call-method", nargs + 2, nreturns, array, returns);
}


/* 3.2.2.3  Device I/O */

/* open a device and return its instance, or NULL on error
 */
Instance *
sf_open(const char *devpath)
{
	Cell args[1], returns[1];

	args[0] = (uPtr)devpath;
	
	if (sf_call_firmware("open", 1, 1, args, returns) != R_OK)
		return NULL;
	
	return (Instance*)(uPtr)returns[0];
}

/* close a previously opened instance
 */
void
sf_close(Instance *inst)
{
	Cell args[1];

	args[0] = (uPtr)inst;
	(void)sf_call_firmware("close", 1, 0, args, NULL);
}

/* read data from an opened device - returns the actual length read,
   -1 on error, or -2 for no data pending for the read - (non-blocking)
 */
int
sf_read(Instance *inst, char *buf, int len)
{
	Cell args[3], returns[1];
	Retcode ret;

	args[0] = (uPtr)inst;
	args[1] = (uPtr)buf;
	args[2] = len;
	
	ret = sf_call_firmware("read", 3, 1, args, returns);

	if (ret < R_OK)
		return ret;
	
	return returns[0];
}

/* write data to an opened device - return the actual length written
   or -1 on error
 */
int
sf_write(Instance *inst, const char *buf, int len)
{
	Cell args[3], returns[1];
	Retcode ret;

	args[0] = (uPtr)inst;
	args[1] = (uPtr)buf;
	args[2] = len;

	ret = sf_call_firmware("write", 3, 1, args, returns);

	if (ret < R_OK)
		return ret;
	
	return returns[0];
}

/* seek device to the location specified by poshi.poslo - returns
   -1 on error or if the device cannot seek
 */
Retcode
sf_seek(Instance *inst, int poshi, int poslo)
{
	Cell args[3], returns[1];
	Retcode ret;

	args[0] = (uPtr)inst;
	args[1] = poshi;
	args[2] = poslo;
	
	ret = sf_call_firmware("seek", 3, 1, args, returns);

	if (ret != R_OK)
		return ret;
	
	return returns[0];
}


/* 6.3.2.4  Memory */
/* - note - these words may not function in all implementations */

/* allocate requested size of memory, beginning at "virtaddr" if "align"
   is 0, or at any addr if "align" is any other value ("virt" is ignored)
   - returns the baseaddr of the memory, or -1 on any error
 */
void *
sf_claim(void *virt, int size, int align)
{
	Cell args[3], returns[1];
	Retcode ret;

	args[0] = (uPtr)virt;
	args[1] = size;
	args[2] = align;

	ret = sf_call_firmware("claim", 3, 1, args, returns);

	if (ret < R_OK)
		return (void*)ret;
	
	return (void*)(uPtr)returns[0];
}

/* release memory allocated by "claim"
 */
void
sf_release(void *virt, int size)
{
	Cell args[2];

	args[0] = (uPtr)virt;
	args[1] = size;
	(void)sf_call_firmware("release", 2, 0, args, NULL);
}


/* 6.3.2.5  Control transfer */
/* - note - these words may not function in all implementations */

/* exit the client program, reset the system, and reboot with the
   given "bootspec" argument passed to the "boot" command
 */
void
sf_boot(const char *bootspec)
{
	Cell args[1];

	args[0] = (uPtr)bootspec;
	(void)sf_call_firmware("boot", 1, 0, args, NULL);
}

/* enter the command-interpreter ("ok" prompt)
 */
void
sf_enter(void)
{
	(void)sf_call_firmware("enter", 0, 0, NULL, NULL);
}

/* exit the command-interpreter
 */
void
sf_exit(void)
{
	(void)sf_call_firmware("exit", 0, 0, NULL, NULL);
}

/* free size bytes at virt, then load another client program at entry
   - pass argstr/arglen to the new program and launch it
 */
void
sf_chain(void *virt, int size, void *entry, char *argstr, int arglen)
{
	Cell args[5];

	args[0] = (uPtr)virt;
	args[1] = size;
	args[2] = (uPtr)entry;
	args[3] = (uPtr)argstr;
	args[4] = arglen;

	(void)sf_call_firmware("chain", 5, 0, args, NULL);
}


/* 6.3.2.6  User Interface */

/* similar to sf_call_firmware, but uses "interpret" service to
   execute arbitrary Forth commands
 */
Retcode
sf_interpret(const char *cmd, Int nargs, Int nreturns,
		Cell args[], Cell returns[])
{
	Int i;
	Cell array[MAX_ARR];

	if (nargs + 1 >= MAX_ARR)
		return R_ERR;

	array[0] = (uPtr)cmd;

	for (i = 0; i < nargs; i++)
		array[1 + i] = args[i];

	return sf_call_firmware("interpret", nargs + 2, nreturns, array, returns);
}

/* define routine for handling "callback" and "sync" Forth words
   - old entry point is returned
 */
Callback *
sf_set_callback(Callback *newfunc)
{
	Cell args[1], returns[1];
	Retcode ret;

	args[0] = (uPtr)newfunc;

	ret = sf_call_firmware("set-callback", 1, 1, args, returns);

	if (ret < R_OK)
		return (void*)ret;
	
	return (Callback*)(uPtr)returns[0];
}

/* define "defer" words "sym>value" and "value>sym" to look up symbols 
 */
void
sf_set_symbol_lookup(Callback *sym_to_value, Callback *value_to_sym)
{
	Cell args[2];

	args[0] = (uPtr)sym_to_value;
	args[1] = (uPtr)value_to_sym;

	(void)sf_call_firmware("set-symbol-lookup", 2, 0, args, NULL);
}


/* 6.3.2.7  Time */

/* return number of milliseconds, typically since bootup
 */
unsigned long
sf_milliseconds(void)
{
	Cell returns[1];
	Retcode ret;

	ret = sf_call_firmware("milliseconds", 0, 1, NULL, returns);

	if (ret != R_OK)
		return 0;
	
	return returns[0];
}


/* non-standard routines built on top of the above for convenience */


/* Return the root node of the device tree
 */
Package *
sf_getroot(void)
{
	return sf_peer(NULL);
}


/* Read a character from the console.  Return OpenFirmware magic
   values of -1 for any error and -2 if input is not ready.
   Otherwise return the character read from the console.
 */
int
sf_getchar(void)
{
	char buf[2];
	Cell args[3], returns[1];

	args[0] = (uPtr)g_input;
	args[1] = (uPtr)buf;
	args[2] = 1;
	
	if (sf_call_firmware("read", 3, 1, args, returns) != R_OK)
		return R_ERR;

	return returns[0] ? buf[0] : R_NO_DATA;
}

/* read a line from the console - this could take a while
 */
int
sf_gets(char *str, int len)
{
	int c;
	int i = 0;

	for (c = sf_getchar(); c != '\n' && c != '\r' && i < len;
			c = sf_getchar())
	{
		if (c == R_NO_DATA)	/* OpenFirmware magic for no input ready */
			continue;

		if (c < R_OK)		/* OpenFirmware magic for error */
			return R_ERR;

		str[i++] = c;
	}
	
	return i;
}

int
sf_strlen(const char *str)
{
	int len = 0;

	if (str)
		while (*str++)
			len++;

	return len;
}

/* write a string to the console, ignoring errors.
 */
void
sf_puts(const char *str)
{
	Cell args[3], returns[1];

	args[0] = (uPtr)g_output;
	args[1] = (uPtr)str;
	args[2] = sf_strlen(str);
	(void)sf_call_firmware("write", 3, 1, args, returns);
}

/* write a character to the console, ignoring errors.
 */
void
sf_putchar(int c)
{
	char str[2];
	Cell args[3], returns[1];

	args[0] = (uPtr)g_output;
	args[1] = (uPtr)str;
	args[2] = 1;
	str[0] = c;
	(void)sf_call_firmware("write", 3, 1, args, returns);
}

/* vbprintf is like vsprintf but only handles strings and ints,
   with just a few formats and field options plus some custom extensions:
   #	alternate formatting - for hex, prepend 0x to output otherwise ignore
   -	right-pad output with blanks - only for strings right now
   0	left-pad numbers with zero

   0-9+	numeric field width - left-pad with blanks unless leading 0 specified
   *	numeric field width taken from next argument

   lqL	display following integer specification as a Long instead of an Int
   C	display following integer specification as a Cell - may be Int or Long

   d	display integer argument in decimal
   u	display unsigned integer argument in decimal
   x	display unsigned integer argument in hexadecimal - case HEX_A
   X	display unsigned integer argument in upper-case hexadecimal
   p	display pointer argument as hex with leading 0x - like %#x or %#lx
   c	display integer as a single character
   s	display C string (null-byte terminated)
   P	display Pascal string (1st byte is length)
   S	display Forth string (2 args - string & length)
 */
extern int vbprintf(char *buf, const char *fmt, va_list args);

#define STR_SIZE 256
#define HEX_A	'A'
#define isdigit(x) ((x) >= '0' && (x) <= '9')

int
vbprintf(char *buf, const char *fmt, va_list args)
{
	char *sarg, *s;
	uLong iarg;
	int field, hex;
	Int len;
	Bool rpad, lzero, sign, larg, alt;
	char nbuf[STR_SIZE];
	
	if (fmt == NULL || buf == NULL)
		return 0;
		
	while (*fmt)
	{
		if (*fmt != '%')
		{
			if (*fmt == '\n')
				*buf++ = '\r';

			*buf++ = *fmt++;
			continue;
		}

		if (*++fmt == '%')
		{
			*buf++ = *fmt++;
			continue;
		}
		
		field = 0;
		rpad = FALSE;
		lzero = FALSE;
		sign = FALSE;
		hex = 0;
		larg = FALSE;
		alt = FALSE;
	
		if (*fmt == '#')
		{
			fmt++;
			alt = TRUE;
		}
	
		if (*fmt == '-')
		{
			fmt++;
			rpad = TRUE;
		}

		if (*fmt == '0')
		{
			fmt++;
			lzero = TRUE;
		}

		if (*fmt == '*')
		{
			field = va_arg(args, Int);
			fmt++;
		}
		else
			for (; isdigit(*fmt); fmt++)
				field = field * 10 + *fmt - '0';
		
		/* following arg is long instead of int */
		if (*fmt == 'l' || *fmt == 'L' || *fmt == 'q' || *fmt == 'C')
		{
			if (*fmt != 'C' || sizeof (Cell) > sizeof (Int))
				larg = TRUE;

			fmt++;

			/* skip "%ll*" style formats */
			if (*fmt == 'l')
				fmt++;
		}

		/* used to be switch (*fmt++) but some gcc revs generate bad code */
		iarg = *fmt++;

		if (iarg == 'c')			/* character */
		{
			iarg = va_arg(args, Int);
			*buf++ = iarg;
		}
		else if (iarg == 'x' || iarg == 'X' || iarg == 'd' || iarg == 'u' ||
				iarg == 'p')
		{
			if (iarg == 'x')
				hex = HEX_A;
			else if (iarg == 'X')
				hex = 'A';
			else if (iarg == 'p')
			{
				hex = HEX_A;
				alt = TRUE;
			}

			if (larg || (iarg == 'p' && sizeof (Ptr) > sizeof (Int)))
				iarg = va_arg(args, uLong);
			else if (hex || iarg == 'u')
			{
				iarg = va_arg(args, uInt);

				/* work around for gcc ARM problem */
				iarg &= 0xFFFFFFFF;
			}
			else
				iarg = va_arg(args, Int);

			s = nbuf + STR_SIZE - 1;
			*s = '\0';
			
			if (!hex && (Long)iarg < 0)
			{
				sign = TRUE;
				iarg = -(Long)iarg;
				field--;
			}
			else if (iarg == 0)
			{
				*--s = '0';
				field--;
			}
			
			for (; iarg; field--)
			{
				if (hex)
				{
					int d = iarg & 0xF;
					*--s = (d < 10) ? d + '0' : d - 10 + hex;
					iarg >>= 4;
				}
				else
				{
					*--s = iarg % 10 + '0';
					iarg /= 10;
				}
			}
			
			while (field-- > 0)
				*--s = (lzero) ? '0' : ' ';
			
			if (hex && alt)
			{
				*--s = 'x';
				*--s = '0';
			}

			if (sign)
				*--s = '-';
			
			while (*s)
				*buf++ = *s++;
		}
		else if (iarg == 's' || iarg == 'S' || iarg == 'P')		/* string */
		{
			sarg = va_arg(args, char*);

			if (iarg == 'S')			/* Forth-style string */
				len = va_arg(args, Int);
			else if (sarg == NULL)		/* no string */
				len = 0;
			else if (iarg == 'P')		/* Pascal-style string */
				len = *(uByte*)sarg++;
			else						/* C string */
				len = sf_strlen(sarg);
			
			if (!rpad)
			{
				while (field - len > 0)
				{
					*buf++ = ' ';
					field--;
				}
			}

			for (; len-- > 0; field--)
				*buf++ = *sarg++;
			
			if (rpad)
				while (field-- > 0)
					*buf++ = ' ';
		}
	}
	
	*buf = '\0';
	return sf_strlen(buf);
}
 
/* this is like sprintf but renamed to avoid collision with standard libs */
int
bprintf(char *buf, const char *fmt, ...)
{
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = vbprintf(buf, fmt, args);
	va_end(args);
	return ret;
}

/* format a strings to the console 
 */
int
sf_printf(const char *fmt, ...)
{
	char buf[2048];
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = vbprintf(buf, fmt, args);
	va_end(args);
	sf_puts(buf);
	return ret;
}

/* initialization stuff */

/* Initialize our global pointer and console I/O instances.
 */
Retcode
sf_init( Int (*client_interface)(Cell array[]) )
{
	Int ret;
	Retcode catch;
	Cell returns[2];

	if (client_interface == NULL)
		return R_ERR;

	g_client_interface = client_interface;

	/* we could pick these up from the /chosen properties, but this
	   is a lot simpler - we could also open the "screen" and
	   "keyboard" devices but that would be even more work
	 */
	ret = sf_interpret("stdin @", 0, 2, NULL, returns);
	catch = returns[0];
	g_input = (Instance*)(uPtr)returns[1];

	if (ret == R_OK && catch == R_OK)
	{
		ret = sf_interpret("stdout @", 0, 2, NULL, returns);
		catch = returns[0];
		g_output = (Instance*)(uPtr)returns[1];
	}

	return (catch == R_OK) ? ret : catch;
}

/* Use this as the entry-point to setup the client-interface handler
   and still use a traditional C main().
 */
int
sf_start(int argc, const char *argv[], const char *envv[])
{
	void *client_interface;
	extern int main(int argc, const char *argv[], const char *envv[]);

	client_interface = (void *)argv[argc + 1];
	sf_init(client_interface);
	return main(argc, argv, envv);
}
