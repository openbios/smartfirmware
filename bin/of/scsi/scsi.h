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

/* (c) Copyright 1996-1997 by CodeGen, Inc.  All Rights Reserved. */

/* Copied and modified from FreeBSD scsi_all.h - original copyright follows: */

/*
 * Largely written by Julian Elischer (julian@tfs.com)
 * for TRW Financial Systems.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 * Ported to run under 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 */

/*
 * SCSI general  interface description
 */

#ifndef	_SCSI_H
#define _SCSI_H


/*
 * SCSI command format
 */

/*
 * Define some bits that are in ALL (or a lot of) scsi commands
 */
#define SCSI_CTL_LINK		0x01
#define SCSI_CTL_FLAG		0x02
#define SCSI_CTL_VENDOR		0xC0
#define	SCSI_CMD_LUN		0xA0	/* these two should not be needed */
#define	SCSI_CMD_LUN_SHIFT	5		/* LUN in the cmd is no longer SCSI */


struct scsi_generic
{
	uByte	opcode;
	uByte	bytes[11];
};

struct scsi_test_unit_ready
{
	uByte	op_code;
	uByte	byte2;
	uByte	unused[3];
	uByte	control;
};

struct scsi_send_diag
{
	uByte	op_code;
	uByte	byte2;
#define	SSD_UOL			0x01
#define	SSD_DOL			0x02
#define	SSD_SELFTEST	0x04
#define	SSD_PF			0x10
	uByte	unused[1];
	uByte	paramlen[2];
	uByte	control;
};

struct scsi_sense
{
	uByte	op_code;
	uByte	byte2;
	uByte	unused[2];
	uByte	length;
	uByte	control;
};

struct scsi_inquiry
{
	uByte	op_code;
	uByte	byte2;
	uByte	unused[2];
	uByte	length;
	uByte	control;
};

struct scsi_mode_sense
{
	uByte	op_code;
	uByte	byte2;
#define	SMS_DBD						0x08
	uByte	page;
#define	SMS_PAGE_CODE 				0x3F
#define	SMS_PAGE_CTRL 				0xC0
#define	SMS_PAGE_CTRL_CURRENT 		0x00
#define	SMS_PAGE_CTRL_CHANGEABLE 	0x40
#define	SMS_PAGE_CTRL_DEFAULT 		0x80
#define	SMS_PAGE_CTRL_SAVED 		0xC0
	uByte	unused;
	uByte	length;
	uByte	control;
};

struct scsi_mode_sense_big
{
	uByte	op_code;
	uByte	byte2; 		/* same bits as small version */
	uByte	page; 		/* same bits as small version */
	uByte	unused[4];
	uByte	length[2];
	uByte	control;
};

struct scsi_mode_select
{
	uByte	op_code;
	uByte	byte2;
#define	SMS_SP			0x01
#define	SMS_PF			0x10
	uByte	unused[2];
	uByte	length;
	uByte	control;
};

struct scsi_mode_select_big
{
	uByte	op_code;
	uByte	byte2;		/* same bits as small version */
	uByte	unused[5];
	uByte	length[2];
	uByte	control;
};

struct scsi_reserve
{
	uByte	op_code;
	uByte	byte2;
	uByte	unused[2];
	uByte	length;
	uByte	control;
};

struct scsi_release
{
	uByte	op_code;
	uByte	byte2;
	uByte	unused[2];
	uByte	length;
	uByte	control;
};

struct scsi_prevent
{
	uByte	op_code;
	uByte	byte2;
	uByte	unused[2];
	uByte	how;
	uByte	control;
};
#define	PR_PREVENT 0x01
#define PR_ALLOW   0x00

struct scsi_changedef
{
	uByte	op_code;
	uByte	byte2;
	uByte	unused1;
	uByte	how;
	uByte	unused[4];
	uByte	datalen;
	uByte	control;
};
#define SC_SCSI_1 0x01
#define SC_SCSI_2 0x03

/*
 * Opcodes
 */

