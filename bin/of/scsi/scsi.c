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

/* (c) Copyright 1996-1998 by CodeGen, Inc.  All Rights Reserved. */

/* Generic SCSI routines for use by C chip-specific drivers.
 */

#include "defs.h"
#include "scsi.h"


static Retcode
execute_cmd(Environ *e, Byte *buf, Int buflen, Int dir,
		Byte *cmd, Int cmdlen, Int *hwerr, Int *status)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Retcode ret;

	IFCKSP(e, 0, 5);
	PUSHP(e, buf);
	PUSH(e, buflen);
	PUSH(e, dir);
	PUSHP(e, cmd);
	PUSHP(e, cmdlen);
	ret = execute_method_name(e, inst, "execute-command", CSTR);
	
	if (ret == NO_ERROR)
	{
		POP(e, *hwerr);

		if (*hwerr == 0)
			POP(e, *status);
	}

	DPRINTF(("execute_cmd: buf=%p buflen=%d dir=%d cmd=%p cmdlen=%d "
			"hwerr=%#x status=%d ret=%d\n",
			buf, buflen, dir, cmd, cmdlen, *hwerr, *status, ret));
	return ret;
}

Retcode
scsi_retry_cmd(Environ *e, Byte *buf, Int buflen, Int dir, Int retries,
		Byte *cmd, Int cmdlen, Int *hwerr, Int *status, Bool *gotsense,
		struct scsi_sense *sense_cmd, struct scsi_sense_data_new *sense_data)
{
	Int status2;
	Retcode ret;
	
	DPRINTF(("scsi_retry_cmd\n"));
	*status = -1;
	*gotsense = FALSE;

	while (retries == -1 || retries-- > 0)
	{
		*gotsense = FALSE;
		ret = execute_cmd(e, buf, buflen, dir, cmd, cmdlen, hwerr, status);

		if (ret != NO_ERROR)
			return ret;

		if (*hwerr == 0 && *status == SCSI_OK)	/* command successful */
			return NO_ERROR;

		if (*hwerr)			/* hardware error - do not retry */
			return NO_ERROR;

		if (*status == SCSI_BUSY)	/* busy - retry */
			continue;

		/* send REQUEST SENSE command to get extended info */
		DPRINTF(("scsi_retry_cmd: got status=%#x - sending REQUEST SENSE\n",
				*status));
		memset(sense_cmd, 0, sizeof *sense_cmd);
		sense_cmd->op_code = REQUEST_SENSE;
		sense_cmd->length = sizeof *sense_data;
		ret = execute_cmd(e, (Byte*)sense_data, sizeof *sense_data,
				FTRUE, (Byte*)sense_cmd, sizeof *sense_cmd,
				hwerr, &status2);
		DPRINTF(("scsi_retry_cmd: status=%#x err_code=%#x extflags=%#x\n",
			status2, sense_data->error_code, sense_data->ext.extended.flags));

		if (ret != NO_ERROR)
			return ret;

		if (*hwerr)			/* hardware error - do not retry */
			return NO_ERROR;

		if (status2 == SCSI_OK &&
				(sense_data->error_code & SSD_ERRCODE_VALID))
		{
			Int sense = sense_data->ext.extended.flags & SSD_KEY;

			*gotsense = TRUE;

			if (sense_data->ext.extended.flags &
					(SSD_FILEMARK | SSD_ILI | SSD_EOM))		/* not retryable */
				break;

			/* no retryable errors? */
			if (!((sense & SSD_KEY_NO_SENSE) ||
					(sense & SSD_KEY_RECOVERED_ERROR) ||
					(sense & SSD_KEY_NOT_READY) ||
					(sense & SSD_KEY_UNIT_ATTENTION) ||
					(sense & SSD_KEY_ABORTED_COMMAND)))
				break;
		}

		/* wait for a second or so before retrying - maybe a slow CD */
		u_sleep(1000000);
	}

	return NO_ERROR;
}

ESCSIDEV(install_scsi_disk_driver);

static Retcode
make_scsi_device(Environ *e, Package *pkg, Int id, Int lun, Int devid)
{
	/* array matches types of possible SCSI devices as reported by INQUIRY */
	static SCSI_device devices[] =
	{
		{ "disk", "block", install_scsi_disk_driver },
		{ "tape", "byte", NULL },
		{ "printer", "serial", NULL },
		{ "processor", "processor", NULL },
		{ "worm", "block", install_scsi_disk_driver },
		{ "cd", "block", install_scsi_disk_driver },
		{ "scanner", "serial", NULL },
		{ "optical", "block", install_scsi_disk_driver },
		{ "changer", "block", NULL },
		{ "comm", "serial", NULL },
		{ "asc0", "asc", NULL },
		{ "asc1", "asc", NULL },
		{ "reserved", "reserved", NULL },
	};
	Byte *prop;
	Int plen = 0;
	Retcode ret;
	SCSI_device *dev = &devices[devid > 0x0B ? 0x0C : devid];

	/* create new package with name property */
	pkg = new_pkg_name(pkg, dev->name);

	if (pkg == NULL)
		return E_OUT_OF_MEMORY;

	/* set the type property of this device */
	prop_set_str(pkg->props, "device_type", CSTR, dev->type, CSTR);

	/* encode "reg" property for unit address */
	prop = prop_alloc(e, 3 * sizeof (Int));

	if (prop == NULL)
		return E_OUT_OF_MEMORY;

	prop_encode_int(prop + plen, &plen, id);	/* phys.hi - ID */
	prop_encode_int(prop + plen, &plen, lun);	/* phys.lo - LUN */
	prop_encode_int(prop + plen, &plen, 0);		/* size */
	ret = add_property(pkg->props, "reg", CSTR, prop, plen);

	if (ret == NO_ERROR && dev->install)
		ret = dev->install(e, pkg, dev, id, 0);

	return ret;
}

