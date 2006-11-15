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

#undef DPRINTF
#define DPRINTF(arg)	/* arg */

extern int g_amd8111_device_id;

volatile unsigned int CSR_BASE;

#define SMB_BASE		CSR_BASE	/* XXX */
#define SMB_STATUS		(SMB_BASE + 0xE0)
#define SMB_CONTROL		(SMB_BASE + 0xE2)
#define SMB_ADDR		(SMB_BASE + 0xE4)
#define SMB_DATA		(SMB_BASE + 0xE6)
#define SMB_CMD			(SMB_BASE + 0xE8) /* byte */
#define SMB_FIFO		(SMB_BASE + 0xE9) /* byte - for blk cmds */

/* PME0 */
#define SMB_STAT_SMB_BUSY	0x0800	/* RO */
#define SMB_STAT_HOST_BUSY	0x0008	/* RO */
#define SMB_STAT_HOST_DONE	0x0010	/* write to clear */
#define SMB_STAT_ERROR		0x0027	/* write to clear */

/* PME2 */
#define SMB_CTRL_START		0x0008
#define SMB_CTRL_CYC_BYTE	0x0002
#define SMB_CTRL_CYC_WORD	0x0003
#define SMB_CTRL_CYC_BLOCK	0x0005

/* PME3 */
#define SMB_ADDR_READ_CYCLE	0x0001	/* read==high write==low */
#define SMB_ADDR_WRITE_CYCLE	0x0000	/* read==high write==low */

#define SMB_WRITE_BYTE	0x06
#define SMB_READ_BYTE	0x07
#define SMB_WRITE_BLOCK	0x0A
#define SMB_READ_BLOCK	0x0B


uShort
isa_readw(uInt addr)
{
	uShort val;

	__asm __volatile("inw %%dx,%0" : "=a" (val) : "d" (addr));
	return val;
}

void
isa_writew(uInt addr, uShort val)
{
	uShort v;

	v = val;
	__asm __volatile("outw %0,%%dx" : : "a" (v), "d" (addr));
}


static int
in(int addr)
{
    return isa_readw(addr);
}

static void
out(int addr, int val)
{
    isa_writew(addr, val);
}

static int
inb(int addr)
{
    return isa_read(addr);
}

static void
outb(int addr, int val)
{
    isa_write(addr, val);
}


static int
wait(void)
{
    DPRINTF((".%02X", in(SMB_STATUS)));

    while (in(SMB_STATUS) & (SMB_STAT_SMB_BUSY | SMB_STAT_HOST_BUSY))
    {
	DPRINTF(("?%#X", in(SMB_STATUS)));
    }

    while ((in(SMB_STATUS) & (SMB_STAT_HOST_DONE | SMB_STAT_ERROR)) == 0)
    {
	DPRINTF(("?%#X", in(SMB_STATUS)));
    }

    DPRINTF((".%#X ", in(SMB_STATUS)));
    DPRINTF((",%02X ", in(SMB_STATUS)));

    if (in(SMB_STATUS) & SMB_STAT_HOST_DONE)
	out(SMB_STATUS, SMB_STAT_HOST_DONE);

    if (in(SMB_STATUS) & SMB_STAT_ERROR)
    {
	out(SMB_STATUS, SMB_STAT_ERROR);
	return 1;
    }

    return 0;
}


int
smb_write_blk(unsigned char addr, unsigned char cmd,
		unsigned char blk[], int num)
{
    int i;

    out(SMB_ADDR, (addr << 1) | SMB_ADDR_WRITE_CYCLE);
    outb(SMB_CMD, cmd);
    out(SMB_CONTROL, SMB_CTRL_CYC_BLOCK);
    out(SMB_DATA, num);

    for (i = 0; i < num; i++)
	outb(SMB_FIFO, blk[i]);

    out(SMB_CONTROL, SMB_CTRL_CYC_BLOCK | SMB_CTRL_START);

    if (wait())
	return 1;

    return 0;
}

int
smb_read_blk(unsigned char addr, unsigned char cmd,
		unsigned char blk[], int *num)
{
    int i;

    out(SMB_ADDR, (addr << 1) | SMB_ADDR_READ_CYCLE);
    outb(SMB_CMD, cmd);
    out(SMB_CONTROL, SMB_CTRL_CYC_BLOCK);
    out(SMB_CONTROL, SMB_CTRL_CYC_BLOCK | SMB_CTRL_START);

    if (wait())
	return 1;

    *num = in(SMB_DATA);

    for (i = 0; i < *num; i++)
	blk[i] = inb(SMB_FIFO);

    return 0;
}

