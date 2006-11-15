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

/* (c) Copyright 1996-1999 by CodeGen, Inc.  All Rights Reserved. */

#include "defs.h"

static Bool
package_name(Environ *e, Package *p, Byte *name)
{
	Entry *regent;
	Byte *str;
	Int len;
	Package *currpkg = e->currpkg;
	Cell *savesp = e->sp;
	
	if (prop_get_str(p->props, "name", CSTR, &str, &len) != NO_ERROR)
	    return FALSE;
	
	memcpy(name, str, len);
	name[len] = '\0';

	/* if there is a reg property convert it to a unit string */
	regent = find_table(p->props, "reg", CSTR);
	
	if (!regent)
		return TRUE;
	
	e->currpkg = p;
	PUSHP(e, regent->v.array);
	PUSH(e, regent->len);

	if (execute_word(e, "decode-phys") == NO_ERROR)
	{
		if (execute_static_method_name(e, p->parent, "encode-unit", CSTR) ==
				NO_ERROR)
		{
			POP(e, len);
			POPT(e, str, Byte*);
			strcat(name, "@");
			strncat(name, str, len);
		}
#ifdef SUN_COMPATIBILITY
		else
		{
			/* display the "reg" property as comma-separated hex digits */
			Int cells = get_address_cells(e->currpkg->parent);

			if (!CKSP(e, cells + 2, 0))
			{
				/* display phys.* values as hex */
				strcat(name, "@");

				while (cells--)
				{
					str = name + strlen(name);
					bprintf(str, (cells) ? "%x," : "%x", (uInt)TOP(e));
					DROP(e);
				}
			}
		}
#endif
	}

	e->currpkg = currpkg;
	e->sp = savesp;			/* restore the stack */
	return TRUE;
}

C(f_ls)			/* ls (["dir-path"] --) [non-standard extension] */
{
	Package *p;
	Byte *dev;

	dev = parse_word(e);

	if (dev && *dev)
	{
		p = find_device(e, dev, PSTR);

		if (p == NULL)
			return E_NO_DEVICE;
	}
	else
	{
		p = e->currpkg;

		if (p == NULL)
		{
			cprintf(e, "No device selected.\n");
			return NO_ERROR;
		}
	}

	/* if this is a leaf-node try running the $list-files command
	   [non-standard]
	 */
	if (p->children == NULL)
	{
		Byte name[STR_SIZE];

		if (find_table(p->dict, "read-blocks", CSTR) != NULL &&
				((dev && *dev) || get_device_name(e, p, name)))
		{
			Cell *savesp = e->sp;

			IFCKSP(e, 0, 2);

			if (dev && *dev)
			{
				PUSHP(e, dev + 1);
				PUSH(e, *dev);
			}
			else
			{
				PUSHP(e, name);
				PUSH(e, strlen(name));
			}

			(void)execute_word(e, "$list-files");	/* ignore any errors */
			e->sp = savesp;
		}

		return NO_ERROR;
	}

	for (p = p->children; p; p = p->link)
	{
		Byte name[STR_SIZE];

		if (!package_name(e, p, name))
			continue;

#ifdef SUN_COMPATIBILITY
		cprintf(e, "%0*p %s\n", sizeof p * 2, p, name);
#else
		cprintf(e, "%s\n", name);
#endif
	}

	return NO_ERROR;
}

Bool
get_device_name(Environ *e, Package *p, Byte *str)
{
	Byte name[STR_SIZE];

	if (p == e->root)
	{
		strcpy(str, "/");
		return TRUE;
	}

	*str = '\0';

	if (p == NULL || p->parent == NULL)
		return FALSE;

	get_device_name(e, p->parent, str);

	if (p->parent != e->root)
		strcat(str, "/");

	if (!package_name(e, p, name))
		return FALSE;

	strcat(str, name == NULL ? "?" : name);
	return TRUE;
}

Bool
get_interposed_device_name(Environ *e, Instance *i, Byte *str)
{
	Byte name[STR_SIZE];

	if (i->package == e->root)
	{
		strcpy(str, "/");
		return TRUE;
	}

	*str = '\0';
	get_interposed_device_name(e, i->parent, str);

	if (i->package != e->root)
		strcat(str, "/");

	if (i->interposed)
		strcat(str, "%");

	if (package_name(e, i->package, name))
		return FALSE;

	strcat(str, name == NULL ? "?" : name);
	return TRUE;
}

C(f_pwd)				/* pwd (--) */
{
	Byte str[STR_SIZE];
	
	if (e->currpkg == NULL)
	{
		cprintf(e, "No device selected.\n");
		return NO_ERROR;
	}

	if (!get_device_name(e, e->currpkg, str))
		cprintf(e, "/\n");
	else
		cprintf(e, "%s\n", str);
		
	return NO_ERROR;
}

C(f_ipwd)				/* ipwd (--) [non-standard] */
{
	Byte str[STR_SIZE];
	
	if ((Instance*)(uPtr)e->currinst == NULL)
	{
		cprintf(e, "No instance opened.\n");
		return NO_ERROR;
	}

	if (!get_interposed_device_name(e, (Instance*)(uPtr)e->currinst, str))
		cprintf(e, "/\n");
	else
		cprintf(e, "%s\n", str);
		
	return NO_ERROR;
}

Int
get_physical_cells(Package *pkg)
{
	Retcode ret;
	Int plen;
	
	if (pkg == NULL)
		return 2;
	
	ret = prop_get_int(pkg->props, "#physical-cells", CSTR, &plen);

	if (ret == NO_ERROR)
		return plen;
	
	return 2;
}

static const char *tags[] =
{
	"none",
	"fcode",
	"xt",
	"forth",
	"func",
	"variable",
	"value",
	"create",
	"defer",
	"buffer",
	"var",
	"constant",
	"2constant",
	"field",
	"val",
	"alias",
	"string",
	"property",
	"temp",
	"code",
	"label",
};

