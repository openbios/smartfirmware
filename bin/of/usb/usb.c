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

/* Common USB routines for all controller chips.
 */

#include "defs.h"
#include "pci.h"
#include "usb.h"


/* dummy open method */
CC(f_usb_open)			/* open (-- okay?) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, FTRUE);
	return NO_ERROR;
}

/* dummy close method */
CC(f_usb_close)			/* close (--) */
{
	return NO_ERROR;
}


CC(f_usb_port_decode_unit)	/* decode-unit (str len -- port) */
{
	Byte *str;
	Int slen;
	Cell err, port;

	IFCKSP(e, 2, 1);
	POP(e, slen);
	POPT(e, str, Byte*);
	setstrlen(&str, &slen);
	parse_number(16, &str, &slen, &port, &err, FALSE);

	if (err)
		return E_BAD_NUM_FORMAT;

	PUSH(e, port);
	return NO_ERROR;
}

CC(f_usb_port_encode_unit)	/* encode-unit (port -- str len) */
{
	static Byte buf[32];
	uInt port;
	Byte *s = buf;

	IFCKSP(e, 1, 2);
	POP(e, port);
	*s = '\0';
	bprintf(s, "%x", port);
	PUSHP(e, buf);
	PUSH(e, strlen(buf));
	return NO_ERROR;
}


CC(f_usb_dev_decode_unit)	/* decode-unit (str len -- config# interface#) */
{
	Byte *str;
	Int slen;
	Cell err, config, interface;

	IFCKSP(e, 2, 2);
	POP(e, slen);
	POPT(e, str, Byte*);
	setstrlen(&str, &slen);
	parse_number(16, &str, &slen, &interface, &err, FALSE);

	if (err)
		return E_BAD_NUM_FORMAT;

	if (*str++ == ',')
	{
		parse_number(16, &str, &slen, &config, &err, FALSE);

		if (err)
			return E_BAD_NUM_FORMAT;
	}
	else
		config = 1;

	PUSH(e, config);
	PUSH(e, interface);
	return NO_ERROR;
}

CC(f_usb_dev_encode_unit)	/* encode-unit (config# interface# -- str len) */
{
	static Byte buf[32];
	uInt config, interface;
	Byte *s = buf;

	IFCKSP(e, 2, 2);
	POP(e, interface);
	POP(e, config);
	*s = '\0';

	if (config == 1)
		bprintf(s, "%x", interface);
	else
		bprintf(s, "%x,%x", interface, config);

	PUSHP(e, buf);
	PUSH(e, strlen(buf));
	return NO_ERROR;
}

const Initentry usb_device_methods[] =
{
	{ "open",			f_usb_open,				INVALID_FCODE },
	{ "close",			f_usb_close,			INVALID_FCODE },
	{ "decode-unit",	f_usb_dev_decode_unit,	INVALID_FCODE },
	{ "encode-unit",	f_usb_dev_encode_unit,	INVALID_FCODE },
	{ NULL,				NULL },
};


/* generic common USB stuff for all interface chips
 */

Retcode
install_usb_root_driver(Environ *e, Package *pkg, Pci_device_info *devinfo,
		const Initentry methods[], uInt size)
{
	Retcode ret;
	Pci_device_info *newinfo;

	DPRINTF(("install_usb_driver: pkg=%p devinfo=%p"
			" class=%#x vendid=%#x devid=%#x\n", pkg, devinfo,
			devinfo->classcode, devinfo->vendid, devinfo->devid));

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	/* setup the basic PCI properties for this device */
	pci_load_reg_and_name_props(e, pkg, devinfo);

	/* set the type of this device */
	ret = prop_set_str(pkg->props, "device_type", CSTR, "usb", CSTR);

	if (ret == NO_ERROR)
		ret = init_entries(e, pkg->dict, methods);

	/* allocate space and stash a copy of our devinfo for our methods */
	newinfo = (Pci_device_info*)malloc(sizeof *newinfo);

	if (newinfo == NULL)
		return E_OUT_OF_MEMORY;

	*newinfo = *devinfo;					/* copy it */
	newinfo->self = (struct pci_self*)malloc(size);

	if (newinfo->self == NULL)
		return E_OUT_OF_MEMORY;

	memset(newinfo->self, 0, size);

	pkg->self = (struct pself*)newinfo;		/* and save it */

	/* create standard OpenFirmware USB host controller properties */
	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "#address-cells", CSTR, 1);

	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "#size-cells", CSTR, 0);

	return ret;
}

