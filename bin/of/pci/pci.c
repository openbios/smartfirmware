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

/* (c) Copyright 1996-1999 by CodeGen, Inc.  All Rights Reserved */

/*#define DEBUG*/

#include "defs.h"
#include "stdlib.h"
#include "pci.h"

Int
pci_find_host_bridge_number(Environ *e, Package *pkg)
{
	static Package *last = NULL;
	static Int lastnum;
	static Int max = -1;

	if (max == -1)
		max = pci_num_host_bridges();

	if (max < 2)
		return 0;

	/* we have more than host to pci bridge */

	/* keep one cached copy to speed things up. If this isn't sufficient */
	/* a larger cache should be maintained. */
	if (last == pkg)
		return lastnum;

	/* search the package tree to find a device node with the property */
	/* pci-bridge-number set, and return the value of the property */
	for (last = pkg; pkg != NULL && pkg != e->root; pkg = pkg->parent)
		if (prop_get_int(pkg->props, "pci-bridge-number", CSTR, &lastnum) ==
				NO_ERROR)
			return lastnum;

	last = NULL;
	return 0;
}

C(f_pci_open)			/* open (-- okay?) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, FTRUE);
	return NO_ERROR;
}

C(f_pci_close)			/* close (--) */
{
	return NO_ERROR;
}

static Retcode
pci_lookup_assigned_address(Package *pkg, uInt hi, uInt *mid, uInt *lo)
{
	Entry *aaprop;
	Byte *aap;
	uInt aalen;

	DPRINTF(("pci_lookup_assigned_address: hi %#x\n", hi));

	hi |= PCI_PHYSHI_MK(1, 0, 0, 0, 0, 0, 0, 0);
	DPRINTF(("pci_lookup_assigned_address: new hi %#x\n", hi));
	
	/* get the assigned-addresses property */
	aaprop = find_table(pkg->props, "assigned-addresses", CSTR);

	DPRINTF(("pci_lookup_assigned_address: aaprop %#p\n", aaprop));

	if (aaprop == NULL)
		return E_NO_PROPERTY;

	aap = (Byte *)aaprop->v.array;
	aalen = aaprop->len;

	DPRINTF(("pci_lookup_assigned_address: aap %p aalen %d\n", aap, aalen));

	while (aalen)
	{
		Int physhi;
		Int physmid;
		Int physlo;
		Int sizehi;
		Int sizelo;
		Retcode err;

		err = pci_decode_reg_prop(&aap, &aalen, &physhi, &physmid, &physlo,
				&sizehi, &sizelo);

		DPRINTF(("pci_lookup_assigned_address: entry: phys (%#x %#x %#x), size (%#x %#x)\n", physhi, physmid, physlo, sizehi, sizelo));

		if (err != NO_ERROR)
		{
			DPRINTF(("pci_lookup_assigned_address: error decoding assigned-address property\n"));
			return err;
		}

		DPRINTF(("assign addr: reg, phys %#X,%#X,%#X, size %#X,%#X\n", physhi, physmid, physlo, sizehi, sizelo));
		DPRINTF(("assign addr: reg, n %d, space %d, bus %d, dev %d, func %d, reg %02X\n", PCI_PHYSHI_N(physhi), PCI_PHYSHI_SPACE(physhi), PCI_PHYSHI_BUS(physhi), PCI_PHYSHI_DEV(physhi), PCI_PHYSHI_FUNC(physhi), PCI_PHYSHI_REG(physhi)));

		if (physhi == hi)
		{
			*mid = physmid;
			*lo = physlo;
			return NO_ERROR;
		}
	}

	return E_PROPERTY_ERROR;
}

C(f_pci_map_in)			/* map-in (phys.lo phys.mid phys.hi size -- virt) */
{
	Int phys_low;
	Int phys_mid;
	Int phys_hi;
	Cell size;
	Cell virt;
	Int hbridge;
	Retcode ret;

	IFCKSP(e, 4, 1);
	POP(e, size);
	POP(e, phys_hi);
	POP(e, phys_mid);
	POP(e, phys_low);
	hbridge = pci_find_host_bridge_number(e, e->currpkg);

	if (PCI_PHYSHI_N(phys_hi) == 0)
	{
		uInt mid;
		uInt low;
		
		/* relocate address */
		ret = pci_lookup_assigned_address(e->currpkg, phys_hi, &mid, &low);

		DPRINTF(("Original base %#x %#x\n", phys_mid, phys_low));
		DPRINTF(("Assigned address %d, %#x -> %#x %#x\n", ret, phys_hi, mid, low));

		if (ret == NO_ERROR)
		{
			phys_low += low;
			phys_mid += mid + (phys_low < low);
		}

		DPRINTF(("New base %#x %#x\n", phys_mid, phys_low));
	}

	ret = pci_map_in(hbridge, phys_hi, phys_mid, phys_low, size, &virt);
	PUSH(e, virt);
	return ret;
}

C(f_pci_map_out)		/* map-out (virt size --) */
{
	Cell virt;
	Cell size;
	Int hbridge;

	IFCKSP(e, 2, 0);
	POP(e, size);
	POP(e, virt);
	hbridge = pci_find_host_bridge_number(e, e->currpkg);
	return pci_map_out(hbridge, virt, size);
}

C(f_pci_dma_alloc)		/* dma-alloc (size -- virt) */
{
	Cell virt;
	Cell size;
	Int hbridge;
	Retcode ret;

	IFCKSP(e, 1, 1);
	POP(e, size);
	hbridge = pci_find_host_bridge_number(e, e->currpkg);
	ret = pci_dma_alloc(hbridge, size, &virt);
	PUSH(e, virt);
	return ret;
}

C(f_pci_dma_free)		/* dma-free (virt size --) */
{
	Cell virt;
	Cell size;
	Int hbridge;

	IFCKSP(e, 2, 0);
	POP(e, size);
	POP(e, virt);
	hbridge = pci_find_host_bridge_number(e, e->currpkg);
	return pci_dma_free(hbridge, virt, size);
}

C(f_pci_dma_map_in)		/* dma-map-in (virt size cacheable? -- devaddr) */
{
	Cell virt;
	Cell size;
	Int cacheable;
	Int hbridge;
	Cell devaddr;
	Retcode ret;

	IFCKSP(e, 3, 1);
	POP(e, cacheable);
	POP(e, size);
	POP(e, virt);
	hbridge = pci_find_host_bridge_number(e, e->currpkg);
	ret = pci_dma_map_in(hbridge, virt, size, cacheable, &devaddr);
	PUSH(e, devaddr);
	return ret;
}

C(f_pci_dma_map_out)	/* dma-map-out (virt devaddr size --) */
{
	Cell virt;
	Cell devaddr;
	Cell size;
	Int hbridge;

	IFCKSP(e, 3, 0);
	POP(e, size);
	POP(e, devaddr);
	POP(e, virt);
	hbridge = pci_find_host_bridge_number(e, e->currpkg);
	return pci_dma_map_out(hbridge, virt, devaddr, size);
}

C(f_pci_dma_sync)		/* dma-sync (virt devaddr size --) */
{
	Cell virt;
	Cell devaddr;
	Cell size;
	Int hbridge;

	IFCKSP(e, 3, 0);
	POP(e, size);
	POP(e, devaddr);
	POP(e, virt);
	hbridge = pci_find_host_bridge_number(e, e->currpkg);
	return pci_dma_sync(hbridge, virt, devaddr, size);
}

C(f_pci_decode_unit)	/* decode-unit (str len -- phys.lo phys.mid phys.hi) */
{
	Package *pkg;
	Byte *str;
	Int slen;
	Bool n = 0;
	Bool p = 0;
	Bool t = 0;
	int space = 0;
	Int bus = 0;
	Cell dev = 0;
	Cell func = 0;
	Cell reg = 0;
	Cell hi = 0;
	Cell lo = 0;
	Cell err;

	IFCKSP(e, 2, 3);
	POP(e, slen);
	POPT(e, str, Byte *);
	setstrlen(&str, &slen);

	if (slen && (str[0] == 'n' || str[0] == 'N'))
	{
		n = 1;
		str++;
		slen--;
	}

	if (slen)
		switch (str[0])
		{
		case 'i':
		case 'I':
			space = PCI_SPACE_IO;
			str++;
			slen--;
			break;
		case 'm':
		case 'M':
			space = PCI_SPACE_MEM;
			str++;
			slen--;
			break;
		case 'x':
		case 'X':
			space = PCI_SPACE_XMEM;
			str++;
			slen--;
			break;
		case 'u':
		case 'U':
			space = PCI_SPACE_BUS;
			str++;
			slen--;
			break;
		}

	if (slen && (str[0] == 't' || str[0] == 'T'))
	{
		t = 1;
		str++;
		slen--;
	}

	if (slen && (str[0] == 'p' || str[0] == 'P'))
	{
		p = 1;
		str++;
		slen--;
	}

	parse_number(16, &str, &slen, &dev, &err, FALSE);

	if (slen && str[0] == ',')
	{
		str++;
		slen--;
		parse_number(16, &str, &slen, &func, &err, FALSE);

		if (slen && str[0] == ',')
		{
			str++;
			slen--;
			parse_number(16, &str, &slen, &reg, &err, FALSE);

			if (slen && str[0] == ',')
			{
				Int i;
				str++;
				slen--;

				/* this is a hack for 64-bit addresses */
				for (i = 0; i < slen; i++)
					if (!isxdigit(str[i]))
						break;

				if (i <= 8)
					parse_number(16, &str, &slen, &lo, &err, FALSE);
				else
				{
					Byte *ostr = str;
					i -= 8;
					parse_number(16, &str, &i, &hi, &err, FALSE);
					i += 8;
					parse_number(16, &str, &i, &lo, &err, FALSE);
					slen -= str - ostr;
				}
			}
		}
	}

	switch (space)
	{
	case PCI_SPACE_CONFIG:
		n = 0;
		p = 0;
		t = 0;
		hi = 0; 
		lo = 0;
		break;
	case PCI_SPACE_IO:
		p = 0;
		hi = 0; 
		break;
	case PCI_SPACE_MEM:
		hi = 0; 
		break;
	case PCI_SPACE_XMEM:
		t = 0;
		break;
	case PCI_SPACE_BUS:
		p = 0;
		t = 0;
		hi = 0; 
		lo &= 0xFF;
		break;
	}

	/* get the bus number from the bus controller */
	for (pkg = e->currpkg; pkg != NULL && pkg != e->root; pkg = pkg->parent)
	{
		if (prop_get_int(pkg->props, "bus-range", CSTR, &bus) == NO_ERROR)
			break;

		bus = 0;
	}

	PUSH(e, lo);
	PUSH(e, hi);
	PUSH(e, PCI_PHYSHI_MK(n, p, t, space, bus, dev, func, reg));

	return NO_ERROR;
}

C(f_pci_encode_unit)	/* encode-unit (phys.lo phys.mid phys.hi -- str len) */
{
	Cell physhi;
	Cell hi;
	Cell lo;
	static Byte buf[STR_SIZE];

	IFCKSP(e, 3, 2);
	POP(e, physhi);
	POP(e, hi);
	POP(e, lo);

	switch (PCI_PHYSHI_SPACE(physhi))
	{
	case PCI_SPACE_CONFIG:	/* config */
		if (PCI_PHYSHI_FUNC(physhi))
			bprintf(buf, "%x,%x", PCI_PHYSHI_DEV(physhi),
					PCI_PHYSHI_FUNC(physhi));
		else
			bprintf(buf, "%x", PCI_PHYSHI_DEV(physhi));

		break;
	case PCI_SPACE_IO:	/* i/o */
		bprintf(buf, "%si%s%x,%x,%x,%x", PCI_PHYSHI_N(physhi) ? "n" : "",
				PCI_PHYSHI_T(physhi) ? "t" : "", PCI_PHYSHI_DEV(physhi),
				PCI_PHYSHI_FUNC(physhi), PCI_PHYSHI_REG(physhi), (uInt)lo);
		break;
	case PCI_SPACE_MEM:	/* mem 32bit addr */
		bprintf(buf, "%sm%s%s%x,%x,%x,%x", PCI_PHYSHI_N(physhi) ? "n" : "",
				PCI_PHYSHI_T(physhi) ? "t" : "", 
				PCI_PHYSHI_P(physhi) ? "p" : "", PCI_PHYSHI_DEV(physhi),
				PCI_PHYSHI_FUNC(physhi), PCI_PHYSHI_REG(physhi), (uInt)lo);
		break;
	case PCI_SPACE_XMEM:	/* mem 64bit addr */
		if (hi)
			bprintf(buf, "%sx%s%x,%x,%x,%x%08x",
					PCI_PHYSHI_N(physhi) ? "n" : "",
					PCI_PHYSHI_P(physhi) ? "p" : "", PCI_PHYSHI_DEV(physhi),
					PCI_PHYSHI_FUNC(physhi), PCI_PHYSHI_REG(physhi), (uInt)hi,
					(uInt)lo);
		else
			bprintf(buf, "%sx%s%x,%x,%x,%x", PCI_PHYSHI_N(physhi) ? "n" : "",
					PCI_PHYSHI_P(physhi) ? "p" : "", PCI_PHYSHI_DEV(physhi),
					PCI_PHYSHI_FUNC(physhi), PCI_PHYSHI_REG(physhi), (uInt)lo);
		break;
	case PCI_SPACE_BUS:	/* bus */
		bprintf(buf, "%su%x,%x,%x,%x", PCI_PHYSHI_N(physhi) ? "n" : "",
				PCI_PHYSHI_DEV(physhi), PCI_PHYSHI_FUNC(physhi),
				PCI_PHYSHI_REG(physhi), (uInt)lo & 0xFF);
		break;
	default:
		bprintf(buf, "???%d", PCI_PHYSHI_SPACE(physhi));
		break;
	}

	PUSHP(e, buf);
	PUSH(e, strlen(buf));

	return NO_ERROR;
}


