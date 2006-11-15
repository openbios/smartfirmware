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

/* (c) Copyright 1998 by CodeGen, Inc.  All Rights Reserved. */

/* USB root node for UHCI-compatible controller chips.
 */

#define DEBUG
#include "defs.h"
#include "pci.h"
#include "usb.h"


struct pci_self
{
	int val;	/* XXX */
};
typedef struct pci_self Self;


C(f_usb_uhci_open)			/* open (-- okay?) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, FTRUE);
	return NO_ERROR;
}

C(f_usb_uhci_close)			/* close (--) */
{
	return NO_ERROR;
}


static const Initentry usb_uhci_methods[] =
{
	{ "open",			f_usb_uhci_open,		INVALID_FCODE },
	{ "close",			f_usb_uhci_close,		INVALID_FCODE },
	{ "decode-unit",	f_usb_port_decode_unit,	INVALID_FCODE },
	{ "encode-unit",	f_usb_port_encode_unit,	INVALID_FCODE },
	{ NULL,				NULL },
};


PCI(install_usb_uhci_driver)
{
	return install_usb_root_driver(e, pkg, devinfo, usb_uhci_methods,
			sizeof (Self));
}

Pci_driver usb_uhci_driver =
{
	{ 0, 0, 0, 0, 0, 0xC0300, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0xFFFFFF, 0, 0, 0, 0, 0 },
	install_usb_uhci_driver
};
