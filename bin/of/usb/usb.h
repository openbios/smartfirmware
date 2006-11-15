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

/* Copied and modified from NetBSD's usb.h.  Original copyright follows: */

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Author: Lennart Augustsson <augustss@carlstedt.se>
 *         Carlstedt Research & Technology
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _USB_H_
#define _USB_H_

#define USB_MAX_DEVICES 128
#define USB_START_ADDR 0

#define USB_CONTROL_ENDPOINT 0
#define USB_MAX_ENDPOINTS 16

/*
 * The USB records contain some unaligned little-endian word
 * components.  The U[SG]ETW macros take care of both the alignment
 * and endian problem and should always be used to access 16 bit
 * values.
 */
typedef uByte uWord[2];

#define UGETW(w)		((w)[0] | ((w)[1] << 8))
#define USETW(w,v)		((w)[0] = (uByte)(v), (w)[1] = (uByte)((v) >> 8))
#define USETW2(w,h,l)	((w)[0] = (uByte)(l), (w)[1] = (uByte)(h))

typedef struct
{
	uByte		request_type;
	uByte		request;
	uWord		value;
	uWord		index;
	uWord		length;
} usb_device_request_t;

#define USB_DEVICE_REQUEST_SIZE	8

#define UT_WRITE		0x00
#define UT_READ			0x80
#define UT_STANDARD		0x00
#define UT_CLASS		0x20
#define UT_VENDOR		0x40
#define UT_DEVICE		0x00
#define UT_INTERFACE	0x01
#define UT_ENDPOINT		0x02
#define UT_OTHER		0x03

#define UT_READ_DEVICE			(UT_READ  | UT_STANDARD | UT_DEVICE)
#define UT_READ_INTERFACE		(UT_READ  | UT_STANDARD | UT_INTERFACE)
#define UT_READ_ENDPOINT		(UT_READ  | UT_STANDARD | UT_ENDPOINT)
#define UT_WRITE_DEVICE			(UT_WRITE | UT_STANDARD | UT_DEVICE)
#define UT_WRITE_INTERFACE		(UT_WRITE | UT_STANDARD | UT_INTERFACE)
#define UT_WRITE_ENDPOINT		(UT_WRITE | UT_STANDARD | UT_ENDPOINT)
#define UT_READ_CLASS_DEVICE	(UT_READ  | UT_CLASS | UT_DEVICE)
#define UT_READ_CLASS_INTERFACE	(UT_READ  | UT_CLASS | UT_INTERFACE)
#define UT_READ_CLASS_OTHER		(UT_READ  | UT_CLASS | UT_OTHER)
#define UT_WRITE_CLASS_DEVICE	(UT_WRITE | UT_CLASS | UT_DEVICE)
#define UT_WRITE_CLASS_INTERFACE (UT_WRITE | UT_CLASS | UT_INTERFACE)
#define UT_WRITE_CLASS_OTHER	(UT_WRITE | UT_CLASS | UT_OTHER)

/* Requests */
#define UR_GET_STATUS		0x00
#define UR_CLEAR_FEATURE	0x01
#define UR_SET_FEATURE		0x03
#define UR_SET_ADDRESS		0x05
#define UR_GET_DESCRIPTOR	0x06
#define  UDESC_HUB				0
#define  UDESC_DEVICE			1
#define  UDESC_CONFIG			2
#define  UDESC_STRING			3
#define  UDESC_INTERFACE		4
#define  UDESC_ENDPOINT			5
#define  UDESC_INDEX			7
#define UR_SET_DESCRIPTOR	0x07
#define UR_GET_CONFIG		0x08
#define UR_SET_CONFIG		0x09
#define UR_GET_INTERFACE	0x0a
#define UR_SET_INTERFACE	0x0b
#define UR_SYNCH_FRAME		0x0c

/* Feature numbers */
#define UF_ENDPOINT_STALL		0
#define UF_DEVICE_REMOTE_WAKEUP	1

#define USB_MIN_PACKET		8	/* minumum size of any packet */
#define USB_MAX_PACKET		64	/* maxumum size of any bulk/control packet */

typedef struct
{
	uByte		length;
	uByte		descriptor_type;
	uByte		descriptor_subtype;
} usb_descriptor_t;

