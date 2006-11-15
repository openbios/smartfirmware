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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ar.h>
#include <ranlib.h>
#include <a.out.h>

typedef int boolean;
#define TRUE 1
#define FALSE 0

int g_nsyms = 0;
struct nlist *g_syms = NULL;
char *g_strings = NULL;
long g_textsize = 0;
long g_datasize = 0;
unsigned char *g_text = NULL;
unsigned char *g_data = NULL;

void
reloc(FILE *fp, struct exec *exec, int i, unsigned char *seg)
{
#if 0
	struct relocation_info reloc;

	fread(&reloc, sizeof reloc, 1, fp);
	printf( /* ( */ "%4d)  0x%08X  [0x", i, reloc.r_address);
	seg += reloc.r_address;

	switch (reloc.r_length)
	{
	case 3:
		seg += 7;
		break;
	case 2:
		seg += 3;
		break;
	case 1:
		seg += 1;
		printf("    ");
		break;
	case 0:
		printf("      ");
		break;
	}

	switch (reloc.r_length)
	{
	case 3:
		printf("%02X", *seg--);
		printf("%02X", *seg--);
		printf("%02X", *seg--);
		printf("%02X", *seg--);
	case 2:
		printf("%02X", *seg--);
		printf("%02X", *seg--);
	case 1:
		printf("%02X", *seg--);
	case 0:
		printf("%02X", *seg--);
	}

	printf("]  %s  ", reloc.r_pcrel ? "+PC" : "   ");

	if (reloc.r_extern)
		printf("%s", g_syms[reloc.r_symbolnum].n_un.n_name);
	else
	{
		switch (reloc.r_symbolnum)
		{
		case N_UNDF:
			printf(".UNDF");
			break;
		case N_ABS:
			printf(".ABS");
			break;
		case N_TEXT:
			printf(".TEXT");
			break;
		case N_DATA:
			printf(".DATA");
			break;
		case N_BSS:
			printf(".BSS");
			break;
		case N_COMM:
			printf(".COMM");
			break;
		case N_FN:
			printf(".FN");
			break;
		default:
			printf("0x%08X", reloc.r_symbolnum);
			break;
		}
	}

	printf("\n");
#endif
}

void
dumpreloc(FILE *fp, long pos, struct exec *exec)
{
	int i, nreloc, patch;
	long t;

	g_textsize = N_DATOFF((*exec)) - N_TXTOFF((*exec));
	g_datasize = exec->a_data;
	g_text = (unsigned char *)malloc(g_textsize + 1);
	g_data = (unsigned char *)malloc(g_datasize + 1);

	if (g_text == NULL || g_data == NULL)
		exit(1);

	t = N_TXTOFF((*exec));
	fseek(fp, t + pos, SEEK_SET);
	fread(g_text, sizeof *g_text, g_textsize, fp);
	g_text[g_textsize] = '\0';
	t = N_DATOFF((*exec));
	fseek(fp, t + pos, SEEK_SET);
	fread(g_data, sizeof *g_data, g_datasize, fp);
	g_data[g_datasize] = '\0';

#if 0
	printf("\ntext relocation:\n");
	t = N_TREOFF((*exec));
	fseek(fp, t + pos, SEEK_SET);
	nreloc = (N_DRELOC((*exec)) - N_TREOFF((*exec))) /
		sizeof (struct relocation_info);

	for (i = 0; i < nreloc; i++)
		reloc(fp, exec, i, g_text);

	printf("\ndata relocation:\n");
	nreloc = (N_SYMOFF((*exec)) - N_DRELOC((*exec))) /
		sizeof (struct relocation_info);

	for (i = 0; i < nreloc; i++)
		reloc(fp, exec, i, g_data);
#endif
}