void
show_property_entry(Environ *e, Package *pkg, Entry *ent, char *leadin)
{
	Int i;
	Int len = ent->len;
	Byte *arr = ent->v.array;
	Byte name[STR_SIZE];

	if (ent->name)
	{
		memcpy(name, ent->name + 1, *(uByte*)ent->name);
		name[*(uByte*)ent->name] = '\0';
	}
	else
		strcpy(name, "<NULL>");

	if (len == 0)
		return;

	if (pkg)
	{
		if (strcmp(name, "reg") == 0 ||
				strcmp(name, "assigned-addresses") == 0 ||
				strcmp(name, "available") == 0 ||
				strcmp(name, "of-available") == 0 ||
				strcmp(name, "existing") == 0)
		{
			/* decode the reg property relative to package pkg */
			/* phys1 size1 phys2 size2 phys3 size3 ... */
			int acells = get_address_cells(pkg->parent);
			int scells = get_size_cells(pkg->parent);
			int cells = acells + scells;

			if (scells == 0 &&
					(strcmp(name, "available") == 0 ||
					strcmp(name, "of-available") == 0 ||
					strcmp(name, "existing") == 0))
			{
				acells = get_address_cells(e->root);
				scells = get_size_cells(e->root);
				cells = acells + scells;
			}

			if (!CKSP(e, 0, acells))
			{
				Cell *savesp = e->sp;
				Package *savepkg = e->currpkg;
				e->currpkg = pkg;

				while (len >= sizeof (Int) * cells)
				{
					Int strlen;
					Byte *str;
					Byte addr[STR_SIZE];
					int i;
					int nz = 0;

					PUSHP(e, arr);
					PUSH(e, len);
		
					if (execute_word(e, "decode-phys") != NO_ERROR ||
							execute_static_method_name(e, pkg->parent,
									"encode-unit", CSTR) != NO_ERROR)
					{
						e->sp = savesp;
						strcpy(addr, "<address unencodeable>");
					}
					else
					{
						POP(e, strlen);
						POPT(e, str, Byte*);
						DROPN(e, 2);	/* prop-addr2 prop-len2 from decode-phys */
						strncpy(addr, str, strlen);
						addr[strlen] = '\0';
					}

					arr += sizeof (Int) * acells;
					len -= sizeof (Int) * acells;
					cprintf(e, "%s:", addr);

					for (i = 0; i < scells; i++)
					{
						Int val;
						prop_decode_int(&arr, &len, &val);

						if (nz || val)
						{
							if (nz)
								cprintf(e, "%08x", val);
							else
								cprintf(e, "%x", val);

							nz = 1;
						}
					}

					if (!nz)
						cprintf(e, "0");

					if (len)
						cprintf(e, "\n%s%-22s", leadin, "");
				}

				e->sp = savesp;
				e->currpkg = savepkg;
				return;
			}
		}
		else if (strcmp(name, "bus-range") == 0)
		{
			Int vallo;
			Int valhi;
			prop_decode_int(&arr, &len, &vallo);
			prop_decode_int(&arr, &len, &valhi);
			cprintf(e, "%d:%d", vallo, valhi - vallo + 1);
			return;
		}
		else if (strcmp(name, "translations") == 0)
		{
			int acells = get_address_cells(e->root);
			int scells = get_size_cells(e->root);
			int pcells = get_physical_cells(e->root);
			int k;

#if 0
			cprintf(e, "%X -- %X", arr, len);

			if (len)
				cprintf(e, "\n%s%-22s", leadin, "");
#endif

			/* xxx:XXX --> XXX (XXX) */
			/* XXX must use #cells for virt, phys and size */
			while (len >= sizeof (Int) * 4)
			{
				uInt x;
				int zero = 1;

				for (k = 0; k < acells; k++)
				{
					prop_decode_int(&arr, &len, &x);

					if (zero && x == 0 && k < acells - 1)
						continue;

					if (zero)
						cprintf(e, "%X", x);
					else
						cprintf(e, "%08X", x);

					zero = 0;
				}

				cprintf(e, ":");
				zero = 1;

				for (k = 0; k < scells; k++)
				{
					prop_decode_int(&arr, &len, &x);

					if (zero && x == 0 && k < scells - 1)
						continue;

					if (zero)
						cprintf(e, "%X", x);
					else
						cprintf(e, "%08X", x);

					zero = 0;
				}

				cprintf(e, " --> ");
				zero = 1;

				for (k = 0; k < pcells; k++)
				{
					prop_decode_int(&arr, &len, &x);

					if (zero && x == 0 && k < pcells - 1)
						continue;

					if (zero)
						cprintf(e, "%X", x);
					else
						cprintf(e, "%08X", x);

					zero = 0;
				}


				prop_decode_int(&arr, &len, &x);
				cprintf(e, " (%X)", x);

				if (len)
					cprintf(e, "\n%s%-22s", leadin, "");
			}

			return;
		}
	}

	for (i = 0; i < len; i++)
		if (!isprint(arr[i]) && !isspace(arr[i]))
			break;

	if (i == len || (i == len - 1 && arr[i] == '\0'))
	{
#ifdef SUN_COMPATIBILITY
		cprintf(e, "%S", arr, i);
#else
		cprintf(e, "\"%S\"", arr, i);
#endif
		return;
	}

	if (len == 4)
	{
		Int val;
		prop_decode_int(&arr, &len, &val);
		cprintf(e, "%#x (%d)", val, val);
		return;
	}

	cprintf(e, "-- %4d: ", len--);
	i = 3;

	while (len > 0)
	{
		cprintf(e, "%02x", *arr++ & 0xFF);

		while (len-- && i++ < 16)
			cprintf(e, " %02x", *arr++ & 0xFF);

		i = 0;

		if (len > 0)
			cprintf(e, "\n%s%-22s", leadin, "");
	}
}

