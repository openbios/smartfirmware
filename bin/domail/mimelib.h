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


#ifndef __MIMELIB_H__
#define __MIMELIB_H__

struct mime;
typedef struct mime MIME;

#define MIME_VERS		512
#define MIME_MSGID		513
#define MIME_BOH		514
#define MIME_EOH		515
#define MIME_BOP		516
#define MIME_EOP		517
#define MIME_BOB		518
#define MIME_EOB		519
#define MIME_BOCH		520
#define MIME_EOCH		521
#define MIME_BOC		522
#define MIME_EOC		523
#define MIME_BOE		524
#define MIME_EOE		525
#define MIME_BOT		526
#define MIME_EOT		527
#define MIME_TAG		528

#define MIME_NOCRACK		0x0000
#define MIME_HEADERS		0x0001
#define MIME_PREAMBLES		0x0002
#define MIME_BOUNDARIES		0x0004
#define MIME_CONTENTHEADERS	0x0008
#define MIME_CONTENT		0x0010
#define MIME_EPILOGS		0x0020
#define MIME_CRACKHTML		0x0040
#define MIME_HTMLTAGS		0x0080

MIME *mime_open(FILE *fp, int flags);
void mime_close(MIME *m);
int mime_getch(MIME *m);
void mime_ungetch(MIME *m, int ch);

char *mime_headername(MIME *m);
char *mime_contenttype(MIME *m);
char *mime_contentsubtype(MIME *m);
char *mime_transferencoding(MIME *m);
char *mime_charset(MIME *m);
char *mime_contentparameter(MIME *m, char *header, char *param);
char *mime_html_tagname(MIME *m);
int mime_html_attributes(MIME *m);
char *mime_html_attributename(MIME *m, int n);
char *mime_html_attributevalue(MIME *m, int n);

#endif /* __MIMELIB_H__ */