#ifdef BIG_ENDIAN

/* need to override the default reg read/write words for Fcode so that they
   perform the necessary byte-swapping for words and longs so they appear
   to be little-endian
 */

EC(f_rwget);			/* rw@ (waddr -- w) 0x232 */
EC(f_rwset);			/* rw! (w waddr --) 0x233 */
EC(f_rlget);			/* rl@ (qaddr -- quad) 0x234 */
EC(f_rlset);			/* rl! (quad qaddr --) 0x235 */


static Entry *rwget_xtok, *rlget_xtok, *rwset_xtok, *rlset_xtok;

C(f_pci_rwget)			/* pci-rw@ (waddr -- w) */
{
	Retcode ret = f_rwget(e);
	TOP(e) = LE2(TOP(e));
	return ret;
}

C(f_pci_rwset)			/* pci-rw! (w waddr --) */
{
	e->sp[-1] = LE2(e->sp[-1]);
	return f_rwset(e);
}

C(f_pci_rlget)			/* pci-rl@ (qaddr -- quad) */
{
	Retcode ret = f_rlget(e);
	TOP(e) = LE4(TOP(e));
	return ret;
}

C(f_pci_rlset)			/* pci-rl! (quad qaddr --) */
{
	e->sp[-1] = LE4(e->sp[-1]);
	return f_rlset(e);
}
#endif


/* interp the fcode, take care to override and replace the reg I/O words
   above if on a big-endian system
 */
Retcode
pci_interp_fcode(Environ *e, Byte *fcode)
{
	Retcode ret;

#ifdef BIG_ENDIAN
/*	Entry *rwget_sav, *rlget_sav, *rwset_sav, *rlset_sav; */
	int rwget_sav, rlget_sav, rwset_sav, rlset_sav;

	rwget_sav = e->fcodes[0x232];
	rwset_sav = e->fcodes[0x233];
	rlget_sav = e->fcodes[0x234];
	rlset_sav = e->fcodes[0x235];
	e->fcodes[0x232] = rwget_xtok->xtok;
	e->fcodes[0x233] = rwset_xtok->xtok;
	e->fcodes[0x234] = rlget_xtok->xtok;
	e->fcodes[0x235] = rlset_xtok->xtok;
#endif

	ret = interp_fcode(e, fcode, MAGIC_FNEXT);

#ifdef BIG_ENDIAN
	e->fcodes[0x232] = rwget_sav;
	e->fcodes[0x233] = rwset_sav;
	e->fcodes[0x234] = rlget_sav;
	e->fcodes[0x235] = rlset_sav;
#endif

	return ret;
}

/* probe-self (arg-str arg-len reg-str reg-len fcode-str fcode-len --) */
C(f_pci_probe_self)
{
	Retcode ret;
	Byte *args, *reg;
	Int alen, rlen;
	Int fcodelo, fcodemid, fcodehi;
	Int physhi;
	Int hbridge;
	Cell fcode;
	Byte str[STR_SIZE];

	IFCKSP(e, 6, 0);

	/* convert fcode unit addr to our usual lo-mid-hi values */
	ret = f_pci_decode_unit(e);

	if (ret != NO_ERROR)
		return ret;

	POP(e, fcodehi);
	POP(e, fcodemid);
	POP(e, fcodelo);

	/* pop off rest of args passed to us */
	POP(e, rlen);
	POPT(e, reg, Byte*);
	POP(e, alen);
	POPT(e, args, Byte*);

	/* decode device probe-address reg string */
	ret = pci_decode_reg_prop(&reg, &rlen, &physhi, NULL, NULL, NULL, NULL);

	/* map in device to read fcode */
	hbridge = pci_find_host_bridge_number(e, e->currpkg);
	ret = pci_map_in(hbridge, physhi, fcodemid, fcodelo, 0x10000, &fcode);

	/* copy args str into buffer to convert it to a PSTR */
	str[0] = (uByte)alen;
	memcpy(str + 1, args, alen);

	if (ret == NO_ERROR)
		ret = execute_word(e, "new-device");

	/* set new instance arguments */
	if (ret == NO_ERROR)
		((Instance*)(uPtr)e->currinst)->args = str;

	/* run the fcode */
	if (ret == NO_ERROR)
		ret = pci_interp_fcode(e, (Byte*)(uPtr)fcode);

	/* clean up */
	pci_map_out(hbridge, fcode, 0x10000);

	if (ret == NO_ERROR)
		ret = execute_word(e, "finish-device");

	return NO_ERROR;
}

C(f_pci_config_getquad)		/* config-l@ (config-addr -- data) */
{
	Int addr;
	Int val;
	Int hbridge;

	IFCKSP(e, 1, 1);
	POP(e, addr);
	hbridge = pci_find_host_bridge_number(e, e->currpkg);

	if (PCI_PHYSHI_N(addr) != 0 || PCI_PHYSHI_P(addr) != 0 ||
			PCI_PHYSHI_T(addr) != 0 || PCI_PHYSHI_SPACE(addr) != 0)
		val = 0;
	else
		val = pci_config_read(hbridge,
				PCI_PHYSHI_BUS(addr), PCI_PHYSHI_DEV(addr),
				PCI_PHYSHI_FUNC(addr), PCI_PHYSHI_REG(addr));

	PUSH(e, val);
	return NO_ERROR;
}

C(f_pci_config_setquad)		/* config-l! (data config-addr --) */
{
	Int addr;
	Int val;
	Int hbridge;

	IFCKSP(e, 2, 0);
	POP(e, addr);
	POP(e, val);

	if (PCI_PHYSHI_N(addr) != 0 || PCI_PHYSHI_P(addr) != 0 ||
			PCI_PHYSHI_T(addr) != 0 || PCI_PHYSHI_SPACE(addr) != 0)
		return NO_ERROR;

	hbridge = pci_find_host_bridge_number(e, e->currpkg);
	pci_config_write(hbridge, PCI_PHYSHI_BUS(addr), PCI_PHYSHI_DEV(addr),
				PCI_PHYSHI_FUNC(addr), PCI_PHYSHI_REG(addr), val);
	return NO_ERROR;
}

C(f_pci_config_getdoub)		/* config-w@ (config-addr -- data) */
{
	Int addr;
	Int val;
	Int hbridge;

	IFCKSP(e, 1, 1);
	POP(e, addr);
	hbridge = pci_find_host_bridge_number(e, e->currpkg);

	if (PCI_PHYSHI_N(addr) != 0 || PCI_PHYSHI_P(addr) != 0 ||
			PCI_PHYSHI_T(addr) != 0 || PCI_PHYSHI_SPACE(addr) != 0)
		val = 0;
	else
		val = pci_config_read(hbridge,
				PCI_PHYSHI_BUS(addr), PCI_PHYSHI_DEV(addr),
				PCI_PHYSHI_FUNC(addr), PCI_PHYSHI_REG(addr));

	switch (addr & 3)
	{
	case 0:
		val = val & 0xFFFF;
		break;
	case 1:
		val = (val >> 8) & 0xFFFF;
		break;
	case 2:
		val = (val >> 16) & 0xFFFF;
		break;
	case 3:
		val = ((val << 8) & 0xFF00) | ((val >> 24) & 0xFF);
		break;
	}

	PUSH(e, val);
	return NO_ERROR;
}

C(f_pci_config_setdoub)		/* config-w! (data config-addr --) */
{
	Int addr;
	Int val;
	Int oval;
	Int mask = 0;
	Int hbridge;

	IFCKSP(e, 2, 0);
	POP(e, addr);
	POP(e, val);

	if (PCI_PHYSHI_N(addr) != 0 || PCI_PHYSHI_P(addr) != 0 ||
			PCI_PHYSHI_T(addr) != 0 || PCI_PHYSHI_SPACE(addr) != 0)
		return NO_ERROR;

	switch (addr & 3)
	{
	case 0:
		mask = 0xFFFF;
		break;
	case 1:
		val <<= 8;
		mask = 0xFFFF00;
		break;
	case 2:
		val <<= 16;
		mask = 0xFFFF0000;
		break;
	case 3:
		val = ((val >> 8) & 0xFF) | (val << 24);
		mask = 0xFF0000FF;
		break;
	}

	hbridge = pci_find_host_bridge_number(e, e->currpkg);
	oval = pci_config_read(hbridge, PCI_PHYSHI_BUS(addr), PCI_PHYSHI_DEV(addr),
				PCI_PHYSHI_FUNC(addr), PCI_PHYSHI_REG(addr));

	val &= mask;
	val |= oval & ~mask;
	pci_config_write(hbridge, PCI_PHYSHI_BUS(addr), PCI_PHYSHI_DEV(addr),
				PCI_PHYSHI_FUNC(addr), PCI_PHYSHI_REG(addr), val);
	return NO_ERROR;
}

C(f_pci_config_getbyte)		/* config-b@ (config-addr -- data) */
{
	Int addr;
	Int val;
	Int hbridge;

	IFCKSP(e, 1, 1);
	POP(e, addr);
	hbridge = pci_find_host_bridge_number(e, e->currpkg);

	if (PCI_PHYSHI_N(addr) != 0 || PCI_PHYSHI_P(addr) != 0 ||
			PCI_PHYSHI_T(addr) != 0 || PCI_PHYSHI_SPACE(addr) != 0)
		val = 0;
	else
		val = pci_config_read(hbridge,
				PCI_PHYSHI_BUS(addr), PCI_PHYSHI_DEV(addr),
				PCI_PHYSHI_FUNC(addr), PCI_PHYSHI_REG(addr));

	switch (addr & 3)
	{
	case 0:
		val = val & 0xFF;
		break;
	case 1:
		val = (val >> 8) & 0xFF;
		break;
	case 2:
		val = (val >> 16) & 0xFF;
		break;
	case 3:
		val = (val >> 24) & 0xFF;
		break;
	}

	PUSH(e, val);
	return NO_ERROR;
}

C(f_pci_config_setbyte)		/* config-b! (data config-addr --) */
{
	Int addr;
	Int val;
	Int oval;
	Int mask = 0;
	Int hbridge;

	IFCKSP(e, 2, 0);
	POP(e, addr);
	POP(e, val);

	if (PCI_PHYSHI_N(addr) != 0 || PCI_PHYSHI_P(addr) != 0 ||
			PCI_PHYSHI_T(addr) != 0 || PCI_PHYSHI_SPACE(addr) != 0)
		return NO_ERROR;

	switch (addr & 3)
	{
	case 0:
		mask = 0xFF;
		break;
	case 1:
		val <<= 8;
		mask = 0xFF00;
		break;
	case 2:
		val <<= 16;
		mask = 0xFF0000;
		break;
	case 3:
		val <<= 24;
		mask = 0xFF000000;
		break;
	}

	hbridge = pci_find_host_bridge_number(e, e->currpkg);
	oval = pci_config_read(hbridge, PCI_PHYSHI_BUS(addr), PCI_PHYSHI_DEV(addr),
				PCI_PHYSHI_FUNC(addr), PCI_PHYSHI_REG(addr));

	val &= mask;
	val |= oval & ~mask;
	pci_config_write(hbridge, PCI_PHYSHI_BUS(addr), PCI_PHYSHI_DEV(addr),
				PCI_PHYSHI_FUNC(addr), PCI_PHYSHI_REG(addr), val);
	return NO_ERROR;
}

