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
#include <unistd.h>

#include "spamlib.h"

char *prog;

void
usage()
{
    fprintf(stderr, "Usage: %s [-h] [-b] [filename]\n", prog);
    exit(2);
}

int
main(int argc, char **argv)
{
    SPAMDB *db;
    int ch;
    int hash = 0;
    int btree = 0;
    char *filename = NULL;
    extern int optind;

    prog = argv[0];
    g_spamlibprog = argv[0];

    while ((ch = getopt(argc, argv, "B:hb")) != EOF)
	switch (ch)
	{
	case 'B':
	    filename = optarg;
	    break;
	case 'h':
	    hash = 1;
	    break;
	case 'b':
	    btree = 1;
	    break;
	default:
	    usage();
	    break;
	}

    if (optind < argc)
	filename = argv[optind];

    if (!hash && !btree)
	hash = 1;

    if (hash)
	db = spaminitdb(filename, SPAM_INITDB_KEY_HASH);
    else if (btree)
	db = spaminitdb(filename, SPAM_INITDB_KEY);
    else
	db = NULL;

    spamadjustfiles(db,0, 0);
    spamclosedb(db);
    exit(0);
}