Retcode
scsi_probe(Environ *e, Package *pkg, Bool verbose)
{
	Package *child, *next;
	Package *savepkg = e->currpkg;
	Instance *inst;
	Cell saveinst = e->currinst;
	Int id, lun, i;
	Int maxid = 8;
	Retcode ret;
	Int hwerr, status;
	struct scsi_inquiry *cmd;
	struct scsi_inquiry_data *data;
	Byte str[STR_SIZE];

	IFCKSP(e, 0, 2);
	DPRINTF(("scsi_probe\n"));

	if (pkg == NULL)
		return E_NULL_PACKAGE;
	
	get_device_name(e, pkg, str);
	DPRINTF(("scsi_probe: str=\"%s\"\n", str));
	e->currpkg = pkg;

	/* call "open-dev" to open ourself */
	PUSHP(e, str);
	PUSH(e, strlen(str));
	ret = execute_word(e, "open-dev");

	if (ret != NO_ERROR)
		return ret;

	POP(e, e->currinst);

	if (e->currinst == (uPtr)NULL)
		return E_NULL_INSTANCE;

	inst = (Instance*)(uPtr)e->currinst;

	/* allocate space for our command and data buffers */
	PUSH(e, sizeof *cmd + sizeof *data);
	ret = execute_method_name(e, inst, "dma-alloc", CSTR);
	DPRINTF(("scsi_probe: dma-alloc ret=%d\n", ret));

	if (ret != NO_ERROR)
		goto done2;

	POPT(e, cmd, struct scsi_inquiry*);
	data = (struct scsi_inquiry_data *)((Byte*)(cmd + 1));
	DPRINTF(("scsi_probe: cmd=%p data=%p\n", cmd, data));
	memset(cmd, 0, sizeof *cmd);
	memset(data, 0, sizeof *data);
	cmd->op_code = INQUIRY;
	cmd->length = sizeof *data;

	/* delete all children of our parent node which are all SCSI devs */
	for (child = pkg->children; child != NULL; child = next)
	{
		next = child->link;
		delete_package(child);
	}

	PUSH(e, 100);		/* timeout */
	ret = execute_method_name(e, inst, "set-timeout", CSTR);

	if (ret != NO_ERROR)
		goto done;

	/* wide-SCSI supports 0..15 for IDs, as per proposal/extension #288 */
	if (find_table(pkg->props, "wide", CSTR))
		maxid = 16;

	for (id = 0; id < maxid; id++)		/* look at all possible targets */
	{
		for (lun = 0; lun < 8; lun++)	/* and all LUNs at each target */
		{
			PUSH(e, lun);
			PUSH(e, id);
			ret = execute_method_name(e, inst, "set-address", CSTR);

			if (ret != NO_ERROR)
				goto done;

			memset(data, 0, sizeof *data);
			hwerr = 0;
			status = 0;
			ret = execute_cmd(e, (Byte*)data, sizeof *data, TRUE,
					(Byte*)cmd, sizeof *cmd, &hwerr, &status);

			/* if timeout, try next SCSI ID - no device here */
			if (hwerr == -1)		/* assume timeout */
				break;

			/* any other error */
			if (hwerr || status)
			{
				cprintf(e,
				"scsi_probe: SCSI error for ID %d: hwerr=%#x status=%#x\n",
						id, hwerr, status);
				break;
			}

			/* use INQUIRY info to create child device node with right name */
			i = data->device & SID_TYPE;

			/* no more LUNs at this target - break out of LUN loop */
			if (i == 0x1F || (data->device & SID_QUAL_BAD_LU))
				break;

			ret = make_scsi_device(e, pkg, id, lun, i);

			if (ret != NO_ERROR)
				goto done;

			if (verbose)
			{
				/* print out INQUIRY info */
				memcpy(str, data->vendor, sizeof data->vendor);
				str[sizeof data->vendor] = '\0';
				cprintf(e, "%d: %s", id, str);
				memcpy(str, data->product, sizeof data->product);
				str[sizeof data->product] = '\0';
				cprintf(e, " | %s", str);
				memcpy(str, data->revision, sizeof data->revision);
				str[sizeof data->revision] = '\0';
				cprintf(e, " | %s", str);
				cprintf(e, " | SCSI-%d", data->version & SID_ANSII);

				if (data->flags & SID_Sync)
					cprintf(e, " fast");

				if (data->flags & SID_WBus16)
					cprintf(e, " wide");

				if (data->flags & SID_WBus32)
					cprintf(e, " wide32");

				cprintf(e, "\n");
			}
		}
	}

done:
	/* free our command/data buffers */
	PUSHP(e, cmd);
	PUSH(e, sizeof *cmd + sizeof *data);
	(void)execute_method_name(e, inst, "dma-free", CSTR);

done2:
	/* close our device and restore e->currinst */
	PUSH(e, e->currinst);
	(void)execute_word(e, "close-dev");
	e->currinst = saveinst;
	e->currpkg = savepkg;

	return ret;
}