void
dumpsyms(FILE *fp, long pos, struct exec *exec)
{
	int i, strsize;
	char *mesg, type;
	long t;

	g_nsyms = exec->a_syms / sizeof (struct nlist);
	g_syms = (struct nlist *)malloc(exec->a_syms);

	if (g_syms == NULL)
		exit(43);

	printf("\nsymbol table:  (%d symbols)\n", g_nsyms);
	t = N_SYMOFF((*exec));
	fseek(fp, t + pos, SEEK_SET);
	fread(g_syms, sizeof *g_syms, g_nsyms, fp);

	t = N_STROFF((*exec));
	fseek(fp, t + pos, SEEK_SET);
	fread(&strsize, sizeof strsize, 1, fp);
	g_strings = (char *)malloc(strsize);

	if (g_strings == NULL)
		exit(43);

	fread(g_strings, sizeof *g_strings, strsize, fp);

	for (i = 0; i < g_nsyms; i++)
	{
		if (g_syms[i].n_un.n_strx > 0)
			g_syms[i].n_un.n_name = g_strings + g_syms[i].n_un.n_strx -
				sizeof strsize;
		else
			g_syms[i].n_un.n_name = "?";

		switch (g_syms[i].n_type & N_TYPE)
		{
		case N_UNDF:
			type = 'u';
			break;

		case N_ABS:
			type = 'a';
			break;

		case N_TEXT:
			type = 't';
			break;

		case N_DATA:
			type = 'd';
			break;

		case N_BSS:
			type = 'b';
			break;

		case N_COMM:
			type = 'c';
			break;

		case N_FN:
			type = 'f';
			break;

		default:
			type = '?';
		}

		printf("%4d)  ", i);

		if (type != 'u')
			printf("0x%08X  ", g_syms[i].n_value);
		else
			printf("            ");

		if (type != '?' && g_syms[i].n_type & N_EXT)
			type = type + 'A' - 'a';

		printf("%c  %s\n", type, g_syms[i].n_un.n_name);
	}
}

char*
ar2str(char *ar, int len)
{
	static char buf[256];

	strncpy(buf, ar, len);

	for (len--; len && (buf[len] == ' ' || buf[len] == '\0'); len--)
		;

	buf[len + 1] = '\0';
	return buf;
}

extern void dumpfile(FILE *fp, long pos);

int
dumparch(FILE *fp, long pos)
{
	long i, a, n, num;
	char str[SARMAG + 1], *strtab, *s;
	struct ar_hdr ar;
	struct ranlib *ran;

	fread(str, sizeof (char), SARMAG, fp);
	str[SARMAG] = '\0';

	if (strcmp(str, ARMAG) != 0)
	{
		fseek(fp, pos, SEEK_SET);
		return FALSE;
	}

	pos += SARMAG;
	printf("\narchive format:\n");
	a = 0;

	while (fread(&ar, sizeof ar, 1, fp) == 1)
	{
		printf("\nentry %d\n", a++);
		printf("offset = 0x%08X\n", pos);
		pos += sizeof ar;
		printf("name = '%s'\n", ar2str(ar.ar_name, sizeof ar.ar_name));
		printf("date = %s\n", ar2str(ar.ar_date, sizeof ar.ar_date));
		printf("uid/gid = %s\n", ar2str(ar.ar_uid, sizeof ar.ar_uid));
		printf("%s\n", ar2str(ar.ar_gid, sizeof ar.ar_gid));
		printf("mode = %s\n", ar2str(ar.ar_mode, sizeof ar.ar_mode));
		printf("size = %s\n", ar2str(ar.ar_size, sizeof ar.ar_size));

		s = ar2str(ar.ar_name, sizeof ar.ar_name);

		if (!strncmp(ar.ar_name, AR_EFMT1, sizeof (AR_EFMT1)))
		{
			n = atoi(s + sizeof (AR_EFMT1));
			fread(s, sizeof (char), n, fp);
			pos += n;
		}

		if (!strcmp(s, RANLIBMAG))	/* ranlib file */
		{
			printf("ranlib symbol table:\n");
			fread(&n, sizeof (long), 1, fp);
			ran = (struct ranlib *)malloc(n + 1);
			num = n / sizeof (struct ranlib);

			if (ran == NULL)
				exit(2);

			fread(ran, sizeof *ran, num, fp);
			fread(&n, sizeof (long), 1, fp);
			strtab = (char *)malloc(n + 1);
			fread(strtab, sizeof (char), n, fp);

			for (i = 0; i < num; i++)
			{
				ran[i].ran_un.ran_name = strtab + ran[i].ran_un.ran_strx;
				printf( /* ( */ "%4d)  0x%08X  %s\n", i,
						ran[i].ran_off, ran[i].ran_un.ran_name);
			}

			free(strtab);
			free(ran);
		}
		else
			dumpfile(fp, pos);

		pos += atoi(ar2str(ar.ar_size, sizeof ar.ar_size));

		if (pos & 0x01)
			pos++;

		fseek(fp, pos, SEEK_SET);
	}

	return TRUE;
}

