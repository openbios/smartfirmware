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

/* USB routines for hubs.
 */

#include "defs.h"
#include "pci.h"
#include "usb.h"


const Initentry usb_hub_methods[] =
{
	{ "open",			f_usb_open,				INVALID_FCODE },
	{ "close",			f_usb_close,			INVALID_FCODE },
	{ "decode-unit",	f_usb_port_decode_unit,	INVALID_FCODE },
	{ "encode-unit",	f_usb_port_encode_unit,	INVALID_FCODE },
	{ NULL,				NULL },
};


Retcode
usb_install_hub_driver(Environ *e, Package *pkg, USB_self *self,
		usb_device_descriptor_t *dev, usb_config_descriptor_t *cfg,
		usb_interface_descriptor_t *ifc)
{
	usb_hub_descriptor_t hub;
	usb_port_status_t port;
	int i, num;
	uByte *removable, *power;
	Retcode ret = NO_ERROR;

	DPRINTF(("usb_install_hub_driver: addr=%d interface=%d\n",
			self->addr, self->interface));
	memset(&hub, 0, sizeof hub);
	memset(&port, 0, sizeof port);

	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "#address-cells", CSTR, 1);

	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "#size-cells", CSTR, 0);

	if (ret == NO_ERROR)
		ret = init_entries(e, pkg->dict, usb_hub_methods);

	if (ret != NO_ERROR)
		return ret;


	/* set config descriptor to allow powering on hub ports */
	ret = usb_setup(e, self, self->addr, USB_CONTROL_ENDPOINT, UT_WRITE_DEVICE,
			UR_SET_CONFIG, 0, cfg->configuration_value, 0, 0, NULL);

	if (ret != NO_ERROR)
		return ret;

	DPRINTF(("usb_install_hub_driver: hub configuration_value=%d\n",
			cfg->configuration_value));

	ret = usb_setup(e, self, self->addr, USB_CONTROL_ENDPOINT,
			UT_READ_CLASS_DEVICE, UR_GET_DESCRIPTOR, UDESC_HUB, 0, 0,
			1, (Byte*)&hub);

	if (ret == NO_ERROR)
		ret = usb_setup(e, self, self->addr, USB_CONTROL_ENDPOINT,
				UT_READ_CLASS_DEVICE, UR_GET_DESCRIPTOR, UDESC_HUB, 0, 0,
				hub.desc_length, (Byte*)&hub);

	if (ret != NO_ERROR)
		return ret;

	if (hub.nbr_ports < 8)
	{
		num = 1;
		removable = hub.u.p8.device_removable;
		power = hub.u.p8.port_power_ctrl_mask;
	}
	else if (hub.nbr_ports < 16)
	{
		num = 2;
		removable = hub.u.p16.device_removable;
		power = hub.u.p16.port_power_ctrl_mask;
	}
	else if (hub.nbr_ports < 24)
	{
		num = 3;
		removable = hub.u.p24.device_removable;
		power = hub.u.p24.port_power_ctrl_mask;
	}
	else	/* hub.nbr_ports < 32 */
	{
		num = 4;
		removable = hub.u.p32.device_removable;
		power = hub.u.p32.port_power_ctrl_mask;
	}

	DPRINTF(("usb_install_hub_driver: desc_length=%d descriptor_type=%#x\n"
			"\tnbr_ports=%d hub_characteristics=%#x pwr_on2pwr_good=%d\n"
			"\thub_contr_current=%d\n",
			hub.desc_length,
			hub.descriptor_type,
			hub.nbr_ports,
			UGETW(hub.hub_characteristics),
			hub.pwr_on2pwr_good,
			hub.hub_contr_current));

#ifdef DEBUG
	for (i = 0; i < num; i++)
		dprintf("\tdevice_removable[%d]=%#x port_power_ctrl_mask[%d]=%#x\n",
			i, removable[i], i, power[i]);
