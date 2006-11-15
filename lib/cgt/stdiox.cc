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

//
//	Copyright (C) 1991  Thomas J. Merritt
//

#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#ifdef macintosh
  #include <Files.h>
#else
  #include <sys/types.h>
  #include <sys/stat.h>
#endif
#include "stdlibx.h"
#include "stdiox.h"
#include "expand.h"

char *
xgets(FILE *fp, boolean nl)
{
    static char *buf = NULL;
    static int buflen = 0;
    int len = 0;
    int ch;

    while ((ch = getc(fp)) != EOF)
    {
	expand(&buf, &buflen, len + 1);

	if (ch == '\n' && !nl)
	{
	    buf[len] = '\0';
	    return buf;
	}

	buf[len++] = ch;

	if (ch == '\n')
	{
	    expand(&buf, &buflen, len + 1);
	    buf[len] = '\0';
	    return buf;
	}
    }

    if (len == 0)
    	return NULL;

    expand(&buf, &buflen, len + 1);
    buf[len] = '\0';

    if (buf[len - 1] == '\r')
	buf[len - 1] = '\0';

    return buf;
}

FILE *
fileopen(char const *dir, char const *file, char const *mode)
{
    static char *buf = NULL;
    static int buflen = 0;

    if (dir == NULL || dir[0] == '\0')
    	dir = CURRENT_DIR;

    expand(&buf, &buflen, strlen(dir) + strlen(file) + 2);
    sprintf(buf, "%s%c%s", dir, PATH_SEP, file);
    return fopen(buf, mode);
} 

FILE *
pathopen(char *path, char const *file, char const *mode)
{
    if (path == NULL)
	path = CURRENT_DIR;

    if (file == NULL || file == '\0')
	return NULL;

    if (mode == NULL)
	mode = "r";

#ifdef macintosh
    if (file[0] != PATH_SEP)	// Mac abs paths do not start with PATH_SEP
    	return fopen(file, mode);
#else
    if (file[0] == PATH_SEP)	// assume PATH_SEP also marks the root
    	return fopen(file, mode);
#endif

    char *sep;
    FILE *fp;

    for (sep = strchr(path, PATH_LIST_SEP); sep != NULL;
	    path = sep + 1, sep = strchr(path, PATH_LIST_SEP))
    {
	*sep = '\0';
	fp = fileopen(path, file, mode);
	*sep = PATH_LIST_SEP;

	if (fp != NULL)
	    return fp;
    }

    return fileopen(path, file, mode);
}

FILE *
pathvopen(char const *pathvar, char const *file, char const *mode)
{
    char *var = getenv(pathvar);

    if (var == NULL)
	return NULL;

    return pathopen(var, file, mode);
}

boolean
filelength(const char *name, size_t &size)
{
#ifdef macintosh
    short file;
    Str255 path;

    strcpy((char*)&path[1], name);
    path[0] = (unsigned char)strlen(name);

    if (FSOpen(path, 0, &file) != noErr)
	return FALSE;
    
    boolean ret = TRUE;

    if (GetEOF(file, (long*)&size) != noErr)
	ret = FALSE;

    FSClose(file);
    return ret;
#else
    struct stat sbuf;

    if (stat(name, &sbuf) < 0)
	return FALSE;

#ifdef S_ISDIR
    if (S_ISDIR(sbuf.st_mode))
	return FALSE;
#else
    if ((sbuf.st_mode & S_IFMT) == S_IFDIR)
	return FALSE;
#endif

    size = (size_t)sbuf.st_size;
    return TRUE;
#endif
}