Retcode
usb_setup(Environ *e, USB_self *self, int devaddr, int endpoint,
		int request_type, int request, int valhi, int vallo,
		int index, int length, Byte *data)
{
	usb_device_request_t req;

	memset(&req, 0, sizeof req);
	req.request_type = request_type;
	req.request = request;
	USETW2(req.value, valhi, vallo);
	USETW(req.index, index);
	USETW(req.length, length);
	return self->root->device_request(e, self, USB_SETUP, devaddr, endpoint,
			&req, data, length);
}

Retcode
usb_input(Environ *e, USB_self *self, int devaddr, int endpoint,
		int length, Byte *data)
{
	return self->root->device_request(e, self, USB_INPUT, devaddr, endpoint,
			NULL, data, length);
}

Retcode
usb_output(Environ *e, USB_self *self, int devaddr, int endpoint,
		int length, Byte *data)
{
	return self->root->device_request(e, self, USB_OUTPUT, devaddr, endpoint,
			NULL, data, length);
}

Retcode
usb_add_compatible(Environ *e, Byte **paddr, Int *plen, Byte *str)
{
	Retcode ret;

	DPRINTF(("usb_add_compatible: paddr=%#x plen=%d str=\"%s\"\n",
			*paddr, *plen, str));

	if (*paddr)
	{
		IFCKSP(e, 0, 2);
		PUSHP(e, *paddr);
		PUSH(e, *plen);
	}

	IFCKSP(e, 0, 2);
	PUSHP(e, str);
	PUSH(e, strlen(str));
	ret = execute_word(e, "encode-string");

	if (ret != NO_ERROR)
		return ret;

	if (*paddr)
	{
		ret = execute_word(e, "encode+");

		if (ret != NO_ERROR)
			return ret;
	}

	POP(e, *plen);
	POPT(e, *paddr, Byte*);

	return NO_ERROR;
}

char *
usb_device_name(int class, int subclass, int protocol)
{
	char *name = NULL;

	switch (class)
	{
	case UCLASS_HUB:
		name = "hub";
		break;

	case UCLASS_COMM:
		name = "communications";
		break;

	case UCLASS_MASS:
		/* non-standard - return something better than just "device" */
		name = "storage";
		break;
	}

	return NULL;
}