#endif	/* DEBUG */

	/* probe each port on the hub */
	for (i = 1; ret == NO_ERROR && i <= hub.nbr_ports; i++)
	{
		/* get status from this port */
		ret = usb_setup(e, self, self->addr, USB_CONTROL_ENDPOINT,
				UT_READ_CLASS_OTHER, UR_GET_STATUS, 0, 0, i,
				USB_PORT_DESCRIPTOR_SIZE, (Byte*)&port);

		if (ret != NO_ERROR)
			break;

		DPRINTF(("usb_probe_hub(%d): status=%#x change=%#x\n",
				i, UGETW(port.port_status), UGETW(port.port_change)));

		/* power on this port if each port is independently controlled */
		if ((UGETW(hub.hub_characteristics) & UHD_PWR_INDIVIDUAL) &&
				!(UGETW(port.port_status) & UPS_PORT_POWER))
		{
			DPRINTF(("usb_probe_hub(%d): powering up...\n", i));
			ret = usb_setup(e, self, self->addr, USB_CONTROL_ENDPOINT,
					UT_WRITE_CLASS_OTHER, UR_SET_FEATURE,
					0, UHF_PORT_POWER, i, 0, NULL);

			if (ret != NO_ERROR)
				break;

			u_sleep(hub.pwr_on2pwr_good * 2000);
		}

		ret = usb_setup(e, self, self->addr, USB_CONTROL_ENDPOINT,
				UT_READ_CLASS_OTHER, UR_GET_STATUS, 0, 0, i,
				USB_PORT_DESCRIPTOR_SIZE, (Byte*)&port);

		if (ret != NO_ERROR)
			break;

		if (!(UGETW(port.port_status) & UPS_CURRENT_CONNECT_STATUS))
		{
			DPRINTF(("usb_probe_hub(%d): no device\n", i));

			/* turn off power if we turned it on above */
			if (UGETW(hub.hub_characteristics) & UHD_PWR_INDIVIDUAL)
				ret = usb_setup(e, self, self->addr, USB_CONTROL_ENDPOINT,
						UT_WRITE_CLASS_OTHER, UR_CLEAR_FEATURE,
						0, UHF_PORT_POWER, i, 0, NULL);

			continue;
		}

		DPRINTF(("usb_probe_hub(%d): got one!\n", i));

		/* enable the port */
		ret = usb_setup(e, self, self->addr, USB_CONTROL_ENDPOINT,
				UT_WRITE_CLASS_OTHER, UR_SET_FEATURE,
				0, UHF_PORT_ENABLE, i, 0, NULL);

		if (ret != NO_ERROR)
			break;

		u_sleep(10);

		/* send clear enable flag */
		ret = usb_setup(e, self, self->addr, USB_CONTROL_ENDPOINT,
				UT_WRITE_CLASS_OTHER, UR_SET_FEATURE,
				0, UHF_C_PORT_ENABLE, i, 0, NULL);

		if (ret != NO_ERROR)
			break;

		/* reset the port */
		ret = usb_setup(e, self, self->addr, USB_CONTROL_ENDPOINT,
				UT_WRITE_CLASS_OTHER, UR_SET_FEATURE,
				0, UHF_PORT_RESET, i, 0, NULL);

		if (ret != NO_ERROR)
			break;

		do
		{
			u_sleep(USB_RESET_DELAY);
			ret = usb_setup(e, self, self->addr, USB_CONTROL_ENDPOINT,
					UT_READ_CLASS_OTHER, UR_GET_STATUS, 0, 0, i,
					USB_PORT_DESCRIPTOR_SIZE, (Byte*)&port);

			if (ret != NO_ERROR)
				break;
		} while (ret == NO_ERROR &&
				!(UGETW(port.port_change) & UPS_C_PORT_RESET));

		/* send clear reset */
		ret = usb_setup(e, self, self->addr, USB_CONTROL_ENDPOINT,
				UT_WRITE_CLASS_OTHER, UR_SET_FEATURE,
				0, UHF_C_PORT_RESET, i, 0, NULL);

		if (ret != NO_ERROR)
			break;

		/* send clear enable flag if needed */
		if (UGETW(port.port_change) & UPS_C_PORT_ENABLED)
			ret = usb_setup(e, self, self->addr, USB_CONTROL_ENDPOINT,
					UT_WRITE_CLASS_OTHER, UR_SET_FEATURE,
					0, UHF_C_PORT_ENABLE, i, 0, NULL);

		if (ret != NO_ERROR)
			break;


		/* finally probe the device on the port */
		ret = usb_probe_device(e, pkg, self->root,
				(UGETW(port.port_status) & UPS_LOW_SPEED) ? TRUE : FALSE, i);
	}

	return NO_ERROR;
}