int
smb_read_byte(unsigned char addr, unsigned char cmd, int *data)
{
    out(SMB_ADDR, (addr << 1) | SMB_ADDR_READ_CYCLE);
    outb(SMB_CMD, cmd);
    out(SMB_CONTROL, SMB_CTRL_CYC_BYTE);
    out(SMB_CONTROL, SMB_CTRL_CYC_BYTE | SMB_CTRL_START);

    if (wait())
	return 1;

    *data = in(SMB_DATA);
    return 0;
}

#if 0
C(dump_clk)
{
    unsigned char addr = 0xD2;
    unsigned char data[32];
    int num, i;

    pci_config_write(0, 0, g_amd8111_device_id, 2, 4, 0x3);

    smb_read_blk(addr, data, &num);
    cprintf(e, "dump_blk: addr=%#X count=%d\n", addr, num);

    for (i = 0; i < num; i++)
	cprintf(e, "    %02d: %#X\n", i, data[i]);

    return NO_ERROR;
}
#endif

C(dump_smb)
{
    unsigned int addr, cmd;
    int num, i;
    unsigned char data[32];

    cprintf(e, "config_read(0x40)=%#x\n", pci_config_read(0,0,g_amd8111_device_id,3,0x40));
    cprintf(e, "config_read(0x64)=%#x\n", pci_config_read(0,0,g_amd8111_device_id,3,0x64));
    /*pci_config_write(0, 0, g_amd8111_device_id, 3, 0x40,
	    pci_config_read(0,0, g_amd8111_device_id,3,0x40) | 0x8000);*/

    CSR_BASE = pci_config_read(0, 0, g_amd8111_device_id, 3, 0x58);
    CSR_BASE &= 0xFF00;
    cprintf(e, "CSR_BASE=%#x [0:1]=%#x [1]=%#x\n", SMB_BASE + SMB_STATUS,
	    in(SMB_STATUS), inb(SMB_STATUS + 1));

    for (addr = 0; addr <= 0x7F; addr++)
    {
	if (addr != 0x69)
	{
	    for (cmd = 0; cmd <= 0x7F; cmd++)
	    {
		num = 0;

		if (smb_read_byte(addr, cmd, &num))
		{
		    //cprintf(e, "addr=%02X cmd=%02X [NO DEV]\n", addr, cmd);
		    //goto next_addr;
		    continue;
		}

		if (cmd == 0)
		    cprintf(e, "\n");

		cprintf(e, "addr=%02X cmd=%02X data=%02X (%c)\n",
			addr, cmd, num, isprint(num) ? num : '?');
	    }
	}
	else
	{
	    memset(data, 0, sizeof data);
	    num = 0;

	    if (smb_read_blk(addr, 0x00, data, &num))
		//goto next_addr;
		continue;

	    cprintf(e, "\naddr=%02X count=%02X:",
		    addr, num);

	    for (i = 0; i < num; i++)
		cprintf(e, " %02X", data[i]);

	    cprintf(e, "\n");
	}
//next_addr:
    }

    return NO_ERROR;
}

C(load_clk)
{
    unsigned char data[32];
    int num, i, ret;
    int addr = 0x69;

    CSR_BASE = pci_config_read(0, 0, g_amd8111_device_id, 3, 0x58);
    CSR_BASE &= 0xFF00;
    cprintf(e, "CSR_BASE=%#x [0:1]=%#x [1]=%#x\n", SMB_BASE + SMB_STATUS,
	    in(SMB_STATUS), inb(SMB_STATUS + 1));

    memset(data, 0, sizeof data);
    num = 0;

    if (smb_read_blk(addr, 0x00, data, &num))
    {
	cprintf(e, "read-blk command for addr $#x failed!\n", addr);
	return E_ABORT;
    }

    cprintf(e, "\naddr=%02X count=%02X:",
	    addr, num);

    for (i = 0; i < num; i++)
	cprintf(e, " %02X", data[i]);

    cprintf(e, "\n");

    cprintf(e, "attempting write to clk...");
    ret = smb_write_blk(addr, 0x00, data, num);
    cprintf(e, "%s!\n", ret ? "failed" : "successful");

    return NO_ERROR;
}