#define	TEST_UNIT_READY		0x00
#define REQUEST_SENSE		0x03
#define	REASSIGN_BLOCKS		0x07
#define	READ_COMMAND		0x08
#define WRITE_COMMAND		0x0a
#define INQUIRY				0x12
#define MODE_SELECT			0x15
#define MODE_SENSE			0x1a
#define START_STOP			0x1b
#define PREVENT_ALLOW		0x1e
#define RESERVE      		0x16
#define RELEASE      		0x17
#define PREVENT_ALLOW		0x1e
#define	READ_CAPACITY		0x25
#define	READ_BIG			0x28
#define WRITE_BIG			0x2a
#define POSITION_TO_ELEMENT	0x2b
#define	CHANGE_DEFINITION	0x40
#define	MODE_SENSE_BIG		0x54
#define	MODE_SELECT_BIG		0x55
#define MOVE_MEDIUM     	0xa5
#define READ_ELEMENT_STATUS	0xb8



/*
 * peripheral device type
 */
#define T_DIRECT		0
#define T_SEQUENTIAL	1
#define T_PRINTER		2
#define T_PROCESSOR		3
#define T_WORM			4
#define T_READONLY		5
#define T_SCANNER 		6
#define T_OPTICAL 		7
#define T_CHANGER		8
#define T_COMM			9
#define T_ASC0     		10
#define T_ASC1     		11
#define T_TARGET   		12
#define T_UNKNOWN  		13
#define T_NTYPES   		14

#define T_NODEVICE	0x1F

#define T_REMOV		1
#define	T_FIXED		0

struct scsi_inquiry_data
{
	uByte	device;
#define	SID_TYPE			0x1F
#define	SID_QUAL			0xE0
#define	SID_QUAL_LU_OK		0x00
#define	SID_QUAL_LU_OFFLINE	0x20
#define	SID_QUAL_RSVD		0x40
#define	SID_QUAL_BAD_LU		0x60
	uByte	dev_qual2;
#define	SID_QUAL2			0x7F
#define	SID_REMOVABLE		0x80
	uByte	version;
#define SID_ANSII			0x07
#define SID_ECMA			0x38
#define SID_ISO				0xC0
	uByte	response_format;
	uByte	additional_length;
	uByte	unused[2];
	uByte	flags;
#define	SID_SftRe			0x01
#define	SID_CmdQue			0x02
#define	SID_Linked			0x08
#define	SID_Sync			0x10
#define	SID_WBus16			0x20
#define	SID_WBus32			0x40
#define	SID_RelAdr			0x80
	Byte	vendor[8];
	Byte	product[16];
	Byte	revision[4];
	uByte	extra[8];
};


struct	scsi_sense_data
{
/* 1*/	uByte	error_code;	/* same bits as new version */
	union
	{
		struct
		{
/* 2*/			uByte	blockhi;
/* 3*/			uByte	blockmed;
/* 4*/			uByte	blocklow;
		} unextended;
		struct
		{
/* 2*/			uByte	segment;
/* 3*/			uByte	flags;	/* same bits as new version */
/* 7*/			uByte	info[4];
/* 8*/			uByte	extra_len;
			/* allocate enough room to hold new stuff
			( by increasing 16 to 24 below) */
/*32*/			uByte	extra_bytes[24];
		} extended;
	}ext;
};	/* total of 32 bytes */

struct scsi_sense_extended
{
/* 2*/		uByte	segment;
/* 3*/		uByte	flags;
#define	SSD_KEY				0x0F
#define	SSD_ILI				0x20
#define	SSD_EOM				0x40
#define	SSD_FILEMARK		0x80
#define SSD_KEY_NO_SENSE		0x0
#define SSD_KEY_RECOVERED_ERROR	0x1
#define SSD_KEY_NOT_READY		0x2
#define SSD_KEY_MEDIUM_ERROR	0x3
#define SSD_KEY_HARDWARE_ERROR	0x4
#define SSD_KEY_ILLEGAL_REQUEST	0x5
#define SSD_KEY_UNIT_ATTENTION	0x6
#define SSD_KEY_DATA_PROTECT	0x7
#define SSD_KEY_BLANK_CHECK		0x8
#define SSD_KEY_VENDOR_SPECIFIC	0x9
#define SSD_KEY_COPY_ABORTED	0xA
#define SSD_KEY_ABORTED_COMMAND	0xB
#define SSD_KEY_EQUAL			0xC
#define SSD_KEY_VOLUME_OVERFLOW	0xD
#define SSD_KEY_MISCOMPARE		0xE
#define SSD_KEY_RESERVED		0xF
/* 7*/		uByte	info[4];
/* 8*/		uByte	extra_len;
/*12*/		uByte	cmd_spec_info[4];
/*13*/		uByte	add_sense_code;
/*14*/		uByte	add_sense_code_qual;
/*15*/		uByte	fru;
/*16*/		uByte	sense_key_spec_1;
#define	SSD_SCS_VALID		0x80
/*17*/		uByte	sense_key_spec_2;
/*18*/		uByte	sense_key_spec_3;
/*32*/		uByte	extra_bytes[14];
};

