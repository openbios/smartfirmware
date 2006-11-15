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

#include "defs.h"
#include "keymap.h"
#include <stdarg.h>


struct Keybinding
{
	enum Type
	{
		K_NONE = 0,
		K_FUNC,
		K_FUNCPTR,
		K_MAP,
		K_KEYMAP,
		K_MACRO,
		K_USERDEF,
	};

	short key;
	char type;
	union
	{
		Map *map;
		Keymap *kmap;
		Keyfunc *func;
		Keyfunc **funcptr;
		Macro *macro;
		void *userdef;
	};

	Keybinding();
	~Keybinding();
	Keybinding(Keybinding & kb);
	Keybinding(short k);
	void operator=(Keybinding & kb);
	boolean operator==(short k) { return key == k; }
	operator short() { return key; }

	void unbind();
	Map *makemap();
};

inline unsigned keyhash(short k) { return k; }

const int DEFAULTKEY = -1;
// key == -1 is the "default" binding, and MUST be an illegal Keychar!!!
declare_hashtable(Map, MapIter, Keybinding, short);
implement_hashtable(Map, MapIter, Keybinding, short, keyhash);

implement_array(Keystrokes, Keychar, 64);


Keybinding::Keybinding()
{
	type = K_NONE;
	map = NULL;
	key = DEFAULTKEY;
}

void
Keybinding::unbind()
{
	if (type == K_MAP)
		delete map;

	type = K_NONE;
	map = NULL;
}

Map *
Keybinding::makemap()
{
	if (type == K_MAP)
		return map;

	if (type == K_KEYMAP)
		return kmap->map;

	unbind();
	type = K_MAP;
	return map = new Map;
}

Keybinding::Keybinding(Keybinding &kb)
{
	type = K_NONE;
	map = NULL;
	*this = kb;
}

Keybinding::Keybinding(short k)
{
	type = K_NONE;
	map = NULL;
	key = k;
}

Keybinding::~Keybinding()
{
	unbind();
}

void
Keybinding::operator=(Keybinding &kb)
{
	unbind();

	switch (kb.type)
	{
	case K_MAP:
		map = new Map();
		*map = *kb.map;

	Case K_KEYMAP:
		kmap = kb.kmap;

	Case K_FUNC:
		func = kb.func;

	Case K_FUNCPTR:
		funcptr = kb.funcptr;
	}

	type = kb.type;
	key = kb.key;
}


Keyinfo::Keyinfo()
{
	lastchar = '\0';
	lastseq = NULL;
	lastlen = 0;
	lastcmd = thiscmd = NULL;
	error = FALSE;

	newarg = FALSE;
	numargs = 0;
	arg = 0;
	arg2 = -1;
	arglen = 0;
	argstr = NULL;
}

static void
Mapdebug(Map &map, int level = 0)
{
	for (MapIter mi = map; mi; mi++)
	{
		for (int i = 0; i < level; i++)
			debug("   ");

		Keybinding & b = mi();
		debug("%3d> ", b.key);

		switch (b.type)
		{
		case Keybinding::K_MAP:
			debug("MAP:\n");
			Mapdebug(*b.map, level + 1);

		Case Keybinding::K_KEYMAP:
			debug("KEYMAP  0x%X\n", b.kmap);

		Case Keybinding::K_FUNC:
			debug("FUNC    0x%X\n", b.func);

		Case Keybinding::K_FUNCPTR:
			debug("FUNCPTR  0x%X\n", b.funcptr);

		Case Keybinding::K_MACRO:
			debug("MACRO    0x%X\n", b.macro);

		Case Keybinding::K_USERDEF:
			debug("USERDEF  0x%X\n", b.userdef);

		Case Keybinding::K_NONE:
			debug("<NONE>\n");

		Default:
			debug("<%d>\n", b.type);
		}
	}
}



Keymap::Keymap(Keymap &p)
{
	parent = &p;
	map = new Map;
}

Keymap::Keymap()
{
	parent = NULL;
	map = new Map;
}

Keymap::~Keymap()
{
	delete map;
}


void
Keymap::debugdump()
{
	debug("Keymap:\n");
	Mapdebug(*map);
}

Keymap &
Keymap::operator=(Keymap & kmap)
{
	map = kmap.map;
	return *this;
}

static int
seqlen(const Keychar *seq)
{
	int len = 0;

	for (; *seq != 0; seq++)
		len++;

	return len;
}

static void
seqcpy(Keystrokes & kseq, const Keychar *seq, long len)
{
	kseq.reset(len);

	for (long i = 0; i < len; i++)
		kseq[i] = *seq++;
}