void
show_entry(Environ *e, Package *pkg, Entry *ent, char *leadin, Bool tag)
{
	Byte name[STR_SIZE];

	if (ent == NULL)
	{
		cprintf(e, "<unknown>");
		return;
	}

	if (ent->name && *ent->name)
		bprintf(name, "%P", ent->name);
	else
		bprintf(name, "(%#x)", ent->xtok);

	cprintf(e, "%s%-21s", leadin, name);
	
	switch (ent->type)
	{
	case T_CREATE:
	case T_FCODE:
	case T_FUNC:
		break;

	case T_CODE:
	case T_LABEL:
		cprintf(e, " %p", ent->v.addr);
		break;

	case T_REGISTER:
		cprintf(e, " %%%d", ent->v.val);
		break;

	case T_EXECTOKEN:
	case T_DEFER:
		if (ent->v.xtok >= 0)
			cprintf(e, " [xt %#x]", ent->v.xtok);

		break;

	case T_FORTH:
		cprintf(e, " \"%s\"", ent->v.array);
		break;

	case T_ADDR:
		cprintf(e, " %p(%Cd)", ent->v.addr,
				*(Cell *)ent->v.addr);
		break;

	case T_ADDRVAL:
		cprintf(e, " %Cd", *(Cell *)ent->v.addr);
		break;

	case T_BUFFER:
	case T_CONST:
	case T_VALUE:
		cprintf(e, " %Cd", ent->v.val);
		break;

	case T_VAR:
		cprintf(e, " %p(%Cd)", &ent->v.val, ent->v.val);
		break;

	case T_2CONST:
		cprintf(e, " %Cd,%Cd", ent->v.val, ent->len);
		break;

	case T_FIELD:
		cprintf(e, " [offset %Cd]", ent->v.val);
		break;

	case T_ALIAS:
		cprintf(e, " %P", ent->v.alias->name);
		break;

	case T_STRING:
		if (ent->v.array == NULL || ent->v.array[0] == '\0')
			cprintf(e, " <NULL>");
		else
			cprintf(e, " \"%P\"", ent->v.array);

		break;

	case T_PROPERTY:
		cprintf(e, " ");
		show_property_entry(e, pkg, ent, leadin);
		break;

	default:
		cprintf(e,
			" %p[type=%d,flags=%#x,xtok=%#x,val=%#Cx,link=%p,hlink=%p,len=%Cd]",
				ent, ent->type, ent->flags, ent->xtok, ent->v.val,
				ent->link, ent->hlink, ent->len);
		break;
	}

	if (ent->flags & F_IMMEDIATE)
		cprintf(e, " immediate");

	if (ent->flags & F_INSTANCE)
		cprintf(e, " instance[0x%X]", ent->offset);

	if (ent->flags & F_FCODE32)
		cprintf(e, " [32]");

	if (tag)
	{
		int type = ent->type;

		if (type >= 0 && type < sizeof tags / sizeof tags[0])
			cprintf(e, " %s", tags[type]);
		else
			cprintf(e, " type(%d)", type);

		cprintf(e, "[%#x]", ent->xtok);
	}
}

void
show_table(Environ *e, Package *p, Table *t, int type, char *leadin)
{
	int count = 0;
	Entry *ent;

	if (t == NULL)
		return;

	for (ent = t->list; ent; ent = ent->link)
	{
		count++;

		if (type && ent->type != type)
			continue;

		show_entry(e, p, ent, leadin, type == T_NONE);
		cprintf(e, "\n");
	}

#ifdef DEBUG
	if (count != t->num)
		cprintf(e, "%s-- num = %d, count = %d\n", leadin, t->num, count);

	if (t->hash)
	{
		int i, c;

		cprintf(e, "\n%sHASH TABLE:\n", leadin);
		count = 0;

		for (i = 0; i < TABLE_HASH; i++)
		{
			ent = t->hash[i];

			if (ent == NULL)
			{
				cprintf(e, "%s [%2d] <EMPTY>\n", leadin, i);
				continue;
			}

			for (c = 0; ent; ent = ent->hlink)
				c++;

			count += c;
			cprintf(e, "%s [%2d] <%2d>", leadin, i, c);

			for (ent = t->hash[i]; ent; ent = ent->hlink)
				cprintf(e, " %P", ent->name);

			cprintf(e, "\n");
		}

		cprintf(e, "%s num=%d, count=%d\n", leadin, t->num, count);
	}
#endif
}

C(f_dotproperties)		/* .properties (--) */
{
	if (e->currpkg == NULL)
	{
		cprintf(e, "No device selected.\n");
		return NO_ERROR;
	}

	show_table(e, e->currpkg, e->currpkg->props, T_PROPERTY, "");
	return NO_ERROR;
}

C(f_dotprop)		/* .prop ("propname" --) */
{
	Byte *propname = parse_word(e);
	Entry *ent;

	if (e->currpkg == NULL)
	{
		cprintf(e, "No device selected.\n");
		return NO_ERROR;
	}

	ent = find_table(e->currpkg->props, propname, PSTR);

	if (ent && ent->name && *ent->name)
	{
		Byte name[STR_SIZE];
		bprintf(name, "%P", ent->name);
		cprintf(e, "%-21s ", name);
		show_property_entry(e, e->currpkg, ent, "");
	}
	else
		cprintf(e, "Property not found.\n");

	return NO_ERROR;
}

static void
show_devices(Environ *e, Package *pkg)
{
	Package *p;
	Retcode ret;
	Byte *type;
	Int tlen;
	Byte name[STR_SIZE];

	if (!get_device_name(e, pkg, name))
		return;

	ret = prop_get_str(pkg->props, "device_type", CSTR, &type, &tlen);

	if (ret == NO_ERROR)
		cprintf(e, "%s (%S)\n", name, type, tlen);
	else
		cprintf(e, "%s\n", name);

	/* show the names of our children and their subtrees */
	for (p = pkg->children; p; p = p->link)
		show_devices(e, p);
}

C(f_show_devs)		/* show-devs ("{device-specifier}<eol>" --) */
{
	Byte *start;
	Package *savepkg = e->currpkg;
	Package *pkg;

	start = parse_line(e);
	
	if (start && *start)
	{
		pkg = find_device(e, start, PSTR);

		if (pkg == NULL)
			return E_ABORT;
	}
	else
		pkg = e->root;

	show_devices(e, pkg);
	e->currpkg = savepkg;
	return NO_ERROR;
}


/* 7.4.11.1  Device alias */

Retcode
make_devalias(Environ *e, Byte *alias, Int alen, Byte *device, Int dlen)
{
	if (compare_strs(alias, alen, "name", CSTR))
		return E_NO_PROPERTY;

	return prop_set_str(e->aliases->props, alias, alen, device, dlen);
}