typedef struct
{
	uByte		length;
	uByte		descriptor_type;
	uWord		usb_spec;
	uByte		device_class;
	uByte		device_subclass;
	uByte		device_protocol;
	uByte		max_packet_size;
	/* The fields below are not part of the initial descriptor. */
	uWord		id_vendor;
	uWord		id_product;
	uWord		device;
	uByte		manufacturer;
	uByte		product;
	uByte		serial_number;
	uByte		num_configurations;
} usb_device_descriptor_t;

#define USB_DEVICE_DESCRIPTOR_SIZE 18

typedef struct
{
	uByte		length;
	uByte		descriptor_type;
	uWord		total_length;
	uByte		num_interface;
	uByte		configuration_value;
	uByte		configuration;
	uByte		attributes;
		#define UC_BUS_POWERED		0x80
		#define UC_SELF_POWERED		0x40
		#define UC_REMOTE_WAKEUP	0x20
	uByte		max_power; /* max current in 2 mA units */
		#define UC_POWER_FACTOR 2
} usb_config_descriptor_t;

#define USB_CONFIG_DESCRIPTOR_SIZE 9

typedef struct
{
	uByte		length;
	uByte		descriptor_type;
	uByte		interface_number;
	uByte		alternate_setting;
	uByte		num_endpoints;
	uByte		interface_class;
	uByte		interface_subclass;
	uByte		interface_protocol;
	uByte		interface;
} usb_interface_descriptor_t;

#define USB_INTERFACE_DESCRIPTOR_SIZE 9

typedef struct
{
	uByte		length;
	uByte		descriptor_type;
	uByte		endpoint_address;
		#define UE_IN		0x80
		#define UE_OUT		0x00
		#define UE_ADDR		0x0f
		#define UE_GET_IN(a)	(((a) >> 7) & 1)
	uByte		attributes;
		#define UE_CONTROL		0x00
		#define UE_ISOCHRONOUS	0x01
		#define UE_BULK			0x02
		#define UE_INTERRUPT	0x03
		#define UE_XFERTYPE		0x03
	uWord		max_packet_size;
	uByte		interval;
} usb_endpoint_descriptor_t;

#define USB_ENDPOINT_DESCRIPTOR_SIZE 7

typedef struct
{
	uByte		length;
	uByte		descriptor_type;
	uWord		string[127];
} usb_string_descriptor_t;

#define USB_MAX_STRING_LEN 128

/* Hub specific request */
#define UR_GET_BUS_STATE	0x02

/* Hub features */
#define UHF_C_HUB_LOCAL_POWER	0
#define UHF_C_HUB_OVER_CURRENT	1
#define UHF_PORT_CONNECTION		0
#define UHF_PORT_ENABLE			1
#define UHF_PORT_SUSPEND		2
#define UHF_PORT_OVER_CURRENT	3
#define UHF_PORT_RESET			4
#define UHF_PORT_POWER			8
#define UHF_PORT_LOW_SPEED		9
#define UHF_C_PORT_CONNECTION	16
#define UHF_C_PORT_ENABLE		17
#define UHF_C_PORT_SUSPEND		18
#define UHF_C_PORT_OVER_CURRENT	19
#define UHF_C_PORT_RESET		20

typedef struct
{
	uByte		desc_length;
	uByte		descriptor_type;
	uByte		nbr_ports;
	uWord		hub_characteristics;
		#define UHD_PWR				0x03
		#define UHD_PWR_GANGED		0x00
		#define UHD_PWR_INDIVIDUAL	0x01
		#define UHD_PWR_NO_SWITCH	0x02
		#define UHD_COMPOUND		0x04
		#define UHD_OC				0x18
		#define UHD_OC_GLOBAL		0x00
		#define UHD_OC_INDIVIDUAL	0x08
		#define UHD_OC_NONE			0x10
	uByte		pwr_on2pwr_good;	/* delay in 2 ms units */
		#define UHD_PWRON_FACTOR 	2
	uByte		hub_contr_current;
	union
	{
		/* based on nbr_ports */
		struct
		{
			uByte		device_removable[1];
			uByte		port_power_ctrl_mask[1];
		} p8;
		struct
		{
			uByte		device_removable[2];
			uByte		port_power_ctrl_mask[2];
		} p16;
		struct
		{
			uByte		device_removable[3];
			uByte		port_power_ctrl_mask[3];
		} p24;
		struct
		{
			uByte		device_removable[4];
			uByte		port_power_ctrl_mask[4];
		} p32;
	} u;
} usb_hub_descriptor_t;