Keymap::Findret
Keymap::find(Keystrokes &seq, Keybinding *&kb)
{
	long len = seq.size();

	if (len <= 0)
		return NO_BINDING;

	kb = NULL;

	int i;
	short ch = 0;
	Map *k = map;

	for (i = 0; i < len; i++)
	{
		ch = seq[i];

		if (!k->find(ch) || (*k)().type == Keybinding::K_NONE)
			ch = DEFAULTKEY;

		kb = &(*k)[ch];

		if (kb->type == Keybinding::K_MAP)
			k = kb->map;
		else if (kb->type == Keybinding::K_KEYMAP)
			k = kb->kmap->map;
		else
			break;
	}

	if (kb == NULL || i < len - 1)
		return NO_BINDING;

	if (kb->type == Keybinding::K_MAP || kb->type == Keybinding::K_KEYMAP)
		return SHORT_BINDING;

	return GOT_BINDING;
}

Keybinding *
Keymap::make(Keystrokes &seq, boolean def)
{
	long len = seq.size();

	if (len <= 0)
		return NULL;

	Map *k = map;
	int i;
	short ch = seq[0];

	for (i = 0; i < len - 1; i++)
	{
		if (!k->find(ch) || (*k)().type == Keybinding::K_NONE)
		{
			if (k->find(DEFAULTKEY) && ((*k)().type == Keybinding::K_MAP
						|| (*k)().type == Keybinding::K_KEYMAP))
				ch = DEFAULTKEY;
		}

		k = (*k)[ch].makemap();
		ch = seq[i + 1];
	}

	if (def)
		ch = DEFAULTKEY;

	return &(*k)[ch];
}

boolean
Keymap::eval(const Keychar *seq, int len)
{
	if (len < 0 && seq != NULL)
		len = seq != NULL ? seqlen(seq) : 1;

	boolean ret = OK;
	boolean nobindings = TRUE;

	for (int i = 0; i < len; i++)
	{
		if (seq != NULL)
			keyseq[keyseq.size()] = seq[i];

		for (Keymap * m = this; m != NULL; m = m->parent)
		{
			Keybinding *map;
			Findret fr = find(keyseq, map);

			if (fr != GOT_BINDING)
			{
				if (fr == SHORT_BINDING)
					nobindings = FALSE;

				continue;
			}
			if (map->type != Keybinding::K_FUNC
					&& map->type != Keybinding::K_FUNCPTR)
				continue;

			info.error = OK;
			info.lastseq = keyseq.getarr();
			info.lastlen = keyseq.size();
			info.lastchar = keyseq[keyseq.size() - 1];
			info.lastcmd = info.thiscmd;
			info.thiscmd = (map->type == Keybinding::K_FUNC)
					? map->func : *map->funcptr;

			boolean err = (boolean)info.thiscmd(info);

			if (!info.newarg)
			{
				info.numargs = 0;
				info.arg = 0;
				info.arg2 = DEFAULTKEY;
				info.arglen = 0;
				info.argstr = NULL;
			}

			info.newarg = FALSE;
			keyseq.reset();
			return err || info.error;
		}
	}

	if (nobindings)
	{
		keyseq.reset();
		return ERR;
	}

	return ret;
}

boolean
Keymap::dobind(Keybinding &kb, const Keychar *seq,
    long len, Keybinds what)
{
	if (len < 0)
		len = seqlen(seq);

	if (len <= 0)
		return ERR;

	Keystrokes kseq(len);
	seqcpy(kseq, seq, len);

	Keybinding *m;

	if (what == K_UNBIND)
		(void)find(kseq, m);
	else if (what == K_BIND)
		m = make(kseq, FALSE);
	else if (what == K_DEFAULT)
		m = make(kseq, TRUE);
	else
		return ERR;

	if (m == NULL)
		return ERR;

	if (what == K_UNBIND)
		m->unbind();
	else
	{
		kb.key = m->key;
		*m = kb;
	}

	return OK;
}


boolean
Keymap::bind(Keyfunc * func, const Keychar *seq, int len)
{
	Keybinding kb;
	kb.func = func;
	kb.type = Keybinding::K_FUNC;
	return dobind(kb, seq, len, K_BIND);
}

boolean
Keymap::defaultbind(Keyfunc * func, const Keychar *seq, int len)
{
	Keybinding kb;
	kb.func = func;
	kb.type = Keybinding::K_FUNC;
	return dobind(kb, seq, len, K_DEFAULT);
}

boolean
Keymap::bind(void *funcptr, const Keychar *seq, int len)
{
	Keybinding kb;
	kb.funcptr = (Keyfunc **)funcptr;
	kb.type = Keybinding::K_FUNCPTR;
	return dobind(kb, seq, len, K_BIND);
}

boolean
Keymap::defaultbind(void *funcptr, const Keychar *seq, int len)
{
	Keybinding kb;
	kb.funcptr = (Keyfunc **)funcptr;
	kb.type = Keybinding::K_FUNCPTR;
	return dobind(kb, seq, len, K_DEFAULT);
}

static Keystrokes mksq;

const Keychar *
key2seq(int key,...)
{
	va_list ap;
	va_start(ap, key);
	mksq.reset();

	for (int arg = key; arg != 0; arg = va_arg(ap, int))
		mksq[mksq.size()] = arg;

	va_end(ap);
	mksq[mksq.size()] = 0;
	return mksq.getarr();
}