C(f_devalias)		/* devalias ("{alias-name}< >{device-specifier}<cr>" --) */
{
	Entry *ent;
	Byte *alias;
	Byte *device;
	Byte *str;
	Int len;
	Int alen;

	alias = parse_line(e);
	
	if (*alias)
	{
		device = alias + 1;
		len = *alias;

		while (len && !isblank(*device))
		{
			device++;
			len--;
		}

		alen = *alias - len;

		while (len && isblank(*device))
		{
			device++;
			len--;
		}

		/* cannot check to make sure device exists as we may be called
		   during bootup in nvramrc and devices may not be probed yet */

		if (len)
			return make_devalias(e, alias + 1, alen, device, len);
	}
	
	if (!*alias)
	{
		cprintf(e, "%-21s %s\n", "Alias", "Device Path");
		cprintf(e, "--------------------------------------------------\n");
	}

	for (ent = e->aliases->props->list; ent; ent = ent->link)
	{
		if (compare_strs(ent->name, PSTR, "name", CSTR))
			continue;

		if (*alias && !compare_strs(ent->name, PSTR, alias, PSTR))
			continue;

		prop_get_str(e->aliases->props, ent->name, PSTR, &str, &len);
		cprintf(e, "%-21P %S\n", ent->name, str, len);
	}

	return NO_ERROR;
}


/* The following unmitigated garbage is to handle the OpenFirmware speccified
   algorithm (pp. 38-43) for Path Resolution (scn. 4.3), plus the OF Errata,
   plus the Interposition Recommended Practice.
 */

/* is the path-resolution one of these? */
enum context
{
	IS_FIND,
	IS_OPEN,
	IS_EXEC
};

static void
left_split(char *str, int ch, char *head, char *tail)
{
	char *s;
	
	if (head != str)
		strcpy(head, str);
	
	s = strchr(head, ch);
	
	if (s)
	{
		*s++ = '\0';
		strncpy(tail, s, STR_SIZE - 2);
		tail[STR_SIZE - 1] = '\0';
	}
	else
		tail[0] = '\0';
}

static void
split_before(char *str, int ch, char *head, char *tail)
{
	char *s;
	
	if (head != str)
		strcpy(head, str);

	s = strchr(head, ch);
	
	if (s)
	{
		strncpy(tail, s, STR_SIZE - 2);
		tail[STR_SIZE - 1] = '\0';
		*s = '\0';			/* ch belongs in tail, so kill it here */
	}
	else
		tail[0] = '\0';
}

static void
split_after(char *str, int ch, char *head, char *tail)
{
	char *s;
	
	if (head != str)
		strcpy(head, str);

	s = strrchr(head, ch);
	
	if (s)
	{
		s++;				/* ch belongs at end of 1st string */
		strncpy(tail, s, STR_SIZE - 2);		/* copy rest of string */
		tail[STR_SIZE - 1] = '\0';
		*s++ = '\0';		/* and terminate the 1st */
	}
	else
		tail[0] = '\0';
}

static void
catenate(char *head, char *middle, char *tail, char *buf)
{
	strncpy(buf, head, STR_SIZE - 2);
	strncat(buf, middle, STR_SIZE - 2);
	strncat(buf, tail, STR_SIZE - 2);
	buf[STR_SIZE - 1] = '\0';
}

static Bool
do_decode_reg(Environ *e, Package *pkg, Cell unit[])
{
	Entry *ent = find_table(pkg->props, "reg", CSTR);
	Int i, acells, plen, t;
	Byte *paddr;
	Retcode ret = NO_ERROR;
	
	if (!ent)
		return FALSE;
	
	acells = get_address_cells(pkg->parent);
	paddr = ent->v.array;
	plen = ent->len;
	
	for (i = acells - 1; ret == NO_ERROR && i >= 0; i--)
	{
		ret = prop_decode_int(&paddr, &plen, &t);
		unit[i] = (uInt)t;
	}

	return TRUE;
}

static Retcode
do_decode_phys(Environ *e, char *unit, Cell units[])
{
	Retcode ret;
	Int acells, i;
	Cell *savesp = e->sp;
	
	acells = get_address_cells(e->currpkg);
	IFCKSP(e, 0, 2);
	PUSHP(e, unit);
	PUSH(e, strlen(unit));
	ret = execute_static_method_name(e, e->currpkg, "decode-unit", CSTR);
	DPRINTF(("do_decode_phys: pkg=%p unit=%s acells=%d ret=%d:%s\n",
			e->currpkg, unit, acells, ret, err2str(ret)));
	
	if (ret != NO_ERROR)
	{
		e->sp = savesp;
		return ret;
	}
	
	IFCKSP(e, acells - 1, 0);
	
	for (i = acells - 1; i >= 0; i--)
	{
		POP(e, units[i]);

#ifdef SF_64BIT
		units[i] &= QUADLET_MASK;
#endif
	}
	
	DPRINTF(("do_decode_phys: units=%#Cx.%Cx.%Cx.%Cx\n",
			units[0], units[1], units[2], units[3]));
	e->sp = savesp;
	return NO_ERROR;
}

EC(f_set_args);

static Instance *
create_new_inst(Environ *e, char *args, char *unit,
		Instance **parent, Cell unitphys[])
{
	Instance *inst = e->currpkg->initinst ? 
			dup_instance(e->currpkg->initinst) :
			new_instance(e->currpkg);
	Int acells;

	if (inst == NULL)
		return NULL;

	e->currinst = (uPtr)inst;
	inst->args = args ? cstrdup(args) : NULL;
	inst->parent = *parent;
	*parent = inst;
	acells = inst->numunits;
	
	/* e->unit is already initialized to zeros */
	if (!unit || !*unit)
		do_decode_reg(e, e->currpkg, inst->unit);
	else
		memcpy(inst->unit, unitphys, sizeof (Cell) * acells);
	
	/* if there are instance-specific vars, the xtoks[] table needs
	   to point to the newly created vars and not the ones in initinst */
	if (inst->dict)
	{
		Entry *ent;

		for (ent = inst->dict->list; ent; ent = ent->link)
			e->xtoks[ent->xtok] = ent;
	}

	return inst;
}