struct	scsi_sense_data_new
{
/* 1*/	uByte	error_code;
#define	SSD_ERRCODE			0x7F
#define	SSD_ERRCODE_VALID	0x80
	union
	{
		struct	/* this is deprecated, the standard says "DON'T"*/
		{
/* 2*/			uByte	blockhi;
/* 3*/			uByte	blockmed;
/* 4*/			uByte	blocklow;
		} unextended;

		struct scsi_sense_extended extended;
	} ext;
}; /* total of 32 bytes */

struct	blk_desc
{
	uByte	density;
	uByte	nblocks[3];
	uByte	reserved;
	uByte	blklen[3];
};

struct scsi_mode_header
{
	uByte	data_length;	/* Sense data length */
	uByte	medium_type;
	uByte	dev_spec;
	uByte	blk_desc_len;
};

struct scsi_mode_header_big
{
	uByte	data_length[2];	/* Sense data length */
	uByte	medium_type;
	uByte	dev_spec;
	uByte	unused[2];
	uByte	blk_desc_len[2];
};


/*
 * Status Byte
 */
#define	SCSI_OK				0x00
#define	SCSI_CHECK			0x02
#define	SCSI_BUSY			0x08
#define SCSI_INTERM			0x10
#define SCSI_QUEUE_FULL		0x28



/* stuff copied from FreeBSD's scsi_disk.h */

struct scsi_rw
{
	uByte	op_code;
	uByte	addr_2;		/* Most significant */
#define	SRW_TOPADDR	0x1F	/* only 5 bits here */
	uByte	addr_1;
	uByte	addr_0;		/* least significant */
	uByte	length;
	uByte	control;
};

struct scsi_rw_big
{
	uByte	op_code;
	uByte	byte2;
#define	SRWB_RELADDR	0x01
	uByte	addr_3;		/* Most significant */
	uByte	addr_2;
	uByte	addr_1;
	uByte	addr_0;		/* least significant */
	uByte	reserved;
	uByte	length1;
	uByte	length0;
	uByte	control;
};

struct scsi_read_capacity
{
	uByte	op_code;
	uByte	byte2;
	uByte	addr_3;	/* Most Significant */
	uByte	addr_2;
	uByte	addr_1;
	uByte	addr_0;	/* Least Significant */
	uByte	unused[3];
	uByte	control;
};

struct scsi_read_capacity_data
{
	uByte	addr_3;	/* Most Significant */
	uByte	addr_2;
	uByte	addr_1;
	uByte	addr_0;	/* Least Significant */
	uByte	length_3;	/* Most Significant */
	uByte	length_2;
	uByte	length_1;
	uByte	length_0;	/* Least Significant */
};

struct scsi_start_stop
{
	uByte	op_code;
	uByte	byte2;
	uByte	unused[2];
	uByte	how;
#define	SSS_START		0x01
#define	SSS_LOEJ		0x02
	uByte	control;
};


/* to aid installing builtin SCSI drivers */
struct SCSI_device
{
	char *name;
	char *type;
	Retcode (*install)(Environ *e, Package *pkg, struct SCSI_device *dev,
			Int id, Int lun);
	Int id;
	Int lun;
	struct SCSI_self *self;
};
typedef struct SCSI_device SCSI_device;

#define ESCSIDEV(f) extern Retcode f(Environ *e, Package *pkg, \
		SCSI_device *dev, Int id, Int lun)
#define SCSIDEV(f) ESCSIDEV(f); Retcode f(Environ *e, Package *pkg, \
		SCSI_device *dev, Int id, Int lun)

extern Retcode scsi_execute_cmd(Environ *e,
		Byte *buf, Int buflen, Int dir,
		Byte *cmd, Int cmdlen, Int *hwerr, Int *status);
extern Retcode scsi_retry_cmd(Environ *e, 
		Byte *buf, Int buflen, Int dir, Int retries,
		Byte *cmd, Int cmdlen, Int *hwerr, Int *status, Bool *gotsense,
		struct scsi_sense *sense_cmd, struct scsi_sense_data_new *sense_data);
extern Retcode scsi_probe(Environ *e, Package *pkg, Bool verbose);


#endif /*_SCSI_H*/
