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

/* Copyright (c) 1990,1999 by Parag Patel.  All Rights Reserved. */

#ifndef	_BUFVAR_H_
#define	_BUFVAR_H_

enum Bufvartype
{
	V_NONE = 0,
	V_USERDEF,
	V_NUM,
	V_STRING,
	V_REAL,
	V_BUFFER,
};

class Buffer;

class Varvalue
{
	Bufvartype type;
	union
	{
		void *userdef;
		char *str;
		long num;
		double real;
		Buffer *buf;
	};

public:
	~Varvalue();
	Varvalue();
	Varvalue(Varval &);
	Varvalue(void *);
	Varvalue(char *);
	Varvalue(long);
	Varvalue(double);
	Varvalue(Buffer *);

	Varvalue &operator=(Varval &);
	Varvalue &operator=(void *);
	Varvalue &operator=(char *);
	Varvalue &operator=(long);
	Varvalue &operator=(double);
	Varvalue &operator=(Buffer *);

	operator void*();
	operator char*();
	operator long();
	operator double();
	operator Buffer*();
};
declare_array(Valuelist, Varvalue, 1024);


class Bufvar
{
	char *name;
	int id;

public:
	Varvalue v;

	Bufvar(const char *name);
	~Bufvar();
	boolean operator==(const char *key);
	operator char*();
	Varvalue &operator[]();
};
declare_sorttable(Bufvartable,Bufvar,char*);

extern Bufvartable Bufvarlist;


class Bufvals
{
	Valuelist vals;

public:
	Varvalue &operator[](const char *var);
	Varvalue &operator[](int id);
	int id(char *var);
};



#endif	/* _BUFVAR_H_ */