Retcode
usb_build_device_node(Environ *e, Package *parent, USB_self *self,
		usb_device_descriptor_t *dev, usb_config_descriptor_t *cfg,
		Package **retpkg)
{
	Retcode ret = NO_ERROR;
	Package *pkg;
	Byte *name;
	Byte *paddr = NULL;
	Int plen = 0;


	name = usb_device_name(dev->device_class, dev->device_subclass,
			dev->device_protocol);

	if (name == NULL)
		name = "device";

	pkg = new_pkg_name(parent, name);

	if (pkg == NULL)
		return E_OUT_OF_MEMORY;

	pkg->self = (struct pself*)malloc(sizeof *self);

	if (pkg->self == NULL)
		return E_OUT_OF_MEMORY;

	memcpy(pkg->self, self, sizeof *self);


	/* create standard OpenFirmware USB device node properties */
	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "#address-cells", CSTR, 2);

	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "#size-cells", CSTR, 0);

	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "reg", CSTR, self->port);

	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "assigned-address", CSTR, self->addr);

	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "configuration#", CSTR, self->config);

	if (ret == NO_ERROR && self->lowspeed)
		add_property(pkg->props, "low-speed", CSTR, NULL, 0);

	if (ret == NO_ERROR)
		ret = init_entries(e, pkg->dict, usb_device_methods);


	/* finally create the "compatible" property - the order is important */
	if (ret == NO_ERROR && dev->num_configurations > 1)			/* 1 */
		ret = usb_add_compatible(e, &paddr, &plen,
				rbprintf("usb%x,%x.%x.config%x", UGETW(dev->id_vendor),
						UGETW(dev->id_product), UGETW(dev->device),
						self->config));

	if (ret == NO_ERROR)										/* 2 */
		ret = usb_add_compatible(e, &paddr, &plen,
				rbprintf("usb%x,%x.%x", UGETW(dev->id_vendor),
						UGETW(dev->id_product), UGETW(dev->device)));

	if (ret == NO_ERROR && dev->num_configurations > 1)			/* 3 */
		ret = usb_add_compatible(e, &paddr, &plen,
				rbprintf("usb%x,%x.config%x", UGETW(dev->id_vendor),
						UGETW(dev->id_product), self->config));

	if (ret == NO_ERROR)										/* 4 */
		ret = usb_add_compatible(e, &paddr, &plen,
				rbprintf("usb%x,%x", UGETW(dev->id_vendor),
						UGETW(dev->id_product)));

	if (dev->device_class != 0)
	{
		if (ret == NO_ERROR)									/* 5 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usb%x,class%x.%x.%x", UGETW(dev->id_vendor),
							dev->device_class, dev->device_subclass,
							dev->device_protocol));

		if (ret == NO_ERROR)									/* 6 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usb%x,class%x.%x", UGETW(dev->id_vendor),
							dev->device_class, dev->device_subclass));

		if (ret == NO_ERROR)									/* 7 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usb%x,class%x", UGETW(dev->id_vendor),
							dev->device_class));

		if (ret == NO_ERROR)									/* 8 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usb,class%x.%x.%x",
							dev->device_class, dev->device_subclass,
							dev->device_protocol));

		if (ret == NO_ERROR)									/* 9 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usb,class%x.%x", dev->device_class,
							dev->device_subclass));

		if (ret == NO_ERROR)									/* 10 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usb,class%x", dev->device_class));
	}

	if (ret == NO_ERROR)										/* 11 */
		ret = usb_add_compatible(e, &paddr, &plen, "usb,device");

	if (ret == NO_ERROR)
		ret = add_property(pkg->props, "compatible", CSTR, paddr, plen);

	*retpkg = pkg;
	return ret;
}

char *
usb_interface_name(int class, int subclass, int protocol)
{
	char *name = NULL;

	switch (class)
	{
	case UCLASS_AUDIO:
		switch (subclass)
		{
		case USUBCLASS_AUDIOCONTROL:
			name = "sound-control";
			break;

		case USUBCLASS_AUDIOSTREAM:
		default:
			name = "sound";
			break;

		case USUBCLASS_AUDIOMIDI:
			name = "midi";
			break;
		}

		break;

	case UCLASS_COMM:
		{
			static Byte *usb_comm_subclass[] =
			{
				NULL,
				"line",
				"modem",
				"telephone",
				"isdn",
				"isdn",
				"ethernet",
				"atm-network"
			};

			name = "control";

			if (subclass >= 0 && subclass <
					sizeof usb_comm_subclass / sizeof *usb_comm_subclass)
				name = usb_comm_subclass[subclass];
		}

		break;

	case UCLASS_HID:
		if (subclass == USUBCLASS_BOOT)
		{
			if (protocol == UPROTOCOL_KEYBOARD)
				name = "keyboard";
			else if (protocol == UPROTOCOL_MOUSE)
				name = "mouse";
			else
				name = "input";
		}
		else
			name = "input";

		break;

	case UCLASS_MONITOR:
		name = "display-control";
		break;

	case UCLASS_PHYSICAL:
		name = "physical";
		break;

#ifdef NOT_CORRECT
	case UCLASS_POWER:
		name = "power";
		break;
#endif

	case UCLASS_PRINTER:
		name = "printer";
		break;

	case UCLASS_MASS:
		{
			static Byte *usb_mass_subclass[] =
			{
				NULL,
				"solid-state",
				"cdrom",
				"tape",
				"floppy",
				"storage",
				"scsi",
			};
			
			name = "storage";

			if (subclass > 0 && subclass <
					sizeof usb_mass_subclass / sizeof *usb_mass_subclass)
				name = usb_mass_subclass[subclass];
		}

		break;

	case UCLASS_HUB:
		name = "hub";
		break;

	case UCLASS_DATA:
		name = "data";
		break;

	case UCLASS_FIRMWARE:
		name = "firmware";
		break;

	case UCLASS_SECURITY:
		name = "security";
		break;

	case UCLASS_APPL_SPEC:
		switch (subclass)
		{
		case USUBCLASS_APPL_FW:
			name = "firmware";
			break;

		case USUBCLASS_APPL_IRDA:
			name = "irda";
			break;
		}

	case UCLASS_VENDOR:
		/* vendor-specific - just return NULL for now */
		break;
	}

	return name;
}