C(f_pci_intr_ack)		/* intr-ack (--) */
{
	return pci_intr_ack(pci_find_host_bridge_number(e, e->currpkg));
}

C(f_pci_special_set)	/* special-! (data bus# --) */
{
	Int data;
	Int bus;
	Int hbridge;
	IFCKSP(e, 2, 0);
	POP(e, bus);
	POP(e, data);
	hbridge = pci_find_host_bridge_number(e, e->currpkg);
	return pci_special_cycle(hbridge, bus, data);
}

static Int
config_read(Pci_device_info *devinfo, Int addr)
{
	return pci_config_read(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, addr);
}

static void
config_write(Pci_device_info *devinfo, Int addr, Int value)
{
	pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, addr, value);
}

typedef struct _PCI_VENTABLE
{
	unsigned short	VenId ;
	char *	VenShort ;
	char *	VenFull ;
}  PCI_VENTABLE, *PPCI_VENTABLE ;

extern PCI_VENTABLE	PciVenTable[];

typedef struct _PCI_DEVTABLE
{
	unsigned short	VenId ;
	unsigned short	DevId ;
	char *	Chip ;
	char *	ChipDesc ;
}  PCI_DEVTABLE, *PPCI_DEVTABLE ;

extern PCI_DEVTABLE PciDevTable[];

static char *
vendname(int id, int null)
{
	int i;

	for (i = 0; PciVenTable[i].VenId != 0xFFFF; i++)
		if (PciVenTable[i].VenId == id)
		{
			if (PciVenTable[i].VenShort[0])
				return PciVenTable[i].VenShort;

			break;
		}

	return null ? NULL : "";
}

static char *
partnum(int venid, int devid, int null)
{
	int i;

	for (i = 0; PciDevTable[i].VenId != 0xFFFF; i++)
		if (PciDevTable[i].VenId == venid && PciDevTable[i].DevId == devid)
		{
			if (PciDevTable[i].Chip[0])
				return PciDevTable[i].Chip;

			break;
		}

	return null ? NULL : "";
}

static char *
partdesc(int venid, int devid, int null)
{
	int i;

	for (i = 0; PciDevTable[i].VenId != 0xFFFF; i++)
		if (PciDevTable[i].VenId == venid && PciDevTable[i].DevId == devid)
		{
			if (PciDevTable[i].ChipDesc[0])
				return PciDevTable[i].ChipDesc;

			break;
		}

	return null ? NULL : "";
}

void
pci_dump_rom_header(int hbridge, uInt addr)
{
	int i;

	for (i = 0; i < 0x80; i++)
	{
		uInt v;

		if ((i & 0xF) == 0)
			DPRINTF(("%08X: ", i));

		if ((i & 0xF) == 8)
			DPRINTF((" "));

		pci_mem_read(hbridge, addr + (i & ~3), &v, 4);
		DPRINTF((" %02X", (v >> (8 * (i & 3))) & 0xFF));

		if ((i & 0xF) == 0xF)
			DPRINTF(("\n"));
	}
}

Retcode
pci_load_expansion_rom(Environ *e, Package *pkg, Pci_device_info *devinfo,
		Pci_addresses *addrs)
{
	Int hbridge = devinfo->hbridge;
	uInt save;
	uInt size;
	uInt romaddr;
	uInt fcodeoffset = 0;
	Retcode err = NO_ERROR;
	int i;
	Cell virt;
	Byte *rom;

#ifdef PCI_NO_EXPANSION_ROM
	return E_NO_FCODE;
#endif

	if (devinfo->methods == NULL)
		return E_NO_FCODE;

	DPRINTF(("pci_load_expansion_rom: calling romsize...\n"));
	err = (*devinfo->methods->romsize)(e, pkg, devinfo, &size);

	if (err != NO_ERROR)
		return err;

	if (!size)
		return E_NO_FCODE;

	/* allocate addresss space for it */
	romaddr = pci_allocate_mem32(addrs, size);

	if (romaddr == 0)
		return E_NO_FCODE;

	/* load the BAR */
	DPRINTF(("pci_load_expansion_rom: romsize=%#x romaddr=%#x\n",
			size, romaddr));
	err = (*devinfo->methods->writeromaddr)(e, pkg, devinfo, romaddr);

	if (err)
	{
		pci_free_mem_range(addrs, 0, romaddr, 0, size);
		return err;
	}

	/* blow away existing BARs */
	/* XXX: should really assign them temporary legal values */
	config_write(devinfo, PCI_CONFIG_BAR0, 0);
	config_write(devinfo, PCI_CONFIG_BAR1, 0);
	config_write(devinfo, PCI_CONFIG_BAR2, 0);
	config_write(devinfo, PCI_CONFIG_BAR3, 0);
	config_write(devinfo, PCI_CONFIG_BAR4, 0);
	config_write(devinfo, PCI_CONFIG_BAR5, 0);

	/* save a copy of the control register */
	save = config_read(devinfo, PCI_CONFIG_COMMAND);

	/* set the memory enable bit */
	config_write(devinfo, PCI_CONFIG_COMMAND, save | PCI_COMMAND_MEMENABLE);
	DPRINTF(("pci_load_expansion_rom: enabled memory-access\n"));

	/* map-in ROM to get virtual address */
	err = pci_map_in(hbridge, PCI_PHYSHI_MK(1, 0, 0, PCI_SPACE_MEM,
			devinfo->bus, devinfo->dev, devinfo->func, 0),
			0, romaddr, size, &virt);

	if (err != NO_ERROR)
	{
		pci_free_mem_range(addrs, 0, romaddr, 0, size);
		return err;
	}

	rom = (Byte*)(uPtr)virt;
	DPRINTF(("pci_load_expansion_rom: rom=%#x, romaddr=%p\n", rom, romaddr));


	/* search the expansion rom for the magic number 55 AA */
	for (i = 0; i + 511 < size; i += 512)
	{
		uInt p, value;
		uByte id0;
		uByte id1;
		uByte id2;
		uByte id3;
		uByte off0;
		uByte off1;

		pci_mem_read(hbridge, romaddr + i, &id0, 1);
		pci_mem_read(hbridge, romaddr + i + 1, &id1, 1);

		/* check magic number */
		if (id0 != 0x55 || id1 != 0xAA)
			continue;

		DPRINTF(("Magic Number found at offset %06X\n", i));
		pci_dump_rom_header(hbridge, romaddr + i);

		pci_mem_read(hbridge, romaddr + i + 2, &off0, 1);
		pci_mem_read(hbridge, romaddr + i + 3, &off1, 1);
		fcodeoffset = off0 | (off1 << 8);

		DPRINTF(("FCode Offset %04X\n", fcodeoffset));

		/* if magic found verify that PCI data struct is in ROM */
		pci_mem_read(hbridge, romaddr + i + 0x18, &off0, 1);
		pci_mem_read(hbridge, romaddr + i + 0x19, &off1, 1);
		p = off0 | (off1 << 8);

		DPRINTF(("PCI data structure offset %04X\n", p));

		if (p + 0x18 > size)
			continue;

		DPRINTF(("PCI magic addrress %08X\n", romaddr + i + p));

		/* if PCI data struct in, verify that PCI magic is PCIR */
		pci_mem_read(hbridge, romaddr + i + p + 0, &id0, 1);
		pci_mem_read(hbridge, romaddr + i + p + 1, &id1, 1);
		pci_mem_read(hbridge, romaddr + i + p + 2, &id2, 1);
		pci_mem_read(hbridge, romaddr + i + p + 3, &id3, 1);

		DPRINTF(("PCI magic %02X%02X%02X%02X\n", id0, id1, id2, id3));

		if (id0 != 'P' || id1 != 'C' || id2 != 'I' || id3 != 'R')
			continue;

		/* we have a PCI struct */
		/* check that vendid/devid match the part */
		pci_mem_read(hbridge, romaddr + i + p + 4, &id0, 1);
		pci_mem_read(hbridge, romaddr + i + p + 5, &id1, 1);
		value = id0 | (id1 << 8);

		DPRINTF(("Vendor ID %04X\n", value));

		if (devinfo->vendid != value)
			continue;

		pci_mem_read(hbridge, romaddr + i + p + 6, &id0, 1);
		pci_mem_read(hbridge, romaddr + i + p + 7, &id1, 1);
		value = id0 | (id1 << 8);

		DPRINTF(("Device ID %04X\n", value));

		if (devinfo->devid != value)
			continue;

		/* we now have a code image for this device, make sure that it is */
		/* an OF code image */
		/* check for OF headers in PCI data struct */
		pci_mem_read(hbridge, romaddr + i + p + 0x14, &id0, 1);

		DPRINTF(("PCI ROM Image Type %02X\n", id0));

		if (id0 != 1)
			continue;

		break;
	}

	DPRINTF(("pci_load_expansion_rom: err=%s i=%#x size=%#x\n",
			err2str(err), i, size));

	if (err == NO_ERROR && i + 511 < size)
	{
		uByte *buf;
		int sz = 64*1024;

		/* add an fcode-rom-offset property */
		prop_set_int(pkg->props, "fcode-rom-offset", CSTR, i  + fcodeoffset);

		DPRINTF(("pci_load_expansion_rom: fcodeoffset=%#x\n", i + fcodeoffset));

		if (sz > size - i)
			sz = size - i;

		/* copy the Fcode into a temporary buffer */
		buf = (uByte *)malloc(sz);
	
		if (buf != NULL)
		{
			uInt base = romaddr + i + fcodeoffset;
			int j;

			for (j = 0; j < sz; j++)
				pci_mem_read(hbridge, base + j, &buf[j], 1);
		}
		else
			err = E_OUT_OF_MEMORY;

		/* byte-load load the Fcode */
		if (err == NO_ERROR)
			err = pci_interp_fcode(e, buf);

		free(buf);
	}
	else
		err = E_NO_FCODE;

	/* unmap virt for ROM access */
	pci_map_out(hbridge, virt, size);

	/* disable expansion ROM */
	config_write(devinfo, PCI_CONFIG_COMMAND, save);

	/* reset the BAR */
	(*devinfo->methods->writeromaddr)(e, pkg, devinfo, 0);

	/* free up the memory space */
	pci_free_mem_range(addrs, 0, romaddr, 0, size);

	DPRINTF(("pci_load_expansion_rom: returning %s...\n", err2str(err)));
	return err;
}

Retcode
pci_load_builtin_driver(Environ *e, Package *pkg, Pci_device_info *devinfo,
		Pci_addresses *addrs)
{
	const Pci_driver **driver;
	int hbridge = devinfo->hbridge;
	int bus = devinfo->bus;
	int dev = devinfo->dev;
	int func = devinfo->func;
	int header_type = devinfo->header_type;
	int classcode = devinfo->classcode;
	int vendid = devinfo->vendid;
	int devid = devinfo->devid;
	int subsysvendid = devinfo->subsysvendid;
	int subsysdevid = devinfo->subsysdevid;
	int revisionid = devinfo->revisionid;

	for (driver = pci_drivers; *driver; driver++)
	{
		const Pci_driver *d = *driver;

		if ((hbridge & d->mask.hbridge) != d->match.hbridge)
			continue;

		if ((bus & d->mask.bus) != d->match.bus)
			continue;

		if ((dev & d->mask.dev) != d->match.dev)
			continue;

		if ((func & d->mask.func) != d->match.func)
			continue;

		if ((header_type & d->mask.header_type) != d->match.header_type)
			continue;

		if ((classcode & d->mask.classcode) != d->match.classcode)
			continue;

		if ((vendid & d->mask.vendid) != d->match.vendid)
			continue;

		if ((devid & d->mask.devid) != d->match.devid)
			continue;

		if ((subsysvendid & d->mask.subsysvendid) != d->match.subsysvendid)
			continue;

		if ((subsysdevid & d->mask.subsysdevid) != d->match.subsysdevid)
			continue;

		if ((revisionid & d->mask.revisionid) != d->match.revisionid)
			continue;
		
		return (*d->install)(e, pkg, devinfo, addrs);
	}

	return E_NO_DRIVER;
}

