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

#ifndef	_KEYMAP_H_
#define	_KEYMAP_H_


typedef unsigned char Keychar;
declare_array(Keystrokes, Keychar, 64);

struct Keyinfo;
class Keybinding;
class Map;
class Keymap;
class Macro;


typedef int (Keyfunc)(Keyinfo &k);


struct Keyinfo
{
	Keychar lastchar;
	Keychar *lastseq;
	long lastlen;
	Keyfunc *lastcmd;
	Keyfunc *thiscmd;
	boolean error;

	boolean newarg;
	int numargs;
	int arg, arg2, arglen;
	Keychar *argstr;

	Keyinfo();
};

extern const Keychar *key2seq(int key, ...);

class Keymap
{
	friend class Keybinding;
	enum Findret { NO_BINDING, SHORT_BINDING, GOT_BINDING };
	enum Keybinds { K_UNBIND, K_BIND, K_DEFAULT };

	Map *map;
	Keymap *parent;
	Keystrokes keyseq;
	Keyinfo info;

	Findret find(Keystrokes &seq, Keybinding *&kb);
	Keybinding *make(Keystrokes &seq, boolean def = FALSE);
	boolean dobind(Keybinding &kb, const Keychar *seq, long len,
			Keymap::Keybinds what);

public:
	void debugdump();

	Keymap(Keymap &p);
	Keymap();
	~Keymap();

	Keymap &operator=(Keymap &kmap);

	boolean eval(const Keychar *seq = NULL, int len = -1);
	void clear() { keyseq.reset(); }

	boolean unbind(const Keychar *seq, int len = -1);
	boolean bind(Keyfunc *func, const Keychar *seq, int len = -1);
	boolean defaultbind(Keyfunc *func, const Keychar *seq, int len = -1);

	// these should be "Keyfunc**" but the C++ compiler complains...
	boolean bind(void *funcptr, const Keychar *seq, int len = -1);
	boolean defaultbind(void *funcptr, const Keychar *seq, int len = -1);

	boolean bind(Keyfunc *func, int k1)
		{ return bind(func, key2seq(k1, 0)); }
	boolean bind(Keyfunc *func, int k1, int k2)
		{ return bind(func, key2seq(k1, k2, 0)); }
	boolean bind(Keyfunc *func, int k1, int k2, int k3)
		{ return bind(func, key2seq(k1, k2, k3, 0)); }
	boolean bind(Keyfunc *func, int k1, int k2, int k3, int k4)
		{ return bind(func, key2seq(k1, k2, k3, k4, 0)); }
	boolean bind(Keyfunc *func, int k1, int k2, int k3, int k4, int k5)
		{ return bind(func, key2seq(k1, k2, k3, k4, k5, 0)); }

	boolean defaultbind(Keyfunc *func)
		{ return defaultbind(func, key2seq('x', 0)); }
	boolean defaultbind(Keyfunc *func, int k1)
		{ return defaultbind(func, key2seq(k1, 'x', 0)); }
	boolean defaultbind(Keyfunc *func, int k1, int k2)
		{ return defaultbind(func, key2seq(k1, k2, 'x', 0)); }
	boolean defaultbind(Keyfunc *func, int k1, int k2, int k3)
		{ return defaultbind(func, key2seq(k1, k2, k3, 'x', 0)); }
	boolean defaultbind(Keyfunc *func, int k1, int k2, int k3, int k4)
		{ return defaultbind(func, key2seq(k1, k2, k3, k4, 'x', 0)); }

	#ifdef DLD
	boolean bind(const char *fname, const Keychar *seq, int len = -1)
		{ return bind((void *)dldgetfunc(fname), seq, len); }
	boolean defaultbind(const char *fname, const Keychar *seq, int len=-1)
		{ return defaultbind((void *)dldgetfunc(fname), seq, len);}
	#endif
};


#endif	/* _KEYMAP_H_ */