extern Retcode usb_install_hub_driver(Environ *e, Package *pkg, USB_self *self,
		usb_device_descriptor_t *dev, usb_config_descriptor_t *cfg,
		usb_interface_descriptor_t *ifc);

extern Retcode usb_install_keyboard_driver(Environ *e, Package *pkg,
		USB_self *self,
		usb_device_descriptor_t *dev, usb_config_descriptor_t *cfg,
		usb_interface_descriptor_t *ifc);

Retcode
usb_install_driver(Environ *e, char *name, Package *pkg, USB_self *self,
		usb_device_descriptor_t *dev, usb_config_descriptor_t *cfg,
		usb_interface_descriptor_t *ifc)
{
	if (strcmp(name, "hub") == 0)
		return usb_install_hub_driver(e, pkg, self, dev, cfg, ifc);

	if (strcmp(name, "keyboard") == 0)
		return usb_install_keyboard_driver(e, pkg, self, dev, cfg, ifc);
	
	return NO_ERROR;
}

Retcode
usb_build_interface_node(Environ *e, Package *parent, USB_self *self,
		usb_device_descriptor_t *dev, usb_config_descriptor_t *cfg,
		usb_interface_descriptor_t *ifc)
{
	Retcode ret = NO_ERROR;
	Package *pkg;
	Byte *name = NULL;
	Byte *paddr;
	Int plen;

	name = usb_interface_name(ifc->interface_class, ifc->interface_subclass,
			ifc->interface_protocol);

	if (name == NULL)
		name = "interface";

	pkg = new_pkg_name(parent, name);

	if (pkg == NULL)
		return E_OUT_OF_MEMORY;

	pkg->self = (struct pself*)malloc(sizeof *self);

	if (pkg->self == NULL)
		return E_OUT_OF_MEMORY;

	memcpy(pkg->self, self, sizeof *self);


	/* create standard OpenFirmware USB device node properties */
	IFCKSP(e, 0, 2);
	PUSH(e, self->interface);
	ret = execute_word(e, "encode-int");

	if (ret != NO_ERROR)
		return ret;

	PUSH(e, self->config);
	ret = execute_word(e, "encode-int");

	if (ret != NO_ERROR)
		return ret;

	ret = execute_word(e, "encode+");

	if (ret != NO_ERROR)
		return ret;

	POP(e, plen);
	POPT(e, paddr, Byte*);
	ret = add_property(pkg->props, "reg", CSTR, paddr, plen);


	/* finally create the "compatible" property - the order is important */
	paddr = NULL;
	plen = 0;

	if (ret == NO_ERROR)										/* 1 */
		ret = usb_add_compatible(e, &paddr, &plen,
				rbprintf("usbif%x,%x.%x.config%x.%x", UGETW(dev->id_vendor),
						UGETW(dev->id_product), UGETW(dev->device),
						self->config, self->interface));

	if (ret == NO_ERROR)										/* 2 */
		ret = usb_add_compatible(e, &paddr, &plen,
				rbprintf("usbif%x,%x.config%x.%x", UGETW(dev->id_vendor),
						UGETW(dev->id_product), self->config,
						self->interface));

	if (ret == NO_ERROR && ifc->interface_class != 0)
	{
		if (ret == NO_ERROR)									/* 3 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usbif%x,class%x.%x.%x", UGETW(dev->id_vendor),
							ifc->interface_class, ifc->interface_subclass,
							ifc->interface_protocol));

		if (ret == NO_ERROR) 									/* 4 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usbif%x,class%x.%x", UGETW(dev->id_vendor),
							ifc->interface_class, ifc->interface_subclass));

		if (ret == NO_ERROR)									/* 5 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usbif%x,class%x", UGETW(dev->id_vendor),
							ifc->interface_class));

		if (ret == NO_ERROR)									/* 6 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usbif,class%x.%x.%x", ifc->interface_class,
							ifc->interface_subclass, ifc->interface_protocol));

		if (ret == NO_ERROR)									/* 7 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usbif,class%x.%x", ifc->interface_class,
							ifc->interface_subclass));

		if (ret == NO_ERROR)									/* 8 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usbif,class%x", ifc->interface_class));
	}

	if (ret == NO_ERROR)
		ret = add_property(pkg->props, "compatible", CSTR, paddr, plen);


	/* finally install any builtin driver for anything we recognize */
	if (ret == NO_ERROR)
		ret = usb_install_driver(e, name, pkg, self, dev, cfg, ifc);

	return ret;
}

Retcode
usb_build_combined_node(Environ *e, Package *parent, USB_self *self,
		usb_device_descriptor_t *dev, usb_config_descriptor_t *cfg,
		usb_interface_descriptor_t *ifc)
{
	Retcode ret = NO_ERROR;
	Package *pkg;
	Byte *name;
	Byte *paddr = NULL;
	Int plen = 0;

	name = usb_interface_name(ifc->interface_class, ifc->interface_subclass,
			ifc->interface_protocol);

	if (name == NULL)
		name = usb_device_name(dev->device_class, dev->device_subclass,
				dev->device_protocol);

	if (name == NULL)
		name = "device";

	pkg = new_pkg_name(parent, name);

	if (pkg == NULL)
		return E_OUT_OF_MEMORY;

	pkg->self = (struct pself*)malloc(sizeof *self);

	if (pkg->self == NULL)
		return E_OUT_OF_MEMORY;

	memcpy(pkg->self, self, sizeof *self);


	/* create standard OpenFirmware USB device node properties */
	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "reg", CSTR, self->port);

	DPRINTF(("usb_build_combined_node: port=%d\n", self->port));

	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "assigned-address", CSTR, self->addr);

	if (ret == NO_ERROR && self->lowspeed)
		add_property(pkg->props, "low-speed", CSTR, NULL, 0);


	/* finally create the "compatible" property - the order is important */
	if (ret == NO_ERROR && dev->num_configurations > 1)			/* 1 */
		ret = usb_add_compatible(e, &paddr, &plen,
				rbprintf("usb%x,%x.%x", UGETW(dev->id_vendor),
						UGETW(dev->id_product), UGETW(dev->device)));

	if (ret == NO_ERROR)										/* 2 */
		ret = usb_add_compatible(e, &paddr, &plen,
				rbprintf("usb%x,%x", UGETW(dev->id_vendor),
						UGETW(dev->id_product)));

	if (ret == NO_ERROR && dev->device_class != 0)
	{
		if (ret == NO_ERROR)									/* 3 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usb%x,class%x.%x.%x", UGETW(dev->id_vendor),
							dev->device_class, dev->device_subclass,
							dev->device_protocol));

		if (ret == NO_ERROR)									/* 4 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usb%x,class%x.%x", UGETW(dev->id_vendor),
							dev->device_class, dev->device_subclass));

		if (ret == NO_ERROR)									/* 5 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usb%x,class%x", UGETW(dev->id_vendor),
							dev->device_class));

		if (ret == NO_ERROR)									/* 6 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usb,class%x.%x.%x", dev->device_class,
							dev->device_subclass, dev->device_protocol));

		if (ret == NO_ERROR)									/* 7 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usb,class%x.%x", dev->device_class,
							dev->device_subclass));

		if (ret == NO_ERROR)									/* 8 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usb,class%x", dev->device_class));
	}

	if (ret == NO_ERROR && ifc->interface_class != 0)
	{
		if (ret == NO_ERROR)									/* 9 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usbif%x,class%x.%x.%x", UGETW(dev->id_vendor),
							dev->device_class, dev->device_subclass,
							dev->device_protocol));

		if (ret == NO_ERROR)									/* 10 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usbif%x,class%x.%x", UGETW(dev->id_vendor),
							dev->device_class, dev->device_subclass));

		if (ret == NO_ERROR)									/* 11 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usbif%x,class%x", UGETW(dev->id_vendor),
							dev->device_class));

		if (ret == NO_ERROR)									/* 12 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usbif,class%x.%x.%x", dev->device_class,
							dev->device_subclass, dev->device_protocol));

		if (ret == NO_ERROR)									/* 13 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usbif,class%x.%x", dev->device_class,
							dev->device_subclass));

		if (ret == NO_ERROR)									/* 14 */
			ret = usb_add_compatible(e, &paddr, &plen,
					rbprintf("usbif,class%x", dev->device_class));
	}

	if (ret == NO_ERROR)
		ret = add_property(pkg->props, "compatible", CSTR, paddr, plen);


	/* finally install any builtin driver for anything we recognize */
	if (ret == NO_ERROR)
		ret = usb_install_driver(e, name, pkg, self, dev, cfg, ifc);

	return ret;
}