static void
pci_get_device_info(Environ *e, Pci_device_info *devinfo)
{
	Int reg;
	Int type;
	Pci_header_methods const *methods = NULL;
	reg = config_read(devinfo, PCI_CONFIG_ID);
	devinfo->vendid = reg & 0xFFFF;
	devinfo->devid = (reg >> 16) & 0xFFFF;
	reg = config_read(devinfo, PCI_CONFIG_CLASSREV);
	devinfo->classcode = (reg >> 8) & 0xFFFFFF;
	devinfo->revisionid = reg & 0xFF;
	reg = config_read(devinfo, PCI_CONFIG_HEADER);
	type = (reg >> 16) & 0x7F;
	devinfo->header_type = type;

	if (type >= 0 && type < pci_num_headers)
		methods = pci_header_methods[type];

	devinfo->methods = methods;

	if (methods)
		(*methods->getextenedinfo)(e, devinfo);
}

static void
pci_load_standard_properties(Environ *e, Package *pkg, Pci_device_info *devinfo)
{
	Int reg;
	char *str;

	prop_set_int(pkg->props, "vendor-id", CSTR, devinfo->vendid);
	prop_set_int(pkg->props, "device-id", CSTR, devinfo->devid);
	prop_set_int(pkg->props, "revision-id", CSTR, devinfo->revisionid);
	prop_set_int(pkg->props, "class-code", CSTR, devinfo->classcode);
	prop_set_int(pkg->props, "subsystem-id", CSTR, devinfo->subsysdevid);
	prop_set_int(pkg->props, "subsystem-vendor-id", CSTR, devinfo->subsysvendid);

	str = vendname(devinfo->vendid, 1);

	if (str)
		prop_set_str(pkg->props, ".vendor-name", CSTR, str, CSTR);

	str = partnum(devinfo->vendid, devinfo->devid, 1);

	if (str)
		prop_set_str(pkg->props, ".part-number", CSTR, str, CSTR);

	str = partdesc(devinfo->vendid, devinfo->devid, 1);

	if (str)
		prop_set_str(pkg->props, ".description", CSTR, str, CSTR);

	reg = config_read(devinfo, PCI_CONFIG_INTERRUPT);

	if (reg & 0xFF00)
		prop_set_int(pkg->props, "interrupt", CSTR, (reg >> 8) & 0xFF);

	reg = (config_read(devinfo, PCI_CONFIG_COMMAND) >> 16) & 0xFFFF;
	prop_set_int(pkg->props, "devsel-speed", CSTR, (reg >> 9) & 3);

	if (reg & 0x0020)
		add_property(pkg->props, "66mhz-capable", CSTR, NULL, 0);

	if (reg & 0x0040)
		add_property(pkg->props, "udf-supported", CSTR, NULL, 0);

	if (reg & 0x0080)
		add_property(pkg->props, "fast-back-to-back", CSTR, NULL, 0);
}

void
pci_encode_reg_prop(Byte *arr, Int *len, Cell physhi, Cell physmid, Cell physlo,
		Cell sizehi, Cell sizelo)
{
	Int l = *len;
	prop_encode_int(&arr[l], &l, physhi);
	prop_encode_int(&arr[l], &l, physmid);
	prop_encode_int(&arr[l], &l, physlo);
	prop_encode_int(&arr[l], &l, sizehi);
	prop_encode_int(&arr[l], &l, sizelo);
	*len = l;
}

Retcode
pci_decode_reg_prop(Byte **arr, Int *len, Int *physhi, Int *physmid,
		Int *physlo, Int *sizehi, Int *sizelo)
{
	Retcode err;

	if ((err = prop_decode_int(arr, len, physhi)) != NO_ERROR)
		return err;

	if ((err = prop_decode_int(arr, len, physmid)) != NO_ERROR)
		return err;

	if ((err = prop_decode_int(arr, len, physlo)) != NO_ERROR)
		return err;

	if ((err = prop_decode_int(arr, len, sizehi)) != NO_ERROR)
		return err;

	return prop_decode_int(arr, len, sizelo);
}

void
pci_bar_size(Environ *e, Pci_device_info *devinfo, Int reg, Int *ptype,
		Int *psizehi, Int *psizelo)
{
	Int save;
	Int set;
	Int type;
	Int sizehi = 0;
	Int size;

	save = config_read(devinfo, reg);
	config_write(devinfo, reg, 0xFFFFFFFF);
	set = config_read(devinfo, reg);
	config_write(devinfo, reg, save);

	if (set & 1)
	{
		/* i/o space */
		size = (~set | 3) + 1;
		size &= -size;		/* clear any un-programable upper bits */
		type = set & 3;
	}
	else
	{
		/* memory space */
		size = (~set | 0xF) + 1;
		size &= -size;		/* clear any un-programable upper bits */
		type = set & 0xF;

		if ((set & 6) == 4 && size == 0)
		{
			save = config_read(devinfo, reg + 4);
			config_write(devinfo, reg + 4, 0xFFFFFFFF);
			set = config_read(devinfo, reg + 4);
			config_write(devinfo, reg + 4, save);
			sizehi = set & -set;
		}
	}

	if (ptype)
		*ptype = type;

	if (psizehi)
		*psizehi = sizehi;

	if (psizelo)
		*psizelo = size;
}

struct generic_name
{
	Int class;
	Int classmask;
	char *name;
};

struct generic_name generic_names[] =
{
	{ 0x000100, 0x000000, "display" },

	{ 0x010000, 0x0000FF, "scsi" },
	{ 0x010100, 0x0000FF, "ide" },
	{ 0x010200, 0x0000FF, "fdc" },
	{ 0x010300, 0x0000FF, "ipi" },
	{ 0x010400, 0x0000FF, "raid" },

	{ 0x020000, 0x0000FF, "ethernet" },
	{ 0x020100, 0x0000FF, "token-ring" },
	{ 0x020200, 0x0000FF, "fddi" },
	{ 0x020300, 0x0000FF, "atm" },
	{ 0x020400, 0x0000FF, "isdn" },

	{ 0x030000, 0x00FFFF, "display" },

	{ 0x040000, 0x0000FF, "video" },
	{ 0x040100, 0x0000FF, "sound" },
	{ 0x040200, 0x0000FF, "telephone" },

	{ 0x050000, 0x0000FF, "memory" },
	{ 0x050100, 0x0000FF, "flash" },

	{ 0x060000, 0x0000FF, "host" },
	{ 0x060100, 0x0000FF, "isa" },
	{ 0x060200, 0x0000FF, "eisa" },
	{ 0x060300, 0x0000FF, "mca" },
	{ 0x060400, 0x0000FF, "pci" },
	{ 0x060500, 0x0000FF, "pcmcia" },
	{ 0x060600, 0x0000FF, "nubus" },
	{ 0x060700, 0x0000FF, "cardbus" },
	{ 0x060800, 0x0000FF, "raceway" },
	{ 0x068000, 0x0000FF, "other" },

	{ 0x070000, 0x0000FF, "serial" },
	{ 0x070100, 0x0000FF, "parallel" },
	{ 0x070200, 0x0000FF, "serial" },
	{ 0x070300, 0x0000FF, "modem" },

	{ 0x080000, 0x0000FF, "interrupt-controller" },
	{ 0x080100, 0x0000FF, "dma-controller" },
	{ 0x080200, 0x0000FF, "timer" },
	{ 0x080300, 0x0000FF, "rtc" },
	{ 0x080400, 0x0000FF, "hot-plug" },

	{ 0x090000, 0x0000FF, "keyboard" },
	{ 0x090100, 0x0000FF, "pen" },
	{ 0x090200, 0x0000FF, "mouse" },
	{ 0x090300, 0x0000FF, "scanner" },
	{ 0x090400, 0x0000FF, "game-port" },

	{ 0x0A0000, 0x00FFFF, "dock" },

	{ 0x0B0000, 0x00FFFF, "cpu" },

	{ 0x0C0000, 0x0000FF, "firewire" },
	{ 0x0C0100, 0x0000FF, "access-bus" },
	{ 0x0C0200, 0x0000FF, "ssa" },
	{ 0x0C0300, 0x0000FF, "usb" },
	{ 0x0C0400, 0x0000FF, "fibre-channel" },
	{ 0x0C0500, 0x0000FF, "smb" },

	{ 0x0D0000, 0x0000FF, "irda" },
	{ 0x0D0100, 0x0000FF, "wireless-ir" },
	{ 0x0D0200, 0x0000FF, "wireless-rf" },

	{ 0x0E0000, 0x0000FF, "i2o" },

	{ 0x0F0100, 0x0000FF, "tv" },
	{ 0x0F0200, 0x0000FF, "audio" },
	{ 0x0F0300, 0x0000FF, "voice" },
	{ 0x0F0400, 0x0000FF, "data" },

	{ 0x100000, 0x00FFFF, "encryption" },

	{ 0x110000, 0x0000FF, "dpio" },

	{ 0x000000, 0x000000, NULL },
};