static Bool
name_match(Environ *e, char *name, Byte *pname, Int plen)
{
	DPRINTF(("name_match: <%S> <%s>\n", pname, plen, name));

	if (compare_strs(name, CSTR, pname, plen))
		return TRUE;

	while (plen-- > 0 && *pname++ != ',')
		;

	if (plen <= 0)
		return FALSE;

	return (compare_strs(pname, plen, name, CSTR)) ? TRUE : FALSE;
}

static Bool
exact_match(Environ *e, Package *pkg, char *name, char *unit, Cell unitphys[])
{
	Cell units[MAX_ADDR_CELLS];
	Byte *pname;
	Int plen;
	
	if (prop_get_str(pkg->props, "name", CSTR, &pname, &plen) != NO_ERROR)
		return FALSE;
		
	if (!name_match(e, name, pname, plen))
		return FALSE;
	
	if (unit && *unit)
	{
		if (!do_decode_reg(e, pkg, units))
			return FALSE;

	DPRINTF(("exact_match: name=%s parent=%p acells=%d units=%#Cx.%Cx.%Cx.%Cx",
				name, pkg->parent, get_address_cells(pkg->parent), 
				units[0], units[1], units[2], units[3]));
		DPRINTF((" unitphys=%#Cx.%Cx.%Cx.%Cx\n",
				unitphys[0], unitphys[1], unitphys[2], unitphys[3]));

		if (memcmp(units, unitphys,
				sizeof (Cell) * get_address_cells(pkg->parent)) != 0)
			return FALSE;
	}
	else if (!name || !*name)
		return FALSE;
	
	return TRUE;
}

static Bool
wildcard_match(Environ *e, Package *pkg, char *name, char *unit)
{
	Byte *pname;
	Int plen;
	
	if (prop_get_str(pkg->props, "name", CSTR, &pname, &plen) != NO_ERROR)
		return FALSE;
		
	if (!name_match(e, name, pname, plen))
		return FALSE;
	
	if (find_table(pkg->props, "reg", CSTR))	/* has a "reg" property? */
		return FALSE;
		
	if ((!unit || !*unit) && (!name || !*name))
		return FALSE;
	
	return TRUE;
}

static Package *
find_matching_child(Environ *e, char *name, char *unit, Cell unitphys[])
{	
	Retcode ret;
	Package *pkg;

	if (unit && *unit)
	{
		ret = do_decode_phys(e, unit, unitphys);

		if (ret != NO_ERROR)
			return NULL;
	}

	for (pkg = e->currpkg->children; pkg; pkg = pkg->link)
		if (exact_match(e, pkg, name, unit, unitphys))
			return pkg;

	for (pkg = e->currpkg->children; pkg; pkg = pkg->link)
		if (wildcard_match(e, pkg, name, unit))
			return pkg;

	/* optional step to locate nodes without intermediate nodes in the path */

	for (pkg = e->currpkg->children; pkg; pkg = pkg->link)
	{
		Package *sav = e->currpkg;
		Package *rpkg;

		e->currpkg = pkg;
		rpkg = find_matching_child(e, name, unit, unitphys);
		e->currpkg = sav;

		if (rpkg)
			return rpkg;
	}

	return NULL;
}

/* 4.3.7 - Handle interposers procedure */
static Instance *
handle_interposers(Environ *e, Instance *inst, char *unit,
		Instance **parent, Cell unitphys[])
{
	Package *savepkg = e->currpkg;
	Package *ipkg;
	Instance *newinst;
	Retcode ret;
	Int retval = FFALSE;
	Byte args[STR_SIZE];

	while (e->ipos_package)
	{
		/* unschedule interposition package */
		ipkg = e->ipos_package;
		
		if (e->ipos_arg && e->ipos_len)
		{
			memcpy(args, e->ipos_arg + 1, e->ipos_len);
			args[e->ipos_len] = '\0';
		}
		else
			*args = '\0';
		
		e->ipos_package = NULL;

		/* save current package and set it to interposed package */
		savepkg = e->currpkg;
		e->currpkg = ipkg;

		/* create a new instance of it and open the package */
		newinst = create_new_inst(e, args, unit, parent, unitphys);

		if (newinst == NULL)
			return NULL;

		newinst->interposed = TRUE;
		ret = execute_method_name(e, newinst, "open", CSTR);

		if (ret == NO_ERROR)
		{
			if (CKSP(e, 0, 1))
			{
				inst = NULL;
				break;
			}

			POP(e, retval);
		}
		
		if (ret != NO_ERROR || retval != FTRUE)
		{
			error(ret);
			inst = NULL;
			break;
		}

		inst = newinst;
		e->currpkg = savepkg;		/* restore original package */
	}

	e->currpkg = savepkg;		/* restore original package */
	return inst;
}

static Instance *
new_and_open(Environ *e, Package *pkg, char *args, char *unit,
		Instance **parent, Cell unitphys[])
{
	Retcode ret;
	Instance *inst;
	Package *savepkg = e->currpkg;
	Int retval = FFALSE;

	e->currpkg = pkg;
	inst = create_new_inst(e, args, unit, parent, unitphys);

	if (inst == NULL)
		return NULL;

	ret = execute_method_name(e, inst, "open", CSTR);
	e->currpkg = savepkg;

	if (ret == NO_ERROR)
	{
		if (CKSP(e, 0, 1))
			return NULL;

		POP(e, retval);
	}

	if (ret != NO_ERROR || retval != FTRUE)
	{
		error(ret);
		return NULL;		/* error during open - abort */
	}
	
	inst = handle_interposers(e, inst, unit, parent, unitphys);
	return inst;
}