/* assumes port is powered up and answering to default addr/endpoint
 */
Retcode
usb_probe_device(Environ *e, Package *pkg, USB_root *usb, Bool lowspeed,
		int port)
{
	USB_self self;	/* TODO */
	Retcode ret;
	usb_device_descriptor_t dev;
	usb_config_descriptor_t cfg;
	usb_descriptor_t *desc;
	usb_interface_descriptor_t *ifc;
	usb_endpoint_descriptor_t *epd;
	usb_hid_descriptor_t *hid;
	Byte *buf, *bp;
	int len, i, j;
	Bool combined = FALSE;

	memset(&dev, 0, sizeof dev);
	memset(&cfg, 0, sizeof cfg);

	memset(&self, 0, sizeof self);
	self.root = usb;
	self.port = port;
	self.lowspeed = lowspeed;
	self.max_pkt_size = USB_MIN_PACKET;
	DPRINTF(("usb_probe_device: usb=%#x port=%d lowspeed=%d\n",
			usb, port, lowspeed));


	/* first get the device descriptor */
	ret = usb_setup(e, &self, 0, 0,
			UT_READ_DEVICE, UR_GET_DESCRIPTOR, UDESC_DEVICE, 0, 0,
			USB_DEVICE_DESCRIPTOR_SIZE, (Byte*)&dev);

	if (ret != NO_ERROR)
		return ret;

	DPRINTF(("usb_probe_device: device-request ret=%s (%d)\n",
			err2str(ret), ret));
	DPRINTF(("usb_probe_device: device: length=%d/%d type=%d spec=%x\n"
			"\tclass=%#x subclass=%#x protocol=%#x maxpkt=%d\n"
			"\tidvendor=%#x idproduct=%#x device=%x manufacturer=%d\n"
			"\tproduct=%d serial=%d num_configs=%d\n",
			dev.length, sizeof dev,
			dev.descriptor_type,
			UGETW(dev.usb_spec),
			dev.device_class,
			dev.device_subclass,
			dev.device_protocol,
			dev.max_packet_size,
			UGETW(dev.id_vendor),
			UGETW(dev.id_product),
			UGETW(dev.device),
			dev.manufacturer,
			dev.product,
			dev.serial_number,
			dev.num_configurations));

	if (dev.max_packet_size >= USB_MIN_PACKET &&
			dev.max_packet_size <= USB_MAX_PACKET)		/* sanity check */
		self.max_pkt_size = dev.max_packet_size;


	/* set the device address to the next available for this root-hub */
	if (usb->nextaddr >= USB_MAX_DEVICES)
	{
		cprintf(e, "usb_probe_device: too many USB devices?!?\n");
		return E_ABORT;
	}

	self.addr = usb->nextaddr++;
	ret = usb_setup(e, &self, 0, USB_CONTROL_ENDPOINT,
			UT_WRITE_DEVICE, UR_SET_ADDRESS, 0, self.addr, 0, 0, NULL);

	if (ret != NO_ERROR)
		return ret;


	/* get config descriptor 0 */
	ret = usb_setup(e, &self, self.addr, USB_CONTROL_ENDPOINT,
			UT_READ_DEVICE, UR_GET_DESCRIPTOR, UDESC_CONFIG, 0, 0,
			USB_CONFIG_DESCRIPTOR_SIZE, (Byte*)&cfg);

	if (ret != NO_ERROR)
		return ret;

	DPRINTF(("usb_probe_device: config0: length=%d descriptor_type=%d\n"
			"\ttotal_length=%d num_interface=%d configuration_value=%d\n"
			"\tconfiguration=%d attributes=%#x max_power=%d\n",
			cfg.length,
			cfg.descriptor_type,
			UGETW(cfg.total_length),
			cfg.num_interface,
			cfg.configuration_value,
			cfg.configuration,
			cfg.attributes,
			cfg.max_power));
	self.config = cfg.configuration_value;

	/* get all config info */
	len = UGETW(cfg.total_length);
	buf = (Byte*)malloc(len + 1);

	if (buf == NULL)
		return E_OUT_OF_MEMORY;

	/* get config descriptor 0 */
	ret = usb_setup(e, &self, self.addr, USB_CONTROL_ENDPOINT,
			UT_READ_DEVICE, UR_GET_DESCRIPTOR, UDESC_CONFIG, 0, 0,
			len, (Byte*)buf);

	if (ret != NO_ERROR)
		goto fail;

	/* see if we should create a combined node or not */
	if ((dev.device_class == UCLASS_UNSPEC ||
					dev.device_class == UCLASS_HUB) &&
			dev.num_configurations == 1 &&
			cfg.num_interface == 1)
		combined = TRUE;
	else
		ret = usb_build_device_node(e, pkg, &self, &dev, &cfg, &pkg);

	if (ret != NO_ERROR)
		goto fail;

	/* parse interface/endpoint/whatever descriptors */
	bp = buf + USB_CONFIG_DESCRIPTOR_SIZE;

	for (i = USB_CONFIG_DESCRIPTOR_SIZE; i < len; i += desc->length)
	{
		desc = (usb_descriptor_t*)bp;
		DPRINTF(("    descriptor: length=%d descriptor_type=%d: ",
				desc->length, desc->descriptor_type));

		switch (desc->descriptor_type)
		{
		case UDESC_INTERFACE:
			ifc = (usb_interface_descriptor_t*)desc;
			DPRINTF(("interface\n"
				"\tinterface_number=%d alternate_setting=%d num_endpoints=%d\n"
				"\ti_class=%#x i_subclass=%#x i_protocol=%#x interface=%d\n",
					ifc->interface_number,
					ifc->alternate_setting,
					ifc->num_endpoints,
					ifc->interface_class,
					ifc->interface_subclass,
					ifc->interface_protocol,
					ifc->interface));

			self.interface = ifc->interface_number;

			if (combined)
				ret = usb_build_combined_node(e, pkg, &self, &dev, &cfg, ifc);
			else
				ret = usb_build_interface_node(e, pkg, &self, &dev, &cfg, ifc);

			if (ret != NO_ERROR)
				goto fail;

			break;

		case UDESC_ENDPOINT:
			epd = (usb_endpoint_descriptor_t*)desc;
			DPRINTF(("endpoint\n"
				"\tendpoint_address=%#x attributes=%#x\n"
				"\tmax_pkt_size=%d interval=%d\n",
					epd->endpoint_address,
					epd->attributes,
					UGETW(epd->max_packet_size),
					epd->interval));
			break;

		case 0:		/* hub class? */
			DPRINTF(("hub-class?\n"));
			break;

		case UDESC_HID:		/* human interface device (usually mouse or kbd) */
			hid = (usb_hid_descriptor_t*)desc;
			DPRINTF(("hid\n"
					"\thid=%#x country_code=%#x num_descriptors=%d\n",
					UGETW(hid->hid),
					hid->country_code,
					hid->num_descriptors));
			
			for (j = 0; j < hid->num_descriptors; j++)
			{
				DPRINTF(("\tdescriptor[%d] type=%#x length=%d\n",
						j, hid->descrs[j].descriptor_type,
						UGETW(hid->descrs[j].descriptor_length) ));
			}

			break;

		default:
			DPRINTF(("unknown\n"));
			break;
		}

		bp += desc->length;
	}

	// TODO - create SF devs based on device/config/interface info
	// TODO - configure hubs, keyboards, and eventually mass-storage devices

fail:
	free(buf);
	return ret;
}
