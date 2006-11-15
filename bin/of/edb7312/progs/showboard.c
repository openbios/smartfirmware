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

/* (c) Copyright 2001 by CodeGen, Inc.  All Rights Reserved. */

/* Simple program to show board information using the client interface.
 */

#include "sfclient.h"

/* dummies to avoid normal gcc/g++ runtime goo */
void __gccmain() { }
void __main() { }

/* debug printf - could use varargs and vsprintf if so desired
 */
void
dprintf(const char *str)
{
#ifdef DEBUG
	sf_puts(str);
#endif
}


extern int sf_strlen(const char *str);

int
sf_strncmp(const char *s1, const char *s2, int len)
{
	if (s1 == NULL)
		s1 = "";

	if (s2 == NULL)
		s2 = "";

	for (; *s1 && *s2 && len--; s1++, s2++)
		if (*s1 != *s2)
			break;

	return (!len && !*s1 && !*s2) ? 0 : *s2 - *s1;
}

Package *
find_device(char *device)
{
	Package *pkg = sf_getroot();

	if (*device != '/')
		return NULL;

	while (*device)
	{
		char *dev2;
		Package *child;
		int ch;
		char name[256];

		device++;
		dev2 = device;

		while (*dev2 != '\0' && *dev2 != '/')
			dev2++;

		ch = *dev2;
		*dev2 = '\0';

		for (child = sf_child(pkg); child; child = sf_peer(child)) 
		{
			int len = sf_getprop(child, "name", name, 256);

			if (len >= 0 && sf_strncmp(device, name, len) == 0)
				break;
		}

		if (child == NULL)
			return NULL;

		*dev2 = ch;
		device = dev2;
		pkg = child;
	}

	return pkg;
}

char *
sf_strchr(char *str, char *chars)
{
	for (; *str; str++)
	{
		char *p = chars;

		for (; *p; p++)
			if (*str == *p)
				return str;
	}

	return NULL;
}

int
get_string_property(char *device, char *property, char *buf, int buflen)
{
	Instance *devinst;
	Package *devpkg;
	int len;
	int len2;

	dprintf("get_str_prop: dev ");
	dprintf(device);
	dprintf(", prop ");
	dprintf(property);
	dprintf("\r\n");

	devinst = sf_open(device);

	if (devinst == NULL)
	{
		dprintf("get_str_prop: device failed to opened\r\n");
		devpkg = find_device(device);

		if (devpkg == NULL)
			return 0;

		len = sf_getproplen(devpkg, property);

		if (len <= 0)
			return 0;

		dprintf("get_str_prop: length looks good\r\n");

		if (len >= buflen)
			len = buflen - 1;
		
		len2 = sf_getprop(devpkg, property, buf, len);

		if (len2 <= 0)
			return 0;

		buf[len] = '\0';
		dprintf("get_str_prop: got string prop \"");
		dprintf(buf);
		dprintf("\"\r\n");
		return 1;
	}

	dprintf("get_str_prop: device opened\r\n");

	devpkg = sf_instance_to_package(devinst);

	if (devpkg == NULL)
	{
		sf_close(devinst);
		return 0;
	}

	dprintf("get_str_prop: got package\r\n");

	len = sf_getproplen(devpkg, property);

	if (len <= 0)
	{
		sf_close(devinst);
		return 0;
	}

	dprintf("get_str_prop: length looks good\r\n");

	if (len >= buflen)
		len = buflen - 1;
	
	len2 = sf_getprop(devpkg, property, buf, len);
	sf_close(devinst);

	if (len2 <= 0)
		return 0;

	buf[len] = '\0';
	dprintf("get_str_prop: got string prop \"");
	dprintf(buf);
	dprintf("\"\r\n");
	return 1;
}

int
get_int_property(char *device, char *property, int *value)
{
	Instance *devinst;
	Package *devpkg;
	int len;
	int len2;
	char buf[4];

	devinst = sf_open(device);

	if (devinst == NULL)
		return 0;

	devpkg = sf_instance_to_package(devinst);

	if (devpkg == NULL)
	{
		sf_close(devinst);
		return 0;
	}

	len = sf_getproplen(devpkg, property);

	if (len != 4)
	{
		sf_close(devinst);
		return 0;
	}

	len2 = sf_getprop(devpkg, property, buf, 4);

	if (len2 != 4)
		return 0;

	*value = *(uInt *)buf;
	return 1;
}

int
main(int argc, const char *argv[], const char *envv[])
{
	char buf[2048];

	/* addr of C client-interface is passed in argc after terminating NULL
	   - it is also passed as a hex string in envv as "client_interface"
	   - the addr of the asm interface is in a machine-dependent register
	 */
	sf_puts("Board:\t\t");

	if (get_string_property("/", "CODEGEN,board", buf, sizeof buf) || 
			get_string_property("/", "name", buf, sizeof buf))
	{
		sf_puts(buf);
		sf_puts("\r\n");
	}
	else
		sf_puts("unknown\r\n");

	if (get_string_property("/", "CODEGEN,vendor", buf, sizeof buf))
	{
		sf_puts("Vendor:\t\t");
		sf_puts(buf);
		sf_puts("\r\n");
	}

	if (get_string_property("/", "CODEGEN,description", buf, sizeof buf))
	{
		sf_puts("Description:\t");
		sf_puts(buf);
		sf_puts("\r\n");
	}

	if (get_string_property("/openprom", "SmartFirmware-version", buf, sizeof buf))
	{
		char *comma;

		sf_puts("SmartFirmware:\tv");
		sf_puts(buf);
		sf_puts("\r\n");

		if (get_string_property("/openprom", "model", buf, sizeof buf))
		{

			comma = sf_strchr(buf, ",");

			if (comma)
			{
				comma++;
				sf_puts("Board Firmware:\t");
				sf_puts(comma);
				sf_puts("\r\n");
			}
		}

		if (get_string_property("/openprom", "CodeGen-copyright", buf, sizeof buf))
		{
			sf_puts("\r\n");
			sf_puts(buf);
			sf_puts("\r\n");
		}
	}

	return R_OK;
}
