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

/* Demo on-board Fcode display driver.
   Compile with "cc-fcode" then tokenize with "of -t".
 */

#include "fcode.h"
#include "fcpci.h"

/* Solaris insists on 1024x768 or 1152x900 for its console, or it crashes. */
#define SCREEN_WIDTH	1024
#define SCREEN_HEIGHT	768


#ifdef STANDALONE
#pragma expand_inline
static void
setup_device_main(void)
{
	DEV("/pci");
	set_my_self(open_dev("\$/pci"));
	new_device();
}
#endif /* STANDALONE */


struct color
{
	unsigned int r, g, b;
};
typedef struct color Color;


#define ISO_COLORS	16
struct rgb
{
	unsigned char r, g, b;
};

static struct rgb g_iso_colors[ISO_COLORS] =
{
	{ 0, 0, 0 },			/* black */
	{ 0, 0, 0xAA },			/* blue */
	{ 0, 0xAA, 0 },			/* green */
	{ 0, 0xAA, 0xAA },		/* cyan */
	{ 0xAA, 0, 0 },			/* red */
	{ 0xAA, 0, 0xAA },		/* magenta */
	{ 0xAA, 0x55, 0 },		/* brown */
	{ 0xAA, 0xAA, 0xAA },	/* white */
	{ 0x55, 0x55, 0x55 },	/* gray */
	{ 0x55, 0x55, 0xFF },	/* bright blue */
	{ 0x55, 0xFF, 0x55 },	/* bright green */
	{ 0x55, 0xFF, 0xFF },	/* bright cyan */
	{ 0xFF, 0x55, 0x55 },	/* bright red */
	{ 0xFF, 0x55, 0xFF },	/* bright magenta */
	{ 0xFF, 0xFF, 0x55 },	/* yellow */
	{ 0xFF, 0xFF, 0xFF },	/* bright white */
};

static char *g_framebuf = NULL;
static int g_framebufsize = 0;


/* the following words are to support color tables
 */

struct dimensions
{
	int width;
	int height;
};

struct dimensions
dimensions(void)
{
	static struct dimensions d = { SCREEN_WIDTH, SCREEN_HEIGHT };

	return d;
}

#pragma alias "color!"
void
color_set(unsigned int /*r*/, unsigned int /*g*/, unsigned int /*b*/,
		int /*index*/)
		/* color! (r g b number --) */
{
	/* TODO - set the DAC color-table index to r/g/b */
}

#pragma alias "color@"
Color
color_get(int /*index*/)		/* color@ (number -- r g b) */
{
	Color c;

	/* TODO - read the DAC color-table index into c.r/g/b */
    c.r = 0;
    c.g = 0;
    c.b = 0;
	return c;
}

void
set_colors(char *addr, int start, int count)
		/* set-colors (addr number #numbers --) */
{
	int i;

	if (start < 0 || start + count > 255)
		abort();
	
	for (i = start; i < start + count; i++)
	{
		color_set(addr[0], addr[1], addr[2], i);
		addr += 3;
	}
}

void
get_colors(char *addr, int start, int count)
		/* get-colors (addr number #numbers --) */
{
	int i;
	Color c;

	if (start < 0 || start + count > 255)
		abort();
	
	for (i = start; i < start + count; i++)
	{
		c = color_get(i);
		addr[0] = c.r;
		addr[1] = c.g;
		addr[2] = c.b;
		addr += 3;
	}
}

static void
screen_open(void)			/* screen-open (--) */
{
	int physhi, physmid, physlo;
	int sizehi, sizelo;
	int cfg;
	int c, l;
	Font_info fi;
	Prop_err pe;
	Prop_int pi;

	pe = get_my_property("\$assigned-addresses");

	if (pe.err)
	{
		type("\$cannot find assigned-addresses property\n");
		abort();
	}

	pi.prop = pe.prop;

	do
	{
		pi = decode_int(pi.prop);
		physhi = pi.val;
		pi = decode_int(pi.prop);
		physmid = pi.val;
		pi = decode_int(pi.prop);
		physlo = pi.val;
		pi = decode_int(pi.prop);
		sizehi = pi.val;
		pi = decode_int(pi.prop);
		sizelo = pi.val;

	} while (pi.prop.len > 0 && PCI_PHYSHI_SPACE(physhi) != PCI_SPACE_MEM);

	/* enable I/O and memory */
	cfg = config_getl(PCI_CONFIG_COMMAND);
	config_setl(PCI_CONFIG_COMMAND,
			(cfg & 0xFFFF) | PCI_COMMAND_IOENABLE | PCI_COMMAND_MEMENABLE);

	/* initialize frame-buffer */
	g_framebufsize = SCREEN_HEIGHT * SCREEN_WIDTH;
	g_framebuf = map_in(physhi, physmid, physlo, g_framebufsize);

	if (g_framebuf == NULL)
	{
		type("\$screen-open: could not map in frame-buffer!\r\n");
		abort();
	}

	set_frame_buffer_adr(g_framebuf);

	/* set ISO-standard terminal colors */
	set_colors((char*)g_iso_colors, 0, ISO_COLORS);
	color_set(0xFF, 0xFF, 0xFF, 255);

	/* get the default font info and set it up for use */
	fi = default_font();
	set_font(fi);

	/* work around deficient/braindead display packages for window size */
	l = SCREEN_HEIGHT / fi.height;
	c = SCREEN_WIDTH / fi.width;

	/* let fb8 package do the rest */
	fb8_install(SCREEN_WIDTH, SCREEN_HEIGHT, c, l);
}