#define USB_HUB_DESCRIPTOR_MIN_SIZE 9
#define USB_HUB_DESCRIPTOR_SIZE 	15

typedef struct
{
	uWord		status;
		/* Device status flags */
		#define UDS_SELF_POWERED		0x0001
		#define UDS_REMOTE_WAKEUP		0x0002
} usb_status_t;

typedef struct
{
	uWord		hub_status;
		#define UHS_LOCAL_POWER			0x0001
		#define UHS_OVER_CURRENT		0x0002
	uWord		hub_change;
} usb_hub_status_t;

typedef struct
{
	uWord		port_status;
		#define UPS_CURRENT_CONNECT_STATUS	0x0001
		#define UPS_PORT_ENABLED			0x0002
		#define UPS_SUSPEND					0x0004
		#define UPS_OVERCURRENT_INDICATOR	0x0008
		#define UPS_RESET					0x0010
		#define UPS_PORT_POWER				0x0100
		#define UPS_LOW_SPEED				0x0200
	uWord		port_change;
		#define UPS_C_CONNECT_STATUS		0x0001
		#define UPS_C_PORT_ENABLED			0x0002
		#define UPS_C_SUSPEND				0x0004
		#define UPS_C_OVERCURRENT_INDICATOR	0x0008
		#define UPS_C_PORT_RESET			0x0010
} usb_port_status_t;

#define USB_PORT_DESCRIPTOR_SIZE 	4


#define UDESC_CS_DEVICE		0x21
#define UDESC_CS_CONFIG		0x22
#define UDESC_CS_STRING		0x23
#define UDESC_CS_INTERFACE	0x24
#define UDESC_CS_ENDPOINT	0x25

#define UDESC_CS_HUB		0x29

#define UCLASS_UNSPEC			0
#define UCLASS_AUDIO			1
#define  USUBCLASS_AUDIOCONTROL		1
#define  USUBCLASS_AUDIOSTREAM		2
#define  USUBCLASS_AUDIOMIDI		3
#define UCLASS_COMM				2
#define  USUBCLASS_DIRECT_LINE		1
#define  USUBCLASS_ABSTRACT			2
#define  USUBCLASS_TELEPHONE		3
#define  USUBCLASS_MULTICHANNEL		4
#define  USUBCLASS_CAPI				5
#define  USUBCLASS_ETHERNET			6
#define  USUBCLASS_ATM				7
#define UCLASS_HID				3
#define  USUBCLASS_BOOT		 		1
#define  UPROTOCOL_KEYBOARD	 		1
#define  UPROTOCOL_MOUSE	 		2
#define UCLASS_MONITOR			4
#define UCLASS_PHYSICAL			5
#define UCLASS_PRINTER			7
#define  USUBCLASS_PRINTER			1
#define  UPROTO_PRINTER_UNI			1
#define  UPROTO_PRINTER_BI			2
#define UCLASS_MASS				8
#define  USUBCLASS_MASS_SOLIDSTATE	0x01
#define  USUBCLASS_MASS_CDROM		0x02
#define  USUBCLASS_MASS_TAPE		0x03
#define  USUBCLASS_MASS_FLOPPY		0x04
#define  USUBCLASS_MASS_STORAGE		0x05
#define  USUBCLASS_MASS_SCSI		0x06
#define  UPROTOCOL_MASS_CBI			0x00
#define  UPROTOCOL_MASS_CBNOI		0x01
#define  UPROTOCOL_MASS_BULK		0x02	/*?*/
#define UCLASS_HUB				9
#define  USUBCLASS_HUB				1
#define UCLASS_DATA				0x0A
#define UCLASS_FIRMWARE			0x0C
#define UCLASS_SECURITY			0x0D
#define UCLASS_APPL_SPEC		0xfe
#define  USUBCLASS_APPL_FW			1
#define  USUBCLASS_APPL_IRDA		2
#define UCLASS_VENDOR			0xff


#define USB_HUB_MAX_DEPTH	 5