static Instance *
create_well_formed_chain(Environ *e, Package *pkg)
{
	Instance *parent, *inst;

#ifdef DEBUG
	Byte buf[STR_SIZE];

	dprintf("create_well_formed_chain: pkg=%p parent=%p currpkg=%p inst=%p\n",
			pkg, pkg->parent, e->currpkg, (Instance*)(uPtr)e->currinst);
	get_device_name(e, pkg, buf);
	dprintf("\tpkg=%s", buf);
	get_device_name(e, pkg->parent, buf);
	dprintf(" parent=%s", buf);
	get_device_name(e, e->currpkg, buf);
	dprintf(" currpkg=%s", buf);
	get_device_name(e, ((Instance*)(uPtr)e->currinst)->package, buf);
	dprintf(" currinst=%s\n", buf);
#endif

	if (pkg == NULL)
		return NULL;

	if (pkg == e->currpkg)
		return (Instance*)(uPtr)e->currinst;

	parent = create_well_formed_chain(e, pkg->parent);
	e->currpkg = pkg;
#ifdef DEBUG
	get_device_name(e, pkg, buf);
	dprintf("\tpkg=%p:%s", pkg, buf);
	get_device_name(e, parent->package, buf);
	dprintf(" parent=%p:%s", parent, buf);
#endif
	inst = new_and_open(e, pkg, "", "", &parent, NULL);
#ifdef DEBUG
	get_device_name(e, inst->package, buf);
	dprintf(" inst=%p:%s instparent=%p\n", inst, buf, inst->parent);
#endif

	return inst;
}

/* return NO_ERROR if things succeeded, Retcode for any exception
 */
static Int
resolve_path(Environ *e, enum context context, Byte *path, Int plen,
		Byte *method, Instance **retinst)
{
	Byte head[STR_SIZE], tail[STR_SIZE], args[STR_SIZE],
			name[STR_SIZE], unit[STR_SIZE];
	Byte *ustr;
	Int ulen;
	Instance *parent;
	Instance *inst = NULL;
	Package *child;
	Cell saveinst = e->currinst;
	Package *savepkg = e->currpkg;
	Retcode ret = E_NO_DEVICE;
	Int retval = NO_ERROR;
	Cell unitphys[MAX_ADDR_CELLS];
	
	/* clear various buffers */
	memset(unitphys, 0, sizeof unitphys);
	memset(tail, 0, sizeof tail);
	memset(args, 0, sizeof args);
	memset(name, 0, sizeof name);
	memset(unit, 0, sizeof unit);

	/* turn whatever string we have into a C string so we can scribble in it */
	setstrlen(&path, &plen);
	strncpy(head, path, plen);
	head[plen] = '\0';
	path = head;
	DPRINTF(("resolve_path: head=%s\n", head));
	
	/* if path does not begin with a '/', check for an alias, and expand it */
	if (*head != '/')
	{
		split_before(head, '/', head, tail);
		split_before(head, ':', name, args);
		ret = prop_get_str(e->aliases->props, name, CSTR, &ustr, &ulen);
		DPRINTF(("resolve_path: alias: name=%s head=%s tail=%s args=%s\n",
				name, head, tail, args));
		ret = E_NO_DEVICE;
		
		if (ustr && ulen)
		{
			strncpy(name, ustr, ulen);
			name[ulen] = '\0';
			
			if (*args)
			{
				Byte ahead[STR_SIZE], atail[STR_SIZE], dead_args[STR_SIZE];
				
				split_after(name, '/', ahead, atail);
				split_after(atail, ':', atail, dead_args);
				catenate(ahead, atail, args, name);
				DPRINTF(("resolve_path: alias: name=%s ahead=%s atail=%s"
						" args=%s\n", name, ahead, atail, args));
			}

			catenate(name, tail, "", path);
			DPRINTF(("resolve_path: alias: name=%s tail=%s args=%s\n",
					name, tail, args));
		}
	}

	/* if path starts with '/', start search at root, else at currpkg */
	if (*path == '/')
	{
		Byte temp[STR_SIZE];
		strcpy(temp, path);
		strcpy(path, temp + 1);
		e->currpkg = e->root;
	}
	else if (e->currpkg == NULL)
		return E_NULL_PACKAGE;
	
	/* begin creation of an instance chain */
	parent = NULL;
	*args = '\0';
	*unit = '\0';
	
	while (*path)
	{
		DPRINTF(("resolve_path: path=%s head=%s tail=%s args=%s unit=%s\n",
				path, head, tail, args, unit));

		/* open the node if appropriate */
		if (context == IS_EXEC || context == IS_OPEN)
		{
			inst = create_new_inst(e, args, unit, &parent, unitphys);

			if (inst == NULL)
				return E_OUT_OF_MEMORY;

			ret = execute_method_name(e, inst, "open", CSTR);
			DPRINTF(("resolve_path: open: inst=%p ret=%d:%s\n",
					inst, ret, err2str(ret)));

			if (ret == NO_ERROR)
			{
				IFCKSP(e, 0, 1);
				POP(e, retval);
			}

			if (ret != NO_ERROR || retval != FTRUE)
			{
				/* error during open - abort */
				if (ret == NO_ERROR)
					ret = E_ABORT;

				inst = parent;
				goto clean_exit;
			}

			inst = handle_interposers(e, inst, unit, &parent, unitphys);

			if (inst == NULL)
			{
				inst = parent;
				ret = E_NULL_INSTANCE;
				goto clean_exit;
			}
		}
		
		for (;;)
		{
			/* parse the next node into its components */
			left_split(path, '/', name, path);
			left_split(name, ':', name, args);
			left_split(name, '@', name, unit);

			if (name[0] == '%')		/* handle interposition */
			{
				Package *pkg;

				strcpy(tail, name + 1);		/* get rid of '%' */

				for (pkg = e->packages->children; pkg; pkg = pkg->link)
					if (exact_match(e, pkg, tail, unit, unitphys))
						break;
				
				if (pkg)
				{
					inst = new_and_open(e, pkg, args, unit, &parent, unitphys);

					if (inst == NULL)
					{
						inst = parent;
						ret = E_NULL_INSTANCE;
						goto clean_exit;
					}

					continue;
				}
			}

			break;
		}

		child = find_matching_child(e, name, unit, unitphys);
		DPRINTF(("resolve_path: currpkg=%p child=%p name=%s unit=%s args=%s\n",
				e->currpkg, child, name, unit, args));
	
		if (child)
		{
			/* create well-formed instance chains if appropriate */
			if ((context == IS_EXEC || context == IS_OPEN) &&
					child->parent != NULL && child->parent != e->currpkg)
			{
				Instance *i = create_well_formed_chain(e, child->parent);

				if (i == NULL)
				{
					ret = E_NULL_INSTANCE;
					goto clean_exit;
				}

				parent = i;
			}
			
			e->currpkg = child;
		}
		else
		{
			e->currinst = saveinst;
			e->currpkg = savepkg;
			ret = E_NO_DEVICE;
			/* inst is whatever is currently still open */
			goto clean_exit;
		}
	}
	
	/* at this point the final node name has been selected */
	switch (context)
	{
	case IS_OPEN:
		inst = create_new_inst(e, args, unit, &parent, unitphys);
		ret = execute_method_name(e, inst, "open", CSTR);

		if (ret == NO_ERROR)
		{
			IFCKSP(e, 0, 1);
			POP(e, retval);
		}

		if (ret != NO_ERROR || retval != FTRUE)
		{
			/* error during open - abort */
			if (ret == NO_ERROR)
				ret = E_ABORT;

			inst = parent;
			goto clean_exit;
		}
		
		inst = handle_interposers(e, inst, unit, &parent, unitphys);

		if (inst == NULL)
		{
			inst = parent;
			ret = E_NULL_INSTANCE;
			goto clean_exit;
		}

		e->currinst = saveinst;
		e->currpkg = savepkg;
		
		if (retinst)
			*retinst = inst;
		
		return NO_ERROR;
	
	case IS_EXEC:
		inst = create_new_inst(e, args, unit, &parent, unitphys);
		ret = execute_method_name(e, inst, method, CSTR);

		/* go through clean_exit code to clean up open instances */
		/* note we did not open inst and so do not have to close it */
		break;
	
	case IS_FIND:
		return NO_ERROR;
	}

clean_exit:
	DPRINTF(("resolve_path: clean_exit: name=%s retval=%d ret=%s\n",
			name, retval, err2str(ret)));

	/* clean up any open instances and return the error code */
	if (inst)
	{
		parent = inst->parent;		/* open failed so do not run close */
		delete_instance(inst);
	
		while (parent != NULL && parent != (Instance*)(uPtr)saveinst)
		{
			inst = parent;
			parent = inst->parent;
			execute_method_name(e, inst, "close", CSTR);
			delete_instance(inst);
		}
	}

	e->currinst = saveinst;
	e->currpkg = savepkg;
	return ret;
}