static void
screen_close(void)			/* screen-close (--) */
{
	int cfg;

	cfg = config_getl(PCI_CONFIG_COMMAND);
	config_setl(PCI_CONFIG_COMMAND, cfg & 0xFFF8);
	
	map_out(g_framebuf, g_framebufsize);
}

static void
screen_selftest(void)		/* selftest (--) */
{
	Bool fail, diag, pmask;

	diag = diagnostic_mode();

	if (diag)
		type("\$screen selftest...");

	pmask = mask;
	mask = 0xFFFFFFFF;
	fail = memory_test_suite(g_framebuf, g_framebufsize);
	mask = pmask;

	if (fail)
	{
		if (diag)
			type("\$FAILED!\r\n");

		abort();
	}

	if (diag)
		type("\$ok\r\n");
}

static Fstr
encode_reg(Fstr s1, int pcireg)
{
	int addr, reg, save, set, cfg;
	int t = 0;
	int p = 0;
	int size = 0;
	int ptype = 0;
	int space = 0;
	int physhi = 0;
	Fstr s2;

	addr = my_space() & 0xFFFFFF00;
	reg = pcireg | addr;

	/* disable device before probing BARs for size */
	cfg = config_getl(addr | PCI_CONFIG_COMMAND);
	config_setl(cfg & 0xFFF8, addr | PCI_CONFIG_COMMAND);

	save = config_getl(reg);
	config_setl(0xFFFFFFFF, reg);
	set = config_getl(reg);
	config_setl(save, reg);

	if (set & 1)
	{
		/* i/o space */
		size = (~set | 3) + 1;
		size &= -size;		/* clear any un-programable upper bits */
		ptype = set & 3;
	}
	else
	{
		/* memory space */
		size = (~set | 0xF) + 1;
		size &= -size;		/* clear any un-programable upper bits */
		ptype = set & 0xF;
	}

	if (ptype & 1)
	{
		space = PCI_SPACE_IO;		/* i/o space */
		t = (ptype >> 1) & 1;
	}
	else
	{
		/* memory space */
		p = (ptype >> 3) & 1;
		space = PCI_SPACE_MEM;

		if ((ptype & 6) == 2)
			t = 1;	/* 32 bit BAR in lower 1-meg */
	}

	/* calculate size of expansion ROM BAR */
	if (pcireg == PCI_CONFIG_ROMADDR)
	{
		size = (~set | 0x7FF) + 1;
		size &= -size;		/* clear any un-programable upper bits */
		space = PCI_SPACE_MEM;
	}

	/* restore config reg */
	config_setl(cfg & 0xFFFF, addr | PCI_CONFIG_COMMAND);

	physhi = PCI_PHYSHI_MK(0, p, t, space, /*bus*/ 0, /*dev*/ 0,
			/*func*/ 0, reg);

	if (size)
	{
		s2 = encode_phys(0, 0, physhi);
		s1 = encodeplus(s1, s2);

		s2 = encode_int(0);
		s1 = encodeplus(s1, s2);

		s2 = encode_int(size);
		s1 = encodeplus(s1, s2);
	}

	return s1;
}

void
main(void)
{
	Fstr s1, s2;
	Phys3 phys;

	/* create this device's name and type */
	device_name("\$screen");
	device_type("\$display");

#ifdef STANDALONE
	/* setup the basic properties for this device */
	make_properties();
#endif

	/* setup first part of "reg" property for our config addr */
	phys = my_address2();
	phys.hi = my_space();
	s1 = encode_phys(phys.lo, phys.mid, phys.hi);
	s2 = encode_int(0);
	s1 = encodeplus(s1, s2);
	s2 = encode_int(0);
	s1 = encodeplus(s1, s2);

	/* append to "reg" property for BAR[0-5] and ROMADDR config registers */
	s1 = encode_reg(encode_reg(s1, PCI_CONFIG_BAR0), PCI_CONFIG_BAR1);
	s1 = encode_reg(s1, PCI_CONFIG_BAR2);
	s1 = encode_reg(s1, PCI_CONFIG_BAR3);
	s1 = encode_reg(s1, PCI_CONFIG_BAR4);
	s1 = encode_reg(s1, PCI_CONFIG_BAR5);
	s1 = encode_reg(s1, PCI_CONFIG_ROMADDR);

	/* finally save it all as the "reg" property */
	property(s1, "\$reg");

	/* set property for 8-bit color display package */
	property(encode_string("\$"), "\$iso6429-1983-colors");

	/* need to pass in the xtok for our open routine to is-install */
	is_install((Xtok)screen_open);
			/* add open, write, draw-logo, and restore */
	is_remove((Xtok)screen_close);		/* adds a close method */
	is_selftest((Xtok)screen_selftest);	/* adds a selftest method */

#ifdef STANDALONE
	// needs to be here so it is the last thing executed
	finish_device();
	close_dev(my_self());
#endif /* STANDALONE */
}
