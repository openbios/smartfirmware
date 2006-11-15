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


#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

#include <sys/stat.h>

#define MAXPATHLEN	1024

struct spd_info
{
    struct spd_info *next;
    char *name;
    unsigned char data[256];
};

static struct spd_info *spds = NULL;
static struct spd_info **index = NULL;
static int spdcount = 0;

static int
read_spd(char *name, char *path)
{
    FILE *fp = fopen(path, "r");
    unsigned char buf[256];
    struct spd_info *info;

//    printf("Path %s\n", path);

    if (fp == NULL)
	return 0;

//    printf("Reading %s\n", name);

    if (fread(buf, sizeof (unsigned char), 256, fp) != 256)
    {
	fclose(fp);
	return 0;
    }

    fclose(fp);
    info = (struct spd_info *)malloc(sizeof (struct spd_info) + strlen(name) + 1);

    if (info == NULL)
	return 0;

    info->next = spds;
    info->name = (char *)&info[1];
    strcpy(info->name, name);
    memcpy(info->data, buf, 256);
    spds = info;
    return 1;
}

int
read_spds()
{
    DIR *dir = opendir("spds");
    struct dirent *ent;
    int errs = 0;
    char path[MAXPATHLEN];
    struct spd_info *spd;
    int count = 0;

    strcpy(path, "spds/");

//    printf("Reading SPDs\n");

    if (dir == NULL)
	return 0;

//    printf("Got dir\n");

    while ((ent = readdir(dir)) != NULL)
    {
	struct stat statbuf;

	strcpy(&path[5], ent->d_name);
//	printf("Found %s\n", ent->d_name);

	if (stat(path, &statbuf) < 0)
	    continue;

//	printf("Able to stat %s\n", ent->d_name);

	if (!S_ISREG(statbuf.st_mode))
	    continue;

//	printf("Is regular file %s\n", ent->d_name);

	if (statbuf.st_size != 256)
	    continue;

//	printf("Is correct size %s\n", ent->d_name);

	if (!read_spd(ent->d_name, path))
	    errs++;
    }

    closedir(dir);

    for (spd = spds; spd; spd = spd->next)
	count++;

    index = (struct spd_info **)malloc(sizeof (struct spd_info *) * count);

    if (index != NULL)
    {
	for (spd = spds, count = 0; spd; spd = spd->next, count++)
	    index[count] = spd;
    }
    else
	errs++;

    if (errs == 0)
    {
	spdcount = count;
	return 1;
    }

    return 0;
}

int
spd_count()
{
    return spdcount;
}

char *
spd_name(int n)
{
    if (n < 0 || n >= spdcount)
	return NULL;

    return index[n]->name;
}

int
spd_num(char *name)
{
    int i;

//    printf("Name = %s\n", name);

    for (i = 0; i < spdcount; i++)
    {
//	printf("Entry = %s\n", index[i]->name);

	if (strcasecmp(index[i]->name, name) == 0)
	    return i;
    }

    return -1;
}

unsigned char
spd_byte(int n, int b)
{
    if (n < 0 || n >= spdcount || b < 0 || b >= 256)
	return 0xFF;

    return index[n]->data[b];
}