Package *
find_device(Environ *e, Byte *str, Int len)
{
	Retcode ret;
	Package *savepkg, *pkg;

	setstrlen(&str, &len);

	if (len == 2 && str[0] == '.' && str[1] == '.' &&
			e->currpkg && e->currpkg->parent)
		return e->currpkg->parent;

	if (len == 0)
		return NULL;

	savepkg = e->currpkg;
	ret = (Retcode)resolve_path(e, IS_FIND, str, len, NULL, NULL);
	pkg = e->currpkg;
	e->currpkg = savepkg;
	return (ret == NO_ERROR) ? pkg : NULL;
}

Retcode
do_interpose(Environ *e, Package *pkg, Byte *arg, Int len)
{
	if (e->ipos_arg)
		free(e->ipos_arg);
	
	e->ipos_package = pkg;
	e->ipos_len = len;
	e->ipos_arg = lstrdup(arg, len);
	
	if (e->ipos_arg == NULL)
		return E_OUT_OF_MEMORY;
	
	return NO_ERROR;
}

C(f_interpose)		/* interpose (adr len phandle --) 0x12B */
{
	Package *pkg;
	Byte *arg;
	Int len;

	IFCKSP(e, 3, 0);
	POPT(e, pkg, Package*);
	POP(e, len);
	POPT(e, arg, Byte*);
	return do_interpose(e, pkg, arg, len);
}

CC(f_find_device)	/* find-device (dev-str dev-len --) */
{
	Int len;
	Byte *str;

	IFCKSP(e, 2, 0);
	POP(e, len);
	POPT(e, str, Byte*);
	e->currpkg = find_device(e, str, len);
	return (e->currpkg) ? NO_ERROR : E_NO_DEVICE;
}

C(f_dev)			/* dev ("device-name<cr>" --) */
{
    Byte *str = parse_line(e);
    PUSHP(e, str + 1);
    PUSH(e, strlen(str + 1));
    return f_find_device(e);
}


C(f_device_end)		/* device-end (--) */
{
	e->currpkg = NULL;
	return NO_ERROR;
}

/* 7.7  FCode Debugging command group */

/* The following three words define the scope of the following fcode names.
 */

C(f_external)		/* external (--) */
{
	e->headertok = 0xCA;		/* external-token */
	return NO_ERROR;
}

C(f_headerless)		/* headerless (--) */
{
	e->headertok = 0xB5;		/* new-token */
	return NO_ERROR;
}

C(f_headers)		/* headers (--) */
{
	e->headertok = 0xB6;		/* named-token */
	return NO_ERROR;
}

/* fcode-debug? (-- names?) */
EC(f_push_config_bool);

CC(f_open_dev)		/* open-dev (dev-str dev-len -- ihandle | 0) */
{
	Int len;
	Retcode ret;
	Byte *str;
	Instance *inst;
	
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, str, Byte*);
	ret = (Retcode)resolve_path(e, IS_OPEN, str, len, NULL, &inst);

	PUSHP(e, (ret == NO_ERROR) ? inst : NULL);
	return (ret == E_USER_ABORT || ret == R_END) ? ret : NO_ERROR;
}

CC(f_close_dev)		/* close-dev (ihandle --) */
{
	Instance *inst, *parent;
	Package *currpkg = e->currpkg;
	
	IFCKSP(e, 1, 0);
	POPT(e, inst, Instance*);
	
	while (inst)
	{
		parent = inst->parent;
		e->currpkg = inst->package;
		execute_method_name(e, inst, "close", CSTR);

		/* sanity check to prevent bogus symbol-table lookups */
		if ((Instance*)(uPtr)e->currinst == inst)
		{
			e->currinst = 0;
			e->currpkg = (Package *)0;
		}

		delete_instance(inst);
		inst = parent;
	}
	
	e->currpkg = currpkg;
	return NO_ERROR;
}