#define USB_PORT_RESET_DELAY	10  /* ms */
#define USB_PORT_POWERUP_DELAY	100 /* ms */
#define USB_POWER_SETTLE		100 /* ms */

#define USB_MIN_POWER			100 /* mA */
#define USB_MAX_POWER			500 /* mA */


#define USB_RESET_DELAY			100 /* ms XXX?*/
#define USB_RESUME_DELAY		10 /* ms XXX?*/


/* copied from NetBSD usbhid.h */
#define UR_GET_HID_DESCRIPTOR	0x06
#define  UDESC_HID			0x21
#define  UDESC_REPORT		0x22
#define  UDESC_PHYSICAL		0x23
#define UR_SET_HID_DESCRIPTOR	0x07
#define UR_GET_REPORT			0x01
#define UR_SET_REPORT			0x09
#define UR_GET_IDLE				0x02
#define UR_SET_IDLE				0x0a
#define UR_GET_PROTOCOL			0x03
#define UR_SET_PROTOCOL			0x0b

typedef struct usb_hid_descriptor
{
	uByte		length;
	uByte		descriptor_type;
	uWord		hid;
	uByte		country_code;
	uByte		num_descriptors;
	struct
	{
		uByte		descriptor_type;
		uWord		descriptor_length;
	} descrs[1];
} usb_hid_descriptor_t;

#define USB_HID_DESCRIPTOR_SIZE(n) (9+(n)*3)

/* Usage pages */
#define HUP_GENERIC_DESKTOP	0x0001
#define HUP_SIMULATION		0x0002
#define HUP_LEDS			0x0008
#define HUP_BUTTON			0x0009

/* Usages, generic desktop */
#define HUG_MOUSE		0x0002
#define HUG_X			0x0030
#define HUG_Y			0x0031
#define HUG_Z			0x0032
#define HUG_WHEEL		0x0038

#define HID_USAGE2(p,u) (((p) << 16) | u)

#define UHID_INPUT_REPORT	0x01
#define UHID_OUTPUT_REPORT	0x02
#define UHID_FEATURE_REPORT	0x03

/* Bits in the input/output/feature items */
#define HIO_CONST		0x001
#define HIO_VARIABLE	0x002
#define HIO_RELATIVE	0x004
#define HIO_WRAP		0x008
#define HIO_NONLINEAR	0x010
#define HIO_NOPREF		0x020
#define HIO_NULLSTATE	0x040
#define HIO_VOLATILE	0x080
#define HIO_BUFBYTES	0x100


/* SmartFirmware stuff: */

#define USB_INPUT		1
#define USB_OUTPUT		2
#define USB_SETUP		3

struct usb_self;

typedef struct usb_root
{
	Package *pkg;		/* pointer to usb root-hub node */
	int nextaddr;
	Retcode (*device_request)(Environ *e, struct usb_self *us,
			int pkt_type, int devaddr, int endpoint,
			usb_device_request_t *request, Byte *data, Int datalen);
} USB_root;

typedef struct usb_self
{
	USB_root *root;		/* pointer to table of root-hub stuff */
	struct usb_device_self *self;
	uByte port;
	uByte addr;
	uByte config;
	uByte interface;
	uByte endpoint;
	uByte max_pkt_size;
	uByte lowspeed;		/* bool */
} USB_self;

/* from usb.c: */
EC(f_usb_open);
EC(f_usb_close);
EC(f_usb_port_decode_unit);
EC(f_usb_port_encode_unit);
EC(f_usb_dev_decode_unit);
EC(f_usb_dev_encode_unit);

extern Retcode usb_setup(Environ *e, USB_self *self, int devaddr, int endpoint,
		int request_type, int request, int valhi, int vallo,
		int index, int length, Byte *data);
extern Retcode usb_input(Environ *e, USB_self *self, int devaddr, int endpoint,
		int length, Byte *data);
extern Retcode usb_output(Environ *e, USB_self *self, int devaddr, int endpoint,
		int length, Byte *data);

struct pci_device_info;
extern Retcode install_usb_root_driver(Environ *e, Package *pkg,
		struct pci_device_info *devinfo, const Initentry methods[], uInt size);

extern Retcode usb_probe_device(Environ *e, Package *pkg, USB_root *root,
		Bool lowspeed, int port);


#endif /* _USB_H_ */
