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

/* (c) Copyright 1996 by CodeGen, Inc.  All Rights Reserved. */

#ifndef __FAKEPCI_H__
#define __FAKEPCI_H__

struct fake_pci_dev;
typedef struct fake_pci_dev Fake_pci_dev;

struct fake_pci_dev_methods
{
	Bool (*init)(Fake_pci_dev *self);
	Int (*configread)(Fake_pci_dev *self, int reg);
	void (*configwrite)(Fake_pci_dev *self, int reg, Int val);
	int (*barsize)(Fake_pci_dev *self, int bar);
	int (*bartype)(Fake_pci_dev *self, int bar);
	Retcode (*memread)(Fake_pci_dev *self, int bar, Int offset, void *value,
			int size);
	Retcode (*memwrite)(Fake_pci_dev *self, int bar, Int offset, Int value,
			int size);
	Retcode (*ioread)(Fake_pci_dev *self, int bar, Int offset, void *value,
			int size);
	Retcode (*iowrite)(Fake_pci_dev *self, int bar, Int offset, Int value,
			int size);
};

typedef struct fake_pci_dev_methods Fake_pci_dev_methods;

struct fake_pci_dev
{
	int bus;
	int dev;
	int func;
	Fake_pci_dev_methods *methods;

	/* filled in by the init routine */
	int configsize;
	void *private;
};

extern Retcode fakepci_mem_map(Int addr, Int size, Fake_pci_dev *dev, int bar,
		Int offset);
extern Retcode fakepci_mem_unmap(Int addr, Int size, Fake_pci_dev *dev, int bar,
		Int offset);
extern Retcode fakepci_io_map(Int addr, Int size, Fake_pci_dev *dev, int bar,
		Int offset);
extern Retcode fakepci_io_unmap(Int addr, Int size, Fake_pci_dev *dev, int bar,
		Int offset);

#endif /* __FAKEPCI_H__ */