void
dumpfile(FILE *fp, long pos)
{
	int i, nscns;
	char *mesg;
	boolean sysok;
	boolean mips = FALSE;
	struct exec exec;

	if (dumparch(fp, pos))
		return;

	sysok = FALSE;
	fseek(fp, pos, SEEK_SET);
	fread(&exec, sizeof exec, 1, fp);

	printf("midmag = %d (0x%X) ", exec.a_magic, exec.a_magic);
	printf("magic = %d (0x%X) ", N_GETMAGIC(exec), N_GETMAGIC(exec));
	printf("mid = %d (0x%X)\n", N_GETMID(exec), N_GETMID(exec));

	switch (N_GETMID(exec))
	{
	case MID_SUN010:
		mesg = "sun 68010/68020 binary";
		break;

	case MID_SUN020:
		mesg = "sun 68020-only binary";
		break;

	case 100 /* MID_PC386 */ :
		mesg = "386 PC binary. (so quoth BFD)";
		break;

	case MID_HP200:
		mesg = "hp200 (68010) BSD binary";
		break;

	case MID_HP300:
		mesg = "hp300 (68020+68881) BSD binary";
		break;

	case MID_I386:
		mesg = "i386 BSD binary";
		break;

	case 135 /* MID_M68K */ :
		mesg = "m68k BSD binary with 8K page sizes";
		break;

	case 136 /* MID_M68K4K */ :
		mesg = "m68k BSD binary with 4K page sizes";
		break;

	case 137 /* MID_NS32532 */ :
		mesg = "ns32532";
		break;

	case MID_SPARC:
		mesg = "sparc";
		break;

	case 139 /* MID_PMAX */ :
		mesg = "pmax";
		break;

	case 140 /* MID_VAX */ :
		mesg = "vax";
		break;

	case 151 /* MID_MIPS? */ :
		mesg = "mips?";
		mips = TRUE;
		break;

	case MID_HPUX:
		mesg = "hp200/300 HP-UX binary";
		break;

	case MID_HPUX800:
		mesg = "hp800 HP-UX binary";
		break;

	default:
		mesg = "unknown machine";
	}

	printf("%s\n", mesg);

	switch (N_GETMAGIC(exec))
	{
	case OMAGIC:
		mesg = "old impure format (OMAGIC)";
		break;

	case NMAGIC:
		mesg = "read-only text (NMAGIC)";
		break;

	case ZMAGIC:
		mesg = "demand load format (ZMAGIC)";

		if (mips)
		{
			printf("ZMAGIC treated as QMAGIC because of bugs in GNU tools\n");
			exec.a_magic = QMAGIC;
		}

		break;

	case QMAGIC:
		mesg = "\"compact\" demand load format (QMAGIC)";
		break;

	default:
		mesg = "unknown executable type";
	}

	printf("%s\n", mesg);

	printf("sizeof .text = 0x%08X (%10d)\n",
			exec.a_text, exec.a_text);
	printf("sizeof .data = 0x%08X (%10d)\n",
			exec.a_data, exec.a_data);
	printf("sizeof .bss  = 0x%08X (%10d)\n",
			exec.a_bss, exec.a_bss);
	printf("   <total>   = 0x%08X (%10d)\n",
			exec.a_text + exec.a_data + exec.a_bss,
			exec.a_text + exec.a_data + exec.a_bss);
	printf("entry point  = 0x%08X (%10d)\n",
			exec.a_entry, exec.a_entry);
	printf("text_start   = 0x%08X (%10d)\n",
			N_TXTADDR(exec), N_TXTADDR(exec));
	printf("data_start   = 0x%08X (%10d)\n",
			N_DATADDR(exec), N_DATADDR(exec));

	dumpsyms(fp, pos, &exec);
	dumpreloc(fp, pos, &exec);
	free(g_strings);
	free(g_syms);
	free(g_text);
	free(g_data);
	g_strings = NULL;
	g_syms = NULL;
	g_text = NULL;
	g_data = NULL;
}

int
main(int argc, const char *argv[])
{
	int i;
	FILE *fp;

	if (argc == 1)
	{
		fprintf(stderr, "usage: %s files\n", argv[0]);
		exit(1);
	}

	for (i = 1; i < argc; i++)
	{
		printf("\n%s:\n", argv[i]);

		if ((fp = fopen(argv[i], "rb")) == NULL)
		{
			fprintf(stderr, "cannot open file %s for reading\n", argv[i]);
			return;
		}

		dumpfile(fp, 0);
		fclose(fp);
	}

	return 0;
}