void
pci_load_reg_and_name_props(Environ *e, Package *pkg, Pci_device_info *devinfo)
{
	char buf[80];
	char *name = NULL;
	int i;
	Byte *props;
	Int plen = 0;
	Int bus = devinfo->bus;
	Int dev = devinfo->dev;
	Int func = devinfo->func;
	int classcode = devinfo->classcode;

	DPRINTF(("pci: load register and name properties\n"));

	/* look up generic name in generic_names table */
	for (i = 0; generic_names[i].name; i++)
		if ((classcode & ~generic_names[i].classmask) == generic_names[i].class)
		{
			name = generic_names[i].name;
			break;
		}

	if (name == NULL)
	{
		/* no generic name so use pciXXXX,XXXX */
		bprintf(buf, "pci%x,%x", devinfo->vendid, devinfo->devid);
		name = buf;
	}

	/* setup these properties names before creating our reg prop */
	prop_set_str(pkg->props, "name", CSTR, name, CSTR);
	props = prop_alloc(e, 0);

#ifdef ISA_HACK
	/* do not disable ISA devices */
	if (((classcode & 0xFFFF0000) != 0x30000) &&		/* display */
			((classcode & 0xFFFFFFFF) != 0x000100) &&	/* display */
			((classcode & 0xFFFFFF00) != 0x010100))		/* IDE */
#endif
	/* disable the device before probing BAR's */
	config_write(devinfo, PCI_CONFIG_COMMAND, 0);

	/* first reg is always config space */
	(void)prop_alloc(e, 5 * sizeof (Int));
	pci_encode_reg_prop(props, &plen, PCI_PHYSHI_MK(0, 0, 0, PCI_SPACE_CONFIG,
			bus, dev, func, 0), 0, 0, 0, 0);

	DPRINTF(("pci: loadreg: checking for methods\n"));

	if (devinfo->methods)
	{
		uChar *offsets;
		int numoffsets;
		Pci_header_methods const *methods = devinfo->methods;
		DPRINTF(("pci: loadreg: getting address offset list\n"));

		(*methods->addroffsetlist)(e, pkg, devinfo, &offsets,
				&numoffsets);

		DPRINTF(("pci: loadreg: probing BARS\n"));

		/* check each of the BARs */
		for (i = 0; i < numoffsets; i++)
		{
			uInt physhi;
			uInt sizehi;
			uInt sizelo;

#ifdef DEBUG_FULL
			DPRINTF(("pci: loadreg: probing BAR @%02X\n", offsets[i]);
#endif

			(*methods->getaddrinfo)(e, pkg, devinfo, offsets[i], &physhi,
					&sizehi, &sizelo);

			if (!(physhi && (sizehi || sizelo)))
				continue;

			DPRINTF(("pci: loadreg: BAR @%02X, physhi %#X, size %#X,%#X\n",
					offsets[i], physhi, sizehi, sizelo));
			(void)prop_alloc(e, 5 * sizeof (Int));
			pci_encode_reg_prop(props, &plen, physhi, 0, 0, sizehi, sizelo);

			/* skip second register of a 64-bit BAR */
			if (PCI_PHYSHI_SPACE(physhi) == PCI_SPACE_XMEM)
				i++;
		}
	}
	else
		DPRINTF(("pci: loadreg: no methods\n"));

	add_property(pkg->props, "reg", CSTR, props, plen);
	DPRINTF(("pci: loadreg: reg property added\n"));
}

void
pci_dump_blocks(Environ *e, Pci_address_block *b)
{
	for (; b; b = b->next)
		DPRINTF(("\t\tbase %#X,%#X, size %#X,%#X, next %#X\n",
				b->basehi, b->baselo, b->sizehi, b->sizelo, b->next));
}

void
pci_dump_addresses(Environ *e, Pci_addresses *addrs)
{
	DPRINTF(("Adddresses: %#X\n", addrs));
	DPRINTF(("\tbusaddrs = %#X\n", addrs->busaddrs));
	pci_dump_blocks(e, addrs->busaddrs);
	DPRINTF(("\tioaddrs = %#X\n", addrs->ioaddrs));
	pci_dump_blocks(e, addrs->ioaddrs);
	DPRINTF(("\tmemaddrs = %#X\n", addrs->memaddrs));
	pci_dump_blocks(e, addrs->memaddrs);
	DPRINTF(("\tprefetchaddrs = %#X\n", addrs->prefetchaddrs));
	pci_dump_blocks(e, addrs->prefetchaddrs);
}

void
pci_print_blocks(Environ *e, Pci_address_block *b)
{
	for (; b; b = b->next)
	{
		char sizestr[80];
		char addrstr[80];

		/*
		    XXXX	<= 9999
		    X.XK    <= 9.9*1024, 10137
		    XXXK	<= 999*1024, 1022976
		    X.XM	<= 9.9*1024*1024, 10380902
		    XXXM	<= 999*1024*1024, 1047527424
		    X.XG	<= 9.9*1024*1024*1024
		*/

		if (b->sizehi)
			bprintf(sizestr, "%X.%08X", b->sizehi, b->sizelo);
		else if (b->sizelo < 10000)
			bprintf(sizestr, "%u", b->sizelo);
		else if (b->sizelo <= (99*1024)/10)
			bprintf(sizestr, "%u.%uK", b->sizelo >> 10,
					((10 * b->sizelo) >> 10) % 10);
		else if (b->sizelo <= 999*1024)
			bprintf(sizestr, "%uK", b->sizelo >> 10);
		else if (b->sizelo <= (99*1024*1024)/10)
			bprintf(sizestr, "%u.%uM", b->sizelo >> 20,
					((10 * b->sizelo) >> 20) % 10);
		else if (b->sizelo <= 999*1024*1024)
			bprintf(sizestr, "%uM", b->sizelo >> 20);
		else
			bprintf(sizestr, "%u.%uG", b->sizelo >> 30,
					((10 * (b->sizelo >> 10)) >> 20) % 10);

		if (b->basehi)
			bprintf(addrstr, "%X.%08X", b->basehi, b->baselo);
		else if (b->baselo < 1000)
			bprintf(addrstr, "%u", b->baselo);
		else
			bprintf(addrstr, "%X", b->baselo);

		cprintf(e, "    %10s@%s\n", sizestr, addrstr);
	}
}

void
pci_print_addresses(Environ *e, Pci_addresses *addrs)
{
	cprintf(e, "Bus addresses:\n");
	pci_print_blocks(e, addrs->busaddrs);
	cprintf(e, "I/O addresses:\n");
	pci_print_blocks(e, addrs->ioaddrs);
	cprintf(e, "Memory addresses:\n");
	pci_print_blocks(e, addrs->memaddrs);
	cprintf(e, "Prefetchable memory addresses:\n");
	pci_print_blocks(e, addrs->prefetchaddrs);
}

static Retcode
pci_assign_address_space(Environ *e, Package *cpkg, Int hbridge, Int bus,
		Pci_addresses *addrs)
{
	Entry *regprop;
	Byte *regp;
	Int reglen;
	Retcode err;
	Byte *props = prop_alloc(e, 0);
	Int plen = 0;

	DPRINTF(("pci_assign_address_space: e %#X, cpkg %#X, hbridge %#X, bus %#X, addrs %#X\n",
			e, cpkg, hbridge, bus, addrs));

	if (addrs == NULL)
	{
		DPRINTF(("pci_assign_address_space: addresses dumped\n"));
		return NO_ERROR;
	}

	pci_dump_addresses(e, addrs);
	DPRINTF(("pci_assign_address_space: addresses dumped\n"));

	/* examine the reg property and assign address space to the device */
	/* resources. */

	/* allow multiple calls to let builtin-drivers init themselves */
	if (find_table(cpkg->props, "assigned-addresses", CSTR))
	{
		DPRINTF(("pci_assign_address_space: assigned-addresses already built\n"));
		return NO_ERROR;
	}

	/* get the reg property */
	regprop = find_table(cpkg->props, "reg", CSTR);

	if (regprop == NULL)
	{
		add_property(cpkg->props, "assigned-addresses", CSTR, NULL, 0);
		DPRINTF(("pci_assign_address_space: no reg property\n"));
		return NO_ERROR;
	}

	regp = (Byte *)regprop->v.array;
	reglen = regprop->len;

	while (reglen)
	{
		Int physhi;
		Int physmid;
		Int physlo;
		Int sizehi;
		Int sizelo;
		uInt rphyshi;
		uInt rsizehi;
		uInt rsizelo;
		Int dev;
		Int func;
		int reg;

		Pci_device_info devinfo;

		err = pci_decode_reg_prop(&regp, &reglen, &physhi, &physmid, &physlo,
				&sizehi, &sizelo);

		if (err != NO_ERROR)
		{
			DPRINTF(("pci_assign_address_space: error decoding reg property\n"));
			return err;
		}

		DPRINTF(("assign addr: reg, phys %#X,%#X,%#X, size %#X,%#X\n", physhi, physmid, physlo, sizehi, sizelo));
		DPRINTF(("assign addr: reg, n %d, space %d, bus %d, dev %d, func %d, reg %02X\n", PCI_PHYSHI_N(physhi), PCI_PHYSHI_SPACE(physhi), PCI_PHYSHI_BUS(physhi), PCI_PHYSHI_DEV(physhi), PCI_PHYSHI_FUNC(physhi), PCI_PHYSHI_REG(physhi)));

		if (PCI_PHYSHI_N(physhi))
			continue;

		if (PCI_PHYSHI_BUS(physhi) != bus)
			continue;

		dev = PCI_PHYSHI_DEV(physhi);
		func = PCI_PHYSHI_FUNC(physhi);
		reg = PCI_PHYSHI_REG(physhi);

		devinfo.hbridge = hbridge;
		devinfo.bus = bus;
		devinfo.dev = dev;
		devinfo.func = func;
		pci_get_device_info(e, &devinfo);

		err = (*devinfo.methods->getaddrinfo)(e, cpkg, &devinfo, reg, &rphyshi,
				&rsizehi, &rsizelo);

		DPRINTF(("assign addr: reg, rphys %#X, rsize %#X,%#X\n", rphyshi, rsizehi, rsizelo));

		if (err != NO_ERROR || rphyshi != physhi)
			continue;

		/* allocate space for the register resource */
		switch (PCI_PHYSHI_SPACE(physhi))
		{
		case PCI_SPACE_CONFIG:	/* config */
			continue;
		case PCI_SPACE_IO:	/* i/o */
			/* make sure it is really I/O */
			if (rsizelo > sizelo)
				sizelo = rsizelo;

			if (PCI_PHYSHI_T(physhi))
				physlo = pci_allocate_io10(addrs, sizelo);
			else
				physlo = pci_allocate_io32(addrs, sizelo);

			config_write(&devinfo, reg, physlo);

			if (physlo == 0)
				continue;

			physmid = 0;
			sizehi = 0;
			break;
		case PCI_SPACE_MEM:	/* mem 32bit addr */
			if (rsizelo > sizelo)
				sizelo = rsizelo;

			if (PCI_PHYSHI_T(physhi))
				physlo = pci_allocate_mem20(addrs, sizelo);
			else if (PCI_PHYSHI_P(physhi))
				physlo = pci_allocate_pmem32(addrs, sizelo);
			else
				physlo = pci_allocate_mem32(addrs, sizelo);

			config_write(&devinfo, reg, physlo);

			if (physlo == 0)
				continue;

			physmid = 0;
			sizehi = 0;
			break;
		case PCI_SPACE_XMEM:	/* mem 64bit addr */
			if (rsizehi > sizehi)
			{
				sizehi = rsizehi;
				sizelo = 0;
			}
			else if (rsizehi == sizehi && rsizelo > sizelo)
				sizelo = rsizelo;

			if (PCI_PHYSHI_P(physhi))
			{
				if (!pci_allocate_pmem64(addrs, sizehi, sizelo, (uInt*)&physmid,
						(uInt*)&physlo)) 
				{
					config_write(&devinfo, reg, 0);
					config_write(&devinfo, reg + 4, 0);
					continue;
				}
			}
			else
				if (!pci_allocate_mem64(addrs, sizehi, sizelo, (uInt*)&physmid,
						(uInt*)&physlo)) 
				{
					config_write(&devinfo, reg, 0);
					config_write(&devinfo, reg + 4, 0);
					continue;
				}

			config_write(&devinfo, reg, physlo);
			config_write(&devinfo, reg + 4, physmid);
			break;
		case PCI_SPACE_BUS:	/* bus */
			/* this isn't currenlty spec'd */
			continue;
		default:
			continue;
		}

		DPRINTF(("assign addr: assigned-addr, phys %#X,%#X,%#X, size %#X,%#X\n",
				physhi, physmid, physlo, sizehi, sizelo));

		/* add the space to the assigned-addresses property */
		(void)prop_alloc(e, 5 * sizeof (Int));
		pci_encode_reg_prop(props, &plen, PCI_PHYSHI_MK(1,
				PCI_PHYSHI_P(physhi), PCI_PHYSHI_T(physhi),
				PCI_PHYSHI_SPACE(physhi), bus, dev, func, reg), physmid, physlo,
				sizehi, sizelo);
	}

	/* set the assigned-addresses property */
	add_property(cpkg->props, "assigned-addresses", CSTR, props, plen);
	return NO_ERROR;
}

Pci_addresses *g_pci_addrs;

Retcode
pci_probe_bus(Environ *e, Package *buspkg, Int hbridge, Int bus,
	Pci_addresses *addrs, int devs)
{
	Int dev;
	Int func;
	Int maxfunc;
	Package *cpkg;
	Retcode err = NO_ERROR;
	Pci_device_info devinfo;
	Pci_addresses *g_pci_addrs_save;
	Package *savepkg = e->currpkg;
	Cell saveinst = e->currinst;
	Byte path[STR_SIZE];

	/* first open ourself to create new devices beneath the bus node */
	if (!get_device_name(e, buspkg, path))
		return E_BAD_DEVICE;

	IFCKSP(e, 0, 2);
	PUSHP(e, path);
	PUSH(e, strlen(path));
	err = execute_word(e, "open-dev");

	if (err != NO_ERROR)
		return err;

	POP(e, e->currinst);

	if (e->currinst == 0)
		return E_NULL_INSTANCE;

	e->currpkg = buspkg;
	g_pci_addrs_save = g_pci_addrs;
	g_pci_addrs = addrs;

	DPRINTF(("pci_probe_bus: e %#X, buspkg %#X, hbridge %#X, bus %d, addrs %#X, devs %d\n", e, buspkg, hbridge, bus, addrs, devs));

	/* probe for the devices */
	for (dev = 0; dev < devs; dev++)
	{
		Int id;
		int type;

		memset(&devinfo, 0, sizeof devinfo);
		devinfo.hbridge = hbridge;
		devinfo.bus = bus;
		devinfo.dev = dev;
		devinfo.func = 0;

		/* NOTE: the spec says we should use lpeek to read this register */
		/* I don't know how to make this work so we simply do a config */
		/* read and hope we don't bus fault. */
		id = config_read(&devinfo, PCI_CONFIG_ID);

/*		if (id == 0xFFFFFFFFU || id == 0x00000000)	*/
		if (id == 0xFFFFFFFFU)
			continue;

		type = (config_read(&devinfo, PCI_CONFIG_HEADER) >> 16) & 0xFF;

		if (type & 0x80)
			maxfunc = 8;
		else
			maxfunc = 1;

		for (func = 0; func < maxfunc; func++)
		{
			char *vendorname;
			char *partnumber;

			devinfo.func = func;
			id = config_read(&devinfo, PCI_CONFIG_ID);

/*			if (id == 0xFFFFFFFFU || id == 0x00000000)	*/
			if (id == 0xFFFFFFFFU)
				continue;

			/* we have found a device.  create a node for it */
			/* and initialize the base properties. */
			devinfo.vendid = id & 0xFFFF;
			devinfo.devid = (id >> 16) & 0xFFFF;
			vendorname = vendname(devinfo.vendid, 1);
			partnumber = partnum(devinfo.vendid, devinfo.devid, 1);
			DPRINTF(("=============== BEGIN ===============\n"));
			DPRINTF(("Device %X,%X (%s%s%s) found @%d,%d,%d\n", id & 0xFFFF, 
					id >> 16, vendorname ? vendorname : "",
					partnumber ? " " : "", partnumber ? partnumber : "",
					bus, dev, func));

			pci_get_device_info(e, &devinfo);
			type = devinfo.header_type;
			DPRINTF(("pci: device @%d,%d,%d id %#X, type %d\n", bus, dev, func,
					id, type));

			cpkg = new_package(buspkg);
			cpkg->initinst->probe[0] = 0;	/* phys.lo */
			cpkg->initinst->probe[1] = 0;	/* phys.mid */
			cpkg->initinst->probe[2] = PCI_PHYSHI_MK(0, 0, 0, PCI_SPACE_CONFIG,
					devinfo.bus, devinfo.dev, devinfo.func, 0);	/* phys.hi */
			e->currpkg = cpkg;
			cpkg->initinst->parent = (Instance*)(uPtr)e->currinst;
			e->currinst = (uPtr)cpkg->initinst;

			pci_load_standard_properties(e, cpkg, &devinfo);

			/* probe the device */
			if (devinfo.methods)
			{
				DPRINTF(("pci: calling devinfo.methods->probe %#x...\n",
						*devinfo.methods->probe));
				err = (*devinfo.methods->probe)(e, cpkg, &devinfo, addrs);
			}
			else
			{
				/* attempt to load a builtin device driver */
				DPRINTF(("pci: loading builtin device driver...\n"));
				err = pci_load_builtin_driver(e, cpkg, &devinfo, addrs);
			}

			e->currinst = (uPtr)cpkg->initinst->parent;
			e->currpkg = e->currpkg->parent;
			cpkg->initinst->parent = NULL;
			DPRINTF(("--------------- DONE ---------------\n"));

			if (err != NO_ERROR && err != E_NO_DRIVER)
				break;
		}
	}

	/* assign address space to the probed devices */
	for (cpkg = buspkg->children; cpkg; cpkg = cpkg->link)
	{
#ifdef ISA_HACK
		/* do not assign addrs to ISA devices */
		Int classcode;
/*		cprintf(e, "buspkg %#x, cpkg %#x, assigning address space\n", buspkg,
				cpkg);	*/
		err = prop_get_int(cpkg->props, "class-code", CSTR, &classcode);

		if (err != NO_ERROR ||
				((classcode & 0xFFFF0000) != 0x30000 &&		/* display */
				(classcode & 0xFFFFFFFF) != 0x000100 &&		/* display */
				(classcode & 0xFFFFFF00) != 0x010100))		/* IDE */
			err = pci_assign_address_space(e, cpkg, hbridge, bus, addrs);
		else
			err = NO_ERROR;
#else
		err = pci_assign_address_space(e, cpkg, hbridge, bus, addrs);
#endif

		if (err != NO_ERROR)
			break;
	}

	/* close up our parent chain */
	PUSH(e, e->currinst);
	(void)execute_word(e, "close-dev");
	e->currinst = saveinst;
	e->currpkg = savepkg;

#ifdef DEBUG
	DPRINTF(("Done with pci-probe\n"));
	pci_print_addresses(e, addrs);
#endif
	g_pci_addrs = g_pci_addrs_save;
	return err;
}

static void
pci_format_csr_add(char **buf, int *len, char *str)
{
	char *bp = *buf;
	int l = *len;
	int ch;

	while ((ch = *str++) != '\0')
	{
		if (l < 2)
			break;

		*bp++ = ch;
		l--;
	}

	if (l >= 1)
		*bp = '\0';

	*len = l;
	*buf = bp;
}

void
pci_format_csr(char *buf, int buflen, uInt csr)
{
	uInt cmd = csr & 0xFFFF;
	uInt stat = (csr >> 16) & 0xFFFF;

	pci_format_csr_add(&buf, &buflen, "[");

	if (stat & PCI_STATUS_PARITYDETECT)
		pci_format_csr_add(&buf, &buflen, "PAR ");

	if (stat & PCI_STATUS_SIGSYSTEMERR)
		pci_format_csr_add(&buf, &buflen, "SERR ");

	if (stat & PCI_STATUS_RECVMASTERABORT)
		pci_format_csr_add(&buf, &buflen, "MAST ");

	if (stat & PCI_STATUS_RECVTARGETABORT)
		pci_format_csr_add(&buf, &buflen, "TARG ");

	if (stat & PCI_STATUS_SIGTARGETABORT)
		pci_format_csr_add(&buf, &buflen, "SIGTARG ");

	switch (stat & PCI_STATUS_DEVSELTIMING)
	{
	case 0x0000:
		pci_format_csr_add(&buf, &buflen, "FAST ");
		break;
	case 0x0200:
		pci_format_csr_add(&buf, &buflen, "MED ");
		break;
	case 0x0400:
		pci_format_csr_add(&buf, &buflen, "SLOW ");
		break;
	case 0x0600:
		pci_format_csr_add(&buf, &buflen, "DEVSEL3 ");
		break;
	}

	if (stat & PCI_STATUS_DATAPARITY)
		pci_format_csr_add(&buf, &buflen, "MPERR ");

	if (stat & PCI_STATUS_FASTBACK2BACKXFER)
		pci_format_csr_add(&buf, &buflen, "FB2B ");

	if (stat & PCI_STATUS_UDF)
		pci_format_csr_add(&buf, &buflen, "UDF ");

	if (stat & PCI_STATUS_66MHZ)
		pci_format_csr_add(&buf, &buflen, "M66 ");

	if (stat & PCI_STATUS_CAPABILITIESLIST)
		pci_format_csr_add(&buf, &buflen, "CAP ");

	if (cmd & PCI_COMMAND_FASTBACK2BACKENABLE)
		pci_format_csr_add(&buf, &buflen, "FB2BE ");

	if (cmd & PCI_COMMAND_SYSTEMERRENABLE)
		pci_format_csr_add(&buf, &buflen, "SERRE ");

	if (cmd & PCI_COMMAND_WAITENABLE)
		pci_format_csr_add(&buf, &buflen, "WAITE ");

	if (cmd & PCI_COMMAND_PARITYERRENABLE)
		pci_format_csr_add(&buf, &buflen, "PERRE ");

	if (cmd & PCI_COMMAND_PALLETESNOOPENABLE)
		pci_format_csr_add(&buf, &buflen, "VGAE ");

	if (cmd & PCI_COMMAND_MWIENABLE)
		pci_format_csr_add(&buf, &buflen, "MWIE ");

	if (cmd & PCI_COMMAND_SPECCYCLEENABLE)
		pci_format_csr_add(&buf, &buflen, "SPECE ");

	if (cmd & PCI_COMMAND_MASTERENABLE)
		pci_format_csr_add(&buf, &buflen, "MASTE ");

	if (cmd & PCI_COMMAND_MEMENABLE)
		pci_format_csr_add(&buf, &buflen, "MEME ");

	if (cmd & PCI_COMMAND_IOENABLE)
		pci_format_csr_add(&buf, &buflen, "IOE ");

	if (buf[-1] == ' ')
	{
		buf--;
		buflen++;
	}

	pci_format_csr_add(&buf, &buflen, "]");
}

Retcode
pci_dump_config(Environ *e, Package *pkg, Pci_device_info *devinfo)
{
	Int reg;
	char buf[256];

	pci_get_device_info(e, devinfo);
	cprintf(e, "Package %#X\n", pkg);
	cprintf(e, "\thost bridge = %d\n", devinfo->hbridge);
	cprintf(e, "\tbus = %d\n", devinfo->bus);
	cprintf(e, "\tdevice = %d\n", devinfo->dev);
	cprintf(e, "\tfunction = %d\n", devinfo->func);
	cprintf(e, "\theader type = %d\n", devinfo->header_type);
	cprintf(e, "\tclass code = %06X\n", devinfo->classcode);
	cprintf(e, "\trevision id = %02X\n", devinfo->revisionid);
	cprintf(e, "\tvendor id = %04X\n", devinfo->vendid);
	cprintf(e, "\tdevice id = %04X\n", devinfo->devid);
	cprintf(e, "\tsubsystem vendor id = %04X\n", devinfo->subsysvendid);
	cprintf(e, "\tsubsystem device id = %04X\n", devinfo->subsysdevid);

	reg = config_read(devinfo, PCI_CONFIG_COMMAND);
	pci_format_csr(buf, 256, reg);
	cprintf(e, "\tcommand = %04X\n", reg & 0xFFFF);
	cprintf(e, "\tstatus = %04X %s\n", (reg >> 16) & 0xFFFF, buf);
	reg = config_read(devinfo, PCI_CONFIG_HEADER);
	cprintf(e, "\tlatency = %02X\n", (reg >> 8) & 0xFF);
	cprintf(e, "\tBIST = %02X\n", (reg >> 16) & 0xFF);
	cprintf(e, "\tcache = %02X\n", (reg >> 24) & 0xFF);

	switch (devinfo->header_type)
	{
	case 0:	/* PCI device */
		cprintf(e, "\tBAR 0 = %08X\n", config_read(devinfo, PCI_CONFIG_BAR0));
		cprintf(e, "\tBAR 1 = %08X\n", config_read(devinfo, PCI_CONFIG_BAR1));
		cprintf(e, "\tBAR 2 = %08X\n", config_read(devinfo, PCI_CONFIG_BAR2));
		cprintf(e, "\tBAR 3 = %08X\n", config_read(devinfo, PCI_CONFIG_BAR3));
		cprintf(e, "\tBAR 4 = %08X\n", config_read(devinfo, PCI_CONFIG_BAR4));
		cprintf(e, "\tBAR 5 = %08X\n", config_read(devinfo, PCI_CONFIG_BAR5));
		cprintf(e, "\tCIS = %08X\n", config_read(devinfo, PCI_CONFIG_CARDBUS));
		cprintf(e, "\tboot ROM = %08X\n",
				config_read(devinfo, PCI_CONFIG_ROMADDR));
		cprintf(e, "\treserved @34 = %08X\n", 
				config_read(devinfo, PCI_CONFIG_RESERVED_0));
		cprintf(e, "\treserved @38 = %08X\n", 
				config_read(devinfo, PCI_CONFIG_RESERVED_1));
		cprintf(e, "\tinterrupt = %08X\n", 
				config_read(devinfo, PCI_CONFIG_INTERRUPT));
		break;
	case 1:	/* PCI-PCI bridge */
		cprintf(e, "\tBAR 0 = %08X\n", config_read(devinfo, PPB_CONFIG_BAR0));
		cprintf(e, "\tBAR 1 = %08X\n", config_read(devinfo, PPB_CONFIG_BAR1));
		reg = config_read(devinfo, PPB_CONFIG_BUSNUM);
		cprintf(e, "\tprimary bus = %02X\n", reg & 0xFF);
		cprintf(e, "\tsecondary bus = %02X\n", (reg >> 8) & 0xFF);
		cprintf(e, "\tsubordinate bus = %02X\n", (reg >> 16) & 0xFF);
		cprintf(e, "\tsecondary latency = %02X\n", (reg >> 24) & 0xFF);
		reg = config_read(devinfo, PPB_CONFIG_IOBASE);
		cprintf(e, "\ti/o base = %02X\n", reg & 0xFF);
		cprintf(e, "\ti/o limit = %02X\n", (reg >> 8) & 0xFF);
		cprintf(e, "\tsecondary status = %04X\n", (reg >> 16) & 0xFFFF);
		reg = config_read(devinfo, PPB_CONFIG_MEMBASE);
		cprintf(e, "\tmemory base = %04X\n", reg & 0xFFFF);
		cprintf(e, "\tmemory limit = %04X\n", (reg >> 16) & 0xFFFF);
		reg = config_read(devinfo, PPB_CONFIG_PMEMBASE);
		cprintf(e, "\tprefetch base = %04X\n", reg & 0xFFFF);
		cprintf(e, "\tprefetch limit = %04X\n", (reg >> 16) & 0xFFFF);
		cprintf(e, "\tprefetch64 base = %08X\n",
				config_read(devinfo, PPB_CONFIG_PMEMBASEH));
		cprintf(e, "\tprefetch64 limit = %08X\n",
				config_read(devinfo, PPB_CONFIG_PMEMLIMITH));
		reg = config_read(devinfo, PPB_CONFIG_IOBASEH);
		cprintf(e, "\ti/o 32 base = %04X\n", reg & 0xFFFF);
		cprintf(e, "\ti/o 32 limit = %04X\n", (reg >> 16) & 0xFFFF);
		cprintf(e, "\tboot ROM = %08X\n",
				config_read(devinfo, PPB_CONFIG_ROMADDR));
		reg = config_read(devinfo, PPB_CONFIG_INTERRUPT);
		cprintf(e, "\tinterrupt line = %02X\n", reg & 0xFF);
		cprintf(e, "\tinterrupt pin = %02X\n", (reg >> 8) & 0xFF);
		cprintf(e, "\tbridge control = %04X\n", (reg >> 16) & 0xFFFF);
		break;
	case 2:	/* PCI-CardBus bridge */
		cprintf(e, "\tBAR 0 = %08X\n",
				config_read(devinfo, CARDBUS_CONFIG_BAR0));
		cprintf(e, "\tBAR 16-bit I/O = %08X\n",
				config_read(devinfo, CARDBUS_CONFIG_LEGACYBASE));
		reg = config_read(devinfo, CARDBUS_CONFIG_BUSNUM);
		cprintf(e, "\tprimary bus = %02X\n", reg & 0xFF);
		cprintf(e, "\tsecondary bus = %02X\n", (reg >> 8) & 0xFF);
		cprintf(e, "\tsubordinate bus = %02X\n", (reg >> 16) & 0xFF);
		cprintf(e, "\tsecondary latency = %02X\n", (reg >> 24) & 0xFF);
		reg = config_read(devinfo, CARDBUS_CONFIG_SECSTATUS);
		cprintf(e, "\tsecondary status = %04X\n", (reg >> 16) & 0xFFFF);
		cprintf(e, "\tmem base 0 = %08X\n",
				config_read(devinfo, CARDBUS_CONFIG_MEMBASE0));
		cprintf(e, "\tmem limit 0 = %08X\n",
				config_read(devinfo, CARDBUS_CONFIG_MEMLIMIT0));
		cprintf(e, "\tmem base 1 = %08X\n",
				config_read(devinfo, CARDBUS_CONFIG_MEMBASE1));
		cprintf(e, "\tmem limit 1 = %08X\n",
				config_read(devinfo, CARDBUS_CONFIG_MEMLIMIT1));
		cprintf(e, "\tio base 0 = %08X\n",
				config_read(devinfo, CARDBUS_CONFIG_IOBASE0));
		cprintf(e, "\tio limit 0 = %08X\n",
				config_read(devinfo, CARDBUS_CONFIG_IOLIMIT0));
		cprintf(e, "\tio base 1 = %08X\n",
				config_read(devinfo, CARDBUS_CONFIG_IOBASE1));
		cprintf(e, "\tio limit 1 = %08X\n",
				config_read(devinfo, CARDBUS_CONFIG_IOLIMIT1));
		reg = config_read(devinfo, CARDBUS_CONFIG_INTERRUPT);
		cprintf(e, "\tinterrupt line = %02X\n", reg & 0xFF);
		cprintf(e, "\tinterrupt pin = %02X\n", (reg >> 8) & 0xFF);
		cprintf(e, "\tbridge control = %04X\n", (reg >> 16) & 0xFFFF);
		break;
	}

	return NO_ERROR;
}

static Int
pci_get_busnumber(Environ *e, Package *pkg)
{
	Entry *regprop = find_table(pkg->props, "reg", CSTR);
	int bus = 0;

	if (regprop != NULL)
	{
		Byte *regp = (Byte *)regprop->v.array;
		Int reglen = regprop->len;
		Int physhi;

		if (pci_decode_reg_prop(&regp, &reglen, &physhi, NULL, NULL, NULL,
				NULL) == NO_ERROR)
			bus = PCI_PHYSHI_BUS(physhi);
	}

	DPRINTF(("pci_get_busnumber: bus=%d\n", bus));
	return bus;
}

C(f_pci_assign_package_address)	/* assign-package-addresses (phandle --) */
{
	Package *pkg;
	Int hbridge;
	Int bus;
	IFCKSP(e, 1, 0);
	POPT(e, pkg, Package *);
	hbridge = pci_find_host_bridge_number(e, pkg);
	bus = pci_get_busnumber(e, pkg);
	return pci_assign_address_space(e, pkg, hbridge, bus, g_pci_addrs);
}

CC(f_probe_pci)				/* probe-pci (--) */
{
	Pci_addresses addrs;
	Int hbridge;
	Int max = pci_num_host_bridges();
	Retcode err;
	Package *pkg;
	Byte *props;
	Int plen;

	for (hbridge = 0; hbridge < max; hbridge++)
	{
		err = pci_init_addresses(hbridge, &addrs);
		g_pci_addrs = &addrs;

		if (err != NO_ERROR)
			return err;

		err = pci_bus_package(e, hbridge, &pkg);

		if (err != NO_ERROR)
			return err;
		
		if (pkg == NULL)
			return E_NULL_PACKAGE;

		/* add a pci-bridge-number property so we can figure out hbridge */
		/* for a particular node. */
		if (max > 1)
			prop_set_int(pkg->props, "pci-bridge-number", CSTR, hbridge);

		/* create bus-range with all possible bus numbers for the moment */
		plen = 0;
		props = prop_alloc(e, 2 * sizeof (Int));

		if (props == NULL)
			return E_OUT_OF_MEMORY;

		prop_encode_int(props + plen, &plen, 0);
		prop_encode_int(props + plen, &plen, addrs.busaddrs->sizelo);
		err = add_property(pkg->props, "bus-range", CSTR, props, plen);

		if (err != NO_ERROR)
			return err;

		err = pci_probe_bus(e, pkg, hbridge, 0, &addrs, 32);

		if (err != NO_ERROR)
			return err;

		/* now we can update 2nd bus-range number here */
		plen = sizeof (Int);
		prop_encode_int(props + sizeof (Int), &plen,
				addrs.busaddrs->baselo - 1);
	}

	return NO_ERROR;
}

C(f_make_properties)			/* make-properties (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Cell unit;
	Pci_device_info devinfo;

	if (inst == NULL || inst->numunits < 1)
		return E_NO_DEVICE;

	unit = inst->unit[inst->numunits - 1];
	devinfo.hbridge = pci_find_host_bridge_number(e, pkg);
	devinfo.bus = PCI_PHYSHI_BUS(unit);
	devinfo.dev = PCI_PHYSHI_DEV(unit);
	devinfo.func = PCI_PHYSHI_FUNC(unit);
	pci_get_device_info(e, &devinfo);
	pci_load_standard_properties(e, pkg, &devinfo);
	return NO_ERROR;
}

C(f_assign_addresses)			/* assign-addresses (--) */
{
	Int hbridge = pci_find_host_bridge_number(e, e->currpkg);
	Int bus = pci_get_busnumber(e, e->currpkg);
	return pci_assign_address_space(e, e->currpkg, hbridge, bus, g_pci_addrs);
}

static void
show_pci_mem(Environ *e, char *leadin, Pci_device_info *devinfo)
{
	int type = devinfo->header_type;
	int bar;
	int numbars = 0;
	uInt cfg;
	uInt base;
	uInt basehi;
	uInt size;
	uInt sizehi;
	uInt limit;
	uInt limithi;

	switch (type)
	{
	case 0:
		numbars = 6;
		break;
	case 1:
		numbars = 2;
		break;
	case 2:
		numbars = 1;
		break;
	}

	for (bar = 0; bar < numbars; bar++)
	{
		uInt bits;
		uInt bitshi;

		cfg = config_read(devinfo, PCI_CONFIG_COMMAND);
		config_write(devinfo, PCI_CONFIG_COMMAND, cfg & 0xFFF8);
		base = config_read(devinfo, PCI_CONFIG_BAR0 + bar * 4);
		config_write(devinfo, PCI_CONFIG_BAR0 + bar * 4, 0xFFFFFFFF);
		bits = config_read(devinfo, PCI_CONFIG_BAR0 + bar * 4);
		config_write(devinfo, PCI_CONFIG_BAR0 + bar * 4, base);
		config_write(devinfo, PCI_CONFIG_COMMAND, cfg & 0xFFFF);

		if (bits & 1)
		{
			/* i/o space */
			size = (~bits | 3) + 1;
			size &= -size;		/* clear any un-programable upper bits */

			if (!size)
				continue;

			limit = (base & ~3) + size - 1;
			cprintf(e, "%s                   %04X.%04X-          %04X.%04X BAR %d I/O\n",
					leadin, (base >> 16) & 0xFFFF, base & 0xFFFF,
					(limit >> 16) & 0xFFFF, limit & 0xFFFF, bar);
		}
		else
		{
			/* memory space */
			size = (~bits | 0xF) + 1;
			size &= -size;		/* clear any un-programable upper bits */

			if ((bits & 6) == 4)
			{
				config_write(devinfo, PCI_CONFIG_COMMAND, cfg & 0xFFF8);
				basehi = config_read(devinfo, PCI_CONFIG_BAR0 + (bar * 4) + 4);
				config_write(devinfo, PCI_CONFIG_BAR0 + (bar * 4)  + 4,
						0xFFFFFFFF);
				bitshi = config_read(devinfo, PCI_CONFIG_BAR0 + (bar * 4) + 4);
				config_write(devinfo,
						PCI_CONFIG_BAR0 + (bar * 4)  + 4, basehi);
				config_write(devinfo, PCI_CONFIG_COMMAND, cfg & 0xFFFF);
				sizehi = ~bitshi + 1;
				sizehi = sizehi & -sizehi;
				bar++;

				if (!size && !sizehi)
					continue;

				limit = (base & ~0xF) + size;
				limithi = basehi + sizehi + (limit < base);

				if (!limit)
					limithi--;

				limit--;

				cprintf(e, "%s         %04X.%04X.%04X.%04X-%04X.%04X.%04X.%04X BAR %d %sMEM\n",
						leadin, (basehi >> 16) & 0xFFFF, basehi & 0xFFFF,
						(base >> 16) & 0xFFFF, base & 0xFFFF,
						(limithi >> 16) & 0xFFFF, limithi & 0xFFFF,
						(limit >> 16) & 0xFFFF, limit & 0xFFFF,
						bar, (type & 8) ? "P" : "");
			}
			else
			{
				if (!size)
					continue;

				limit = (base & ~0xF) + size - 1;

				cprintf(e, "%s                   %04X.%04X-          %04X.%04X BAR %d %sMEM\n",
						leadin, (base >> 16) & 0xFFFF, base & 0xFFFF,
						(limit >> 16) & 0xFFFF, limit & 0xFFFF,
						bar, (type & 8) ? "P" : "");
			}
		}
	}

	switch (type)
	{
	case 0:
		/* show BIOS BAR */
		cfg = config_read(devinfo, PCI_CONFIG_COMMAND);
		config_write(devinfo, PCI_CONFIG_COMMAND, cfg & 0xFFF8);
		base = config_read(devinfo, PCI_CONFIG_ROMADDR);
		config_write(devinfo, PCI_CONFIG_ROMADDR, 0xFFFFFFFF);
		size = config_read(devinfo, PCI_CONFIG_ROMADDR);
		config_write(devinfo, PCI_CONFIG_ROMADDR, base);
		config_write(devinfo, PCI_CONFIG_COMMAND, cfg & 0xFFFF);
		size = (~size | 0xFFF) + 1;
		size &= -size;

		if (size)
		{
			limit = (base & ~0xFFF) + size - 1;
			cprintf(e, "%s                   %04X.%04X-          %04X.%04X BIOS  MEM\n",
					leadin, (base >> 16) & 0xFFFF, base & 0xFFFF,
					(limit >> 16) & 0xFFFF, limit & 0xFFFF);
		}

		break;
	case 1:
		/* show BIOS BAR */
		cfg = config_read(devinfo, PPB_CONFIG_COMMAND);
		config_write(devinfo, PPB_CONFIG_COMMAND, cfg & 0xFFF8);
		base = config_read(devinfo, PPB_CONFIG_ROMADDR);
		config_write(devinfo, PPB_CONFIG_ROMADDR, 0xFFFFFFFF);
		size = config_read(devinfo, PPB_CONFIG_ROMADDR);
		config_write(devinfo, PPB_CONFIG_ROMADDR, base);
		config_write(devinfo, PPB_CONFIG_COMMAND, cfg & 0xFFFF);
		size = (~size | 0xFFF) + 1;
		size &= -size;

		if (size)
		{
			limit = (base & ~0xFFF) + size - 1;
			cprintf(e, "%s                   %04X.%04X-          %04X.%04X BIOS  MEM\n",
					leadin, (base >> 16) & 0xFFFF, base & 0xFFFF,
					(limit >> 16) & 0xFFFF, limit & 0xFFFF);
		}

		/* show IO */
		base = config_read(devinfo, PPB_CONFIG_IOBASE);
		basehi = config_read(devinfo, PPB_CONFIG_IOBASEH);
		cprintf(e, "%s                   %04X.%01X000-          %04X.%01XFFF IO\n",
				leadin, basehi & 0xFFFF, (base >> 4) & 0xF,
				(basehi >> 16) & 0xFFFF, (base >> 12) & 0xF);

		/* show MEM */
		base = config_read(devinfo, PPB_CONFIG_MEMBASE);
		cprintf(e, "%s                   %04X.0000-          %04X.FFFF MEM\n",
				leadin, base & 0xFFFF, (base >> 16) & 0xFFFF);

		/* show PMEM */
		base = config_read(devinfo, PPB_CONFIG_PMEMBASE);
		basehi = config_read(devinfo, PPB_CONFIG_PMEMBASEH);
		limithi = config_read(devinfo, PPB_CONFIG_PMEMLIMITH);

		if (basehi || limithi)
			cprintf(e, "%s         %04X.%04X.%04X.0000-%04X.%04X.%04X.FFFF PMEM\n",
					leadin,
					(basehi >> 16) & 0xFFFF, basehi & 0xFFFF, base & 0xFFFF,
					(limithi >> 16) & 0xFFFF, limithi & 0xFFFF,
					(base >> 16) & 0xFFFF);
		else
			cprintf(e, "%s                   %04X.0000-          %04X.FFFF PMEM\n",
					leadin, base & 0xFFFF, (base >> 16) & 0xFFFF);

		break;
	case 2:
		/* show IO 0 */
		base = config_read(devinfo, CARDBUS_CONFIG_IOBASE0);
		limit = config_read(devinfo, CARDBUS_CONFIG_IOLIMIT0);
		cprintf(e, "%s                   %04X.%04X-          %04X.%04X IO 0\n",
				leadin, (base >> 16) & 0xFFFF, base & 0xFFFF,
				(limit >> 16) & 0xFFFF, limit & 0xFFFF);
		/* show IO 1 */
		base = config_read(devinfo, CARDBUS_CONFIG_IOBASE1);
		limit = config_read(devinfo, CARDBUS_CONFIG_IOLIMIT1);
		cprintf(e, "%s                   %04X.%04X-          %04X.%04X IO 1\n",
				leadin, (base >> 16) & 0xFFFF, base & 0xFFFF,
				(limit >> 16) & 0xFFFF, limit & 0xFFFF);
		/* show MEM 0 */
		base = config_read(devinfo, CARDBUS_CONFIG_MEMBASE0);
		limit = config_read(devinfo, CARDBUS_CONFIG_MEMLIMIT0);
		cprintf(e, "%s                   %04X.%04X-          %04X.%04X MEM 0\n",
				leadin, (base >> 16) & 0xFFFF, base & 0xFFFF,
				(limit >> 16) & 0xFFFF, limit & 0xFFFF);
		/* show MEM 1 */
		base = config_read(devinfo, CARDBUS_CONFIG_MEMBASE1);
		limit = config_read(devinfo, CARDBUS_CONFIG_MEMLIMIT1);
		cprintf(e, "%s                   %04X.%04X-          %04X.%04X MEM 1\n",
				leadin, (base >> 16) & 0xFFFF, base & 0xFFFF,
				(limit >> 16) & 0xFFFF, limit & 0xFFFF);
		break;
	}
}

static Retcode
show_pci(Environ *e, int full)
{
	int numbridges = pci_num_host_bridges();
	int numbuses = 1;
	int hbridge;
	int bus;
	int header = 0;

	for (hbridge = 0; hbridge < numbridges; hbridge++)
	{
		for (bus = 0; bus < numbuses; bus++)
		{
			Int dev;
			Int func;
			Int maxfunc;
			Pci_device_info devinfo;

			devinfo.hbridge = hbridge;
			devinfo.bus = bus;

			/* probe for the devices */
			for (dev = 0; dev < 32; dev++)
			{
				Int id;
				int type;

				devinfo.dev = dev;
				devinfo.func = 0;

				/* NOTE: the spec says we should use lpeek to read this register */
				/* I don't know how to make this work so we simply do a config */
				/* read and hope we don't bus fault. */
				id = config_read(&devinfo, PCI_CONFIG_ID);

/*				if (id == 0xFFFFFFFFU || id == 0x00000000)	*/
				if (id == 0xFFFFFFFFU)
					continue;

				type = (config_read(&devinfo, PCI_CONFIG_HEADER) >> 16) & 0xFF;

				if (type & 0x80)
					maxfunc = 8;
				else
					maxfunc = 1;

				for (func = 0; func < maxfunc; func++)
				{
					devinfo.func = func;
					id = config_read(&devinfo, PCI_CONFIG_ID);

/*					if (id == 0xFFFFFFFFU || id == 0x00000000)	*/
					if (id == 0xFFFFFFFFU)
						continue;

					devinfo.vendid = id & 0xFFFF;
					devinfo.devid = (id >> 16) & 0xFFFF;
					pci_get_device_info(e, &devinfo);
					type = devinfo.header_type;

					if (type)
						numbuses++;

					if (!header)
					{
						if (numbridges > 1)
							cprintf(e, "Br ");

						cprintf(e, "Bus Dv F Vend Dev  Vendor         "
								"Chip         Description\n");

						cprintf(e, "========================================"
								"=======================================\n");
						header = 1;
					}

					if (numbridges > 1)
						cprintf(e, "%2d ", hbridge);

					cprintf(e, "%3d %2d %1d %04X %04X ", bus, dev, func,
							devinfo.vendid, devinfo.devid);
					cprintf(e, "%-14s %-12s %s\n", vendname(devinfo.vendid, 0),
							partnum(devinfo.vendid, devinfo.devid, 0),
							partdesc(devinfo.vendid, devinfo.devid, 0));

					if (full)
					{
						int irq;
						show_pci_mem(e, numbridges > 1 ?  "   " : "", &devinfo);
						irq = config_read(&devinfo, PCI_CONFIG_INTERRUPT) & 0xFF;
						cprintf(e, "%s                 %02X IRQ\n",
								numbridges > 1 ?  "   " : "", irq);
					}

				}
			}
		}
	}

	return NO_ERROR;
}

C(f_show_pci)			/* show-pci (--) */
{
	return show_pci(e, 0);
}

C(f_show_pci_full)			/* show-pci-full (--) */
{
	return show_pci(e, 1);
}

C(f_dump_pciconfig)
{
	Package *pkg = e->currpkg;
	Pci_device_info devinfo;
	uInt unit;
	Entry *regprop;
	Byte *regp;
	Int reglen;
	Retcode err;

	/* get the reg property */
	regprop = find_table(pkg->props, "reg", CSTR);

	if (regprop == NULL)
	{
		cprintf(e, "Can't find reg property\n");
		return NO_ERROR;
	}

	regp = (Byte *)regprop->v.array;
	reglen = regprop->len;

	while (1)
	{
		Int physhi;

		err = pci_decode_reg_prop(&regp, &reglen, &physhi, NULL, NULL, NULL,
				NULL);

		if (err != NO_ERROR)
			return err;

		if (PCI_PHYSHI_SPACE(physhi) == PCI_SPACE_CONFIG)
		{
			unit = physhi;
			break;
		}

		if (!reglen)
		{
			cprintf(e, "Can't find config space in reg property\n");
			return NO_ERROR;
		}
	}

	devinfo.hbridge = pci_find_host_bridge_number(e, pkg);
	devinfo.bus = PCI_PHYSHI_BUS(unit);
	devinfo.dev = PCI_PHYSHI_DEV(unit);
	devinfo.func = PCI_PHYSHI_FUNC(unit);
	return pci_dump_config(e, pkg, &devinfo);
}

Initentry pci_methods[] =
{
	{ "open", f_pci_open, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- okay?) PCI bus open method") },
	{ "close", f_pci_close, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) PCI bus close method") },

	{ "map-in", f_pci_map_in, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(phys.lo phys.mid phys.hi size -- virt)\n"
					"\tmap PCI address to virtual address") },
	{ "map-out", f_pci_map_out, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(virt size --) done with virtual-to-PCI mapping") },

	{ "dma-alloc", f_pci_dma_alloc, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(size -- virt) allocate PCI DMA memory") },
	{ "dma-free", f_pci_dma_free, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(virt size --) free PCI DMA memory") },
	{ "dma-map-in", f_pci_dma_map_in, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(virt size cacheable? -- devaddr)\n"
					"\tmap virtual address suitable for PCI device access") },
	{ "dma-map-out", f_pci_dma_map_out, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(virt devaddr size --) free PCI DMA dev map") },
	{ "dma-sync", f_pci_dma_sync, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(virt devaddr size --)\n"
					"sync virtual memory to physical for PCI device/DMA") },

	{ "probe-self", f_pci_probe_self, INVALID_FCODE, F_NONE, T_FUNC
	HELP("(arg-str arg-len reg-str reg-len fcode-str fcode-len --)"
			"\tprobe PCI bus") },

	{ "decode-unit", f_pci_decode_unit, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(str len -- phys.lo phys.mid phys.hi)\n"
					"\tconvert string to PCI bus physical address") },
	{ "encode-unit", f_pci_encode_unit, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(phys.lo phys.mid phys.hi -- str len)\n"
					"\tconvert PCI bus physical address to string") },

	{ "config-l@", f_pci_config_getquad, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(config-addr -- data) read PCI config quadlet") },
	{ "config-l!", f_pci_config_setquad, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(data config-addr --) set PCI config quadlet") },

	{ "config-w@", f_pci_config_getdoub, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(config-addr -- data) read PCI config doublet") },
	{ "config-w!", f_pci_config_setdoub, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(data config-addr --) set PCI config doublet") },

	{ "config-b@", f_pci_config_getbyte, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(config-addr -- data) read PCI config byte") },
	{ "config-b!", f_pci_config_setbyte, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(data config-addr --) set PCI config byte") },

	{ "assign-package-address", f_pci_assign_package_address,
			INVALID_FCODE, F_NONE, T_FUNC
			HELP("(phandle --) assign PCI addrs") },

	{ "intr-ack", f_pci_intr_ack, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) ack PCI intrs") },
	{ "special-!", f_pci_special_set, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(data bus# --) send PCI special cycle") },

#ifdef BIG_ENDIAN
	{ "pci-rw@", f_pci_rwget, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(waddr -- w) read and byte-swap PCI doublet reg") },
	{ "pci-rw!", f_pci_rwset, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(w waddr --) byte-swap and set PCI doublet reg") },
	{ "pci-rl@", f_pci_rlget, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(qaddr -- quad) read and byte-swap PCI quad reg") },
	{ "pci-rl!", f_pci_rlset, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(quad qaddr --) byte-swap and set PCI quad reg") },
#endif

	{ NULL, NULL },
};

static Initentry pci_words[] =
{
	{ "probe-pci", f_probe_pci, INVALID_FCODE },
	{ "make-properties", f_make_properties, INVALID_FCODE },
	{ "assign-addresses", f_assign_addresses, INVALID_FCODE },

	/* [non-standard] */
	{ "show-pci", f_show_pci, INVALID_FCODE },
	{ "show-pci-full", f_show_pci_full, INVALID_FCODE },
	{ "dump-pciconfig", f_dump_pciconfig, INVALID_FCODE },

	{ NULL, NULL },
};

CC(install_pci)
{
	Package *pkg = new_pkg_name(e->root, "pci");
	Retcode ret;

	prop_set_str(pkg->props, "device_type", CSTR, "pci", CSTR);
	prop_set_int(pkg->props, "#address-cells", CSTR, 3);
	prop_set_int(pkg->props, "#size-cells", CSTR, 2);
	prop_set_int(pkg->props, "clock-frequency", CSTR, 33333333);

	if (find_table(e->names, "probe-pci", CSTR) == NULL)
		init_entries(e, e->names, pci_words);

	ret = init_entries(e, pkg->dict, pci_methods);

#ifdef BIG_ENDIAN
	if (ret != NO_ERROR)
		return ret;

	rwget_xtok = find_table(pkg->dict, "pci-rw@", CSTR);
	rwset_xtok = find_table(pkg->dict, "pci-rw!", CSTR);
	rlget_xtok = find_table(pkg->dict, "pci-rl@", CSTR);
	rlset_xtok = find_table(pkg->dict, "pci-rl!", CSTR);
#endif /* BIG_ENDIAN */

	e->currpkg = pkg;
	return ret;
}