/* begin-package (arg-str arg-len reg-str reg-len dev-str dev-len --) */
C(f_begin_package)
{
	Instance *inst;
	Retcode ret;

	IFCKSP(e, 0, 6);
	ret = execute_word(e, "open-dev");		/* takes top 2 args */

	if (ret != NO_ERROR)
		return ret;

	POP(e, e->currinst);		/* ihandle returned by open-dev */
	inst = (Instance*)(uPtr)e->currinst;

	if (inst == NULL)
		return E_NULL_INSTANCE;

	e->currpkg = inst->package;
	ret = execute_word(e, "new-device");

	if (ret != NO_ERROR)
		return ret;

	return execute_word(e, "set-args");		/* takes remaining 4 args */
}

C(f_end_package)	/* end-package (--) */
{
	Retcode ret;

	ret = execute_word(e, "finish-device");

	if (ret == NO_ERROR)
		ret = execute_word(e, "forth");

	if (ret == NO_ERROR)
	{
		IFCKSP(e, 0, 1);
		PUSH(e, e->currinst);
		ret = execute_word(e, "close-dev");
	}

	return ret;
}

C(f_apply)		/* apply (... "method-name< >device-specifiers< >" -- ?) */
{
	Byte *method, *devspec;
	Retcode ret;
	Int r;

	IFCKSP(e, 0, 4);
	method = parse_word(e);

	if (method == NULL || *method == 0)
		return E_EMPTY_STRING;

	devspec = parse_line(e);

	if (devspec == NULL || *devspec == 0)
		return E_NO_DEVICE;

	PUSHP(e, devspec + 1);
	PUSH(e, *(uByte*)devspec);
	PUSHP(e, method + 1);
	PUSH(e, *(uByte*)method);
	ret = execute_word(e, "execute-device-method");

	if (ret == NO_ERROR)
	{
		POP(e, r);

		if (r == FTRUE)
			ret = E_ABORT;
	}

	return ret;
}

/* execute-device-method (... dev-str dev-len method-str method-len --
		... false | ? true) */
CC(f_exec_dev_method)
{
	Int len, mlen;
	Retcode ret;
	Byte *str, *method;
	Byte mstr[STR_SIZE];
	
	IFCKSP(e, 4, 1);
	POP(e, mlen);
	POPT(e, method, Byte*);
	POP(e, len);
	POPT(e, str, Byte*);
	setstrlen(&method, &mlen);
	memcpy(mstr, method, mlen);
	mstr[mlen] = '\0';
	ret = (Retcode)resolve_path(e, IS_EXEC, str, len, mstr, NULL);
	PUSH(e, (ret == NO_ERROR) ? FTRUE : FFALSE);
	return (ret == E_USER_ABORT || ret == R_END) ? ret : NO_ERROR;
}

const Initentry init_device[] =
{
	{ "ls",            f_ls,            INVALID_FCODE, F_PAGINATE, T_FUNC
		HELP("([\"dir-spec\"] --)" \
			"display names of active package's children\n" \
			"\t[non-standard extension] show files/dirs using list-files") },
	{ "pwd",           f_pwd,           INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) display device path of active package") },
	{ "ipwd",          f_ipwd,          INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) display interposed device path of current instance") },
			/* non-standard */
	{ ".attributes",   f_dotproperties, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(--) historic alias to .properties") },
			/* historic alias */
	{ ".properties",   f_dotproperties, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(--) display names and values of " \
					"properties in active package") },
	{ ".prop",         f_dotprop, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(--) display value of property in active package") },
#ifdef DEVEL
	{ "props",         f_dotproperties, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(--) display names and values of " \
					"properties in active package") },
#endif
	{ "show-devs",     f_show_devs,     INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(\"{device-specifier}<eol>\" --) " \
					"show all devices beneath node, or /") },
	{ "devalias",      f_devalias,      INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(\"{alias-name}< >{device-specifier}<cr>\" --)\n" \
					"\tcreate device alias or list current alias(es)") },

	{ "interpose",     f_interpose,     0x12B, F_NONE, T_FUNC
			HELP("(args len phandle --)\n" \
				"\tschedule package phandle with args for interposition") },
	{ "find-device",   f_find_device,   INVALID_FCODE, F_NONE, T_FUNC
			HELP("(dev-str dev-len --) " \
					"make specified device the active package") },
	{ "cd",            f_dev,           INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"device-name<cr>\" --) historic alias to dev") },
			/* historic alias */
	{ "dev",           f_dev,           INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"device-name<cr>\" --) " \
					"make specified device the active package") },
	{ "device-end",	   f_device_end,    INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) unselect active package leaving none selected") },

	{ "external", f_external, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) newly created functions will be visible") },
	{ "headerless", f_headerless, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) newly created functions will be invisible") },
	{ "headers", f_headers, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) newly created functions will be optionally visible") },
	{ "fcode-debug?", f_push_config_bool, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- names?) if true, save names for FCodes with headers") },

	{ "open-dev",      f_open_dev,      INVALID_FCODE, F_NONE, T_FUNC
			HELP("(device-str device-len -- ihandle | 0)\n" \
					"\topen device (and parents) named by device") },
	{ "close-dev",     f_close_dev,     INVALID_FCODE, F_NONE, T_FUNC
			HELP("(ihandle --) close device and all of its parents") },
	{ "begin-package", f_begin_package, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(arg-str arg-len reg-str reg-len dev-str dev-len --)\n" \
					"\tset up device tree before creating new node") },
	{ "end-package",   f_end_package,   INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) close device tree set up with begin-package") },
	{ "apply",         f_apply,         INVALID_FCODE, F_NONE, T_FUNC
			HELP("(... \"method-name< >device-specifiers< >\" -- ?)\n" \
					"\texecute named method in the specified package") },
	{ "decode-bytes",  (Command)">r over r@ + swap r@ - rot r>",
			INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(prop-addr1 prop-len1 data-len --\n" \
					"\t\tprop-addr2 prop-len2 data addr data-len)\n" \
					"\tdecode a byte array from a prop-encoded-array") },
	{ "execute-device-method", f_exec_dev_method, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(... dev-str dev-len method-str method-len --\n" \
					"\t\t... false | ? true)\n" \
				"\texecute the named method in the package named dev-str") },

	{ NULL, NULL }
};
