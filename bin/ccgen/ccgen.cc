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
#include <string.h>
#include <ctype.h>

char *
stripcomment(char *str)
{
	char *sp = str;
	int instring = 0;
	int inquote = 0;

	while (*sp != '\0')
		switch (*sp)
		{
		case '"':
			if (!inquote)
				instring = !instring;

			sp++;
			break;
		case '\'':
			if (!instring)
				inquote = !inquote;

			sp++;
			break;
		case '\\':
			if (inquote || instring)
			{
				// skip escape
				if (isdigit(sp[1]))
				{
					if (sp[2] == 'x' || sp[2] == 'X')
					{
						if (isxdigit(sp[3]) && isxdigit(sp[4]))
							sp += 5;
						else
							sp += 2;
					}
					else if (isdigit(sp[2]) && isdigit(sp[3]))
						sp += 4;
					else
						sp += 2;
				}
				else
					sp += 2;
			}
			else
				sp++;

			break;
		case '/':
			if (!instring && !inquote && sp[1] == '/')
			{
				while (sp > str && isspace(sp[-1]))
					sp--;

				*sp = '\0';
				return str;
			}

			sp++;
			break;
		default:
			sp++;
			break;
		}

	while (sp > str && isspace(sp[-1]))
		sp--;

	*sp = '\0';
	return str;
}

int
main()
{
	int inmacro = 0;
	int incomment = 0;
	char str[1024];

	while (fgets(str, 1024, stdin) != NULL)
	{
		if (strncmp(str, "//$begincomment", 15) == 0)
		{
			if (inmacro)
				printf("\\\n");
			else
				printf("\n");

			incomment = 1;
		}
		else if (strncmp(str, "//$endcomment", 13) == 0)
		{
			if (inmacro)
				printf("\\\n");
			else
				printf("\n");

			incomment = 0;
		}
		else if (incomment)
		{
			if (inmacro)
				printf("\\\n");
			else
				printf("\n");
		}
		else if (strncmp(str, "//$begin ", 9) == 0)
		{
			printf("#define %s \\\n", stripcomment(&str[9]));
			inmacro = 1;
		}
		else if (strncmp(str, "//$end", 6) == 0)
		{
			printf("\n");
			inmacro = 0;
		}
		else if (inmacro)
			printf("%s \\\n", stripcomment(str));
		else
			printf("%s\n", stripcomment(str));
	}
	return 0;
}
