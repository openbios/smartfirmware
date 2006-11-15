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


#define DEBUG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <db.h>

#include "spamlib.h"

/* The straight DBM functions resulted in a database that was way to big */
/* so I switch to the DB functions.  These functions are wrappers so that */
/* I don't have to rewrite everything. */

#define DB_REPLACE	0
#define DB_INSERT	1

DB *
xdb_open_btree(char *filename, int flags, int mode)
{
    return dbopen(filename, flags, mode, DB_BTREE, NULL);
}

DB *
xdb_open_hash(char *filename, int flags, int mode)
{
    return dbopen(filename, flags, mode, DB_HASH, NULL);
}

void
xdb_close(DB *db)
{
    (*db->close)(db);
}

DBT
xdb_fetch(DB *db, DBT key)
{
    DBT data;
    int status;

    status = (*db->get)(db, &key, &data, 0);

    if (status)
    {
	data.data = NULL;
	data.size = 0;
    }

    return data;
}

DBT
xdb_firstkey(DB *db)
{
    DBT key;
    DBT data;
    int status;

    status = (*db->seq)(db, &key, &data, R_FIRST);

    if (status)
	key.data = NULL;

    return key;
}

DBT
xdb_nextkey(DB *db)
{
    DBT key;
    DBT data;
    int status;

    status = (*db->seq)(db, &key, &data, R_NEXT);

    if (status)
	key.data = NULL;

    return key;
}

int
xdb_delete(DB *db, DBT key)
{
    if ((*db->del)(db, &key, 0))
	return -1;

    return 0;
}

int
xdb_store(DB *db, DBT key, DBT data, int flags)
{
    if (flags == DB_INSERT)
	flags = R_NOOVERWRITE;
    else
	flags = 0;

    return (*db->put)(db, &key, &data, flags);
}
