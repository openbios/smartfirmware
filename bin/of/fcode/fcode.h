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

// (c) Copyright 1999-2001 by CodeGen, Inc.  All Rights Reserved.

// Externs for standard Fcode and other useful Forth words from IEEE-1275.

#ifndef __FCODE_H_
#define __FCODE_H_

#define NULL	0


// Forth return values
//
#define TRUE	(-1)
#define FALSE	(0)

// useful Forth error codes
#define R_OK		0
#define R_ERR		-1
#define R_NO_DATA	-2

#define MAX_PHYS	4		/* defined by IEEE-1275 as 4 */


typedef void (*Xtok)();		// execution token (function pointer)
typedef int Bool;

typedef char Byte;
typedef unsigned char uByte;
typedef short Doublet;
typedef unsigned short uDoublet;
typedef int Quadlet;
typedef unsigned int uQuadlet;

// This is magic for cc-fcode to convert "\$str" to a Forth string type.
typedef struct $fstr
{
	char *str;
	int len;
} Fstr;

// opaque types for readability
typedef char *Instance;
typedef char *Package;


// some other useful stuff to manipulate the Forth stack from C
//
inline void push(int) { asm("( push )"); }
inline int pop(void) { asm("( pop )"); }
inline int top(void) { asm("dup ( top )"); }



// standard FCode functions
//
typedef struct dnum
{
	unsigned int lo;
	int hi;
} Dnum;

typedef struct udnum
{
	unsigned int lo;
	unsigned int hi;
} uDnum;

inline Dnum dplus(Dnum /*d1*/, Dnum /*d2*/) { asm("d+"); }
inline Dnum dminus(Dnum /*d1*/, Dnum /*d2*/) { asm("d-"); }
inline uDnum ummult(unsigned int /*u1*/, unsigned int /*u2*/) { asm("um*"); }
inline uDnum umdivmod(uDnum /*ud*/, unsigned int /*u*/) { asm("um/mod"); }

extern int aligned(int n);
extern void move(const char *src, char *dest, int len);
#define bcopy move
#define memcpy(dest, src, len)	move(src, dest, len)
extern void fill(char *addr, int len, int chr);
#define memset(addr, chr, len)	fill(addr, len, chr)
#define bzero(addr, len)		fill(addr, len, 0)

inline Bool keyq(void) { asm("key?"); }
extern int key(void);
extern int expect(char *addr, int len);
extern int *span(void);
extern int bl(void);
extern void emit(int chr);
extern void type(Fstr);
extern void cr(void);
#define typecr(str) { type(str); cr(); }

extern Fstr count(Fstr);
extern int *base(void);

inline void dot(int) { asm("."); }
inline void udot(unsigned int) { asm("u."); }
inline void dotr(int /*n*/, int /*size*/) { asm(".r"); }
inline void udotr(unsigned int /*u*/, int /*size*/) { asm("u.r"); }
inline void dotd(int) { asm(".d"); }
inline void dots(void) { asm(".s"); }
	inline void stack(void) { asm(".s"); }
inline void dump(char * /*addr*/, int /*len*/)
		{ asm("\" dump\" evaluate"); }

inline void lthash(void) { asm("<#"); }
inline uDnum hash(uDnum /*ud*/) { asm("#"); }
inline uDnum hashs(uDnum /*ud*/) { asm("#s"); }
inline Fstr hashgt(uDnum /*ud*/) { asm("#>"); }
extern void hold(int chr);
extern void sign(int n);

extern Bool within(int n, int min, int max);

extern void evaluate(Fstr);
#ifndef eval
#	define eval evaluate
#endif
extern void execute(Xtok);
extern void abort(void);
extern int catch(Xtok);
extern void throw(int);

extern char *here(void);
inline void ccomma(char) { asm("c,"); }
inline void comma(int) { asm(","); }
inline void compilecomma(Xtok) { asm("compile,"); }
extern Bool *state(void);
inline char *tobody(Xtok) { asm(">body"); }


// other simple FCode words
//
inline int size_c(void) { asm("/c"); }
inline int size_w(void) { asm("/w"); }
inline int size_l(void) { asm("/l"); }
inline int size_n(void) { asm("/n"); }
inline int size_x(void) { asm("/x"); }

extern void off(int *);
extern void on(int *);

inline unsigned int uhash(unsigned int) { asm("u#"); }
inline unsigned int uhashs(unsigned int) { asm("u#s"); }
inline Fstr uhashgt(unsigned int) { asm("u#>"); }

extern Bool comp(char *addr1, char *addr2, int len);

typedef struct bsplit4
{
	Byte lo;
	Byte b2;
	Byte b3;
	Byte hi;
} Bsplit4;

typedef struct bsplit2
{
	Byte lo;
	Byte hi;
} Bsplit2;

typedef struct wsplit2
{
	Doublet lo;
	Doublet hi;
} Wsplit2;

extern Bsplit4 lbsplit(Quadlet);
extern Wsplit2 lwsplit(Quadlet);
extern Bsplit2 wbsplit(Doublet);
extern Quadlet bljoin(Byte blo, Byte b2, Byte b3, Byte bhi);
extern Doublet bwjoin(Byte blo, Byte bhi);
extern Quadlet wljoin(Doublet wlo, Doublet whi);
extern Doublet wbflip(Doublet);
extern void wbflips(Doublet *waddr, int len);
extern Quadlet lbflip(Quadlet);
extern void lbflips(Quadlet *waddr, int len);
extern Quadlet lwflip(Quadlet);
extern void lwflips(Quadlet *waddr, int len);

extern Bool between(int n, int min, int max);

inline Xtok frombody(char *) { asm("body>"); }

extern void noop(void);
extern int bell(void);
extern int bs(void);

inline int *hashline(void) { asm("#line"); }
inline int *hashout(void) { asm("#out"); }

extern Fstr pack(char *str, int len, char *addr);

extern int lcc(int);
extern int upc(int);
inline void pcr(void) { asm("(cr"); }

typedef struct dnum_err
{
	Dnum dn;
	Bool err;
} Dnum_err;

inline Dnum_err dnumber(Fstr /*str*/) { asm("$number dup if 0 -rot then"); }
		// (-- n false | true)

typedef struct digit_ok
{
	int val;
	Bool ok;
} Digit_ok;

extern Digit_ok digit(int chr, int base); // (-- digit true | chr false)

typedef struct find
{
	union
	{
		Fstr str;
		int xt;
	} u;
	Bool ok;
} Find;

inline Find dfind(Fstr /*str*/) { asm("$find dup if over swap then"); }
		// (-- xt true | str len false)

extern char *alloc_mem(int /*len*/);
extern void free_mem(char * /*s*/, int /*len*/);

// definining new Fcode functions
//
extern void instance(void);

typedef struct token_info
{
	int xtoken;
	Bool immediate;
} Token_info;

extern Token_info get_token(int /*fcode*/);
extern void set_token(int /*xtok*/, Bool /*immediate*/, int /*fcode*/);
extern int behavior(int xtok);



// package/instance stuff
//
typedef struct pkg_ok
{
	Package pkg;
	Bool ok;
} Pkg_ok;

extern Pkg_ok find_package(Fstr);
		// (-- phandle true | false)
extern Instance open_package(Fstr /*fstr*/, Package /*pkg*/);
inline Instance dopen_package(Fstr /*arg*/,
		Fstr /*name*/) { asm("$open-package"); }
extern void close_package(Instance);
extern Instance my_self(void);
inline void set_my_self(Instance /*i*/) { asm("to my-self"); }
extern Instance my_parent(void);
inline Package ihandle2phandle(Instance) { asm("ihandle>phandle"); }

typedef struct fstr_ok
{
	Fstr str;
	Bool ok;
} Fstr_ok;

inline Fstr_ok next_property(Fstr /*prevstr*/, Package)
		{ asm("next-property dup 0= if 0 0 then"); }
		// (-- name-str name-len true | false)
extern Package peer(Package);
extern Package child(Package);

typedef struct xtok_ok
{
	int xtok;
	Bool ok;
} Xtok_ok;

inline Bool find_method(Fstr /*method*/, Package)
		{ asm("find-method dup 0= if 0 then"); }
		// (-- xtok true | false)
extern void call_package(Xtok, Instance);
inline void dcall_method(Fstr /*method*/, Instance)
		{ asm("$call-method"); }
inline void dcall_parent(Fstr /*method*/) { asm("$call-parent"); }

typedef struct phys1
{
	int lo;
} Phys1;

typedef struct phys2
{
	int lo;
	int hi;
} Phys2;

typedef struct phys3
{
	int lo;
	int mid;
	int hi;
} Phys3;

typedef struct phys4
{
	int lo;
	int mid1;
	int mid2;
	int hi;
} Phys4;

inline Phys2 my_address1(void) { asm("my-address 0"); }
inline Phys3 my_address2(void) { asm("my-address 0"); }
inline Phys4 my_address3(void) { asm("my-address 0"); }
		// (-- phys.lo ...)

extern int my_space(void);

inline Phys1 my_unit1(void) { asm("my-unit"); }
inline Phys2 my_unit2(void) { asm("my-unit"); }
inline Phys3 my_unit3(void) { asm("my-unit"); }
inline Phys4 my_unit4(void) { asm("my-unit"); }
		// (-- phys.lo ... phys.hi)

extern Fstr my_args(void);

typedef struct two_strs
{
	Fstr right;
	Fstr left;
} Two_fstrs;

extern Two_fstrs left_parse_string(Fstr /*str*/, int /*chr*/);

typedef struct strval
{
	int lo;
	int hi;
} Strval;

extern Strval parse_2int(Fstr /*str*/);

extern char *map_low(int /*lo*/, ... /*...hi, size*/);
extern void free_virtual(char * /*virt*/, int /*size*/);

// Property management
//
extern Fstr encode_int(int);
extern Fstr encode_string(Fstr);
extern Fstr encode_bytes(char * /*addr*/, int /*len*/);
extern Fstr encode_phys(int /*physlo*/, ... /* ...phys.hi */);
inline Fstr encode_phys2(void) { asm("encode-phys"); }
		// (phys.lo ... phys.hi --)
inline Fstr encodeplus(Fstr /*prop1*/, Fstr /*prop2*/)
		{ asm("encode+"); }

typedef struct prop_int
{
	Fstr prop;
	int val;
} Prop_int;

extern Prop_int decode_int(Fstr /*prop*/);

typedef struct prop_phys1
{
	Fstr prop;
	Phys1 phys;
} Prop_phys1;

typedef struct prop_phys2
{
	Fstr prop;
	Phys2 phys;
} Prop_phys2;

typedef struct prop_phys3
{
	Fstr prop;
	Phys3 phys;
} Prop_phys3;

typedef struct prop_phys4
{
	Fstr prop;
	Phys4 phys;
} Prop_phys4;

inline Prop_phys1 decode_phys1(Fstr /*prop*/) { asm("decode-phys"); }
inline Prop_phys2 decode_phys2(Fstr /*prop*/) { asm("decode-phys"); }
inline Prop_phys3 decode_phys3(Fstr /*prop*/) { asm("decode-phys"); }
inline Prop_phys4 decode_phys4(Fstr /*prop*/) { asm("decode-phys"); }
		// (-- prop len phys.lo...phys.hi)

extern Two_fstrs decode_string(Fstr /*prop*/);

extern void property(Fstr prop, Fstr name);
inline void property4(char * /*prop*/, int /*plen*/, char * /*name*/,
	int /*nlen*/) { asm("property"); }
extern void delete_property(Fstr);
extern void device_name(Fstr);
extern void device_type(Fstr);
extern void reg(int physlo, ... /*...physhi, size*/);
extern void model(Fstr);

typedef struct prop_err
{
	Fstr prop;
	Bool err;
} Prop_err;

inline Prop_err get_package_property(Fstr /*name*/, Package)
		{ asm("get-package-property dup if 0 0 rot then"); }
		/* (-- true | prop-addr prop-len false) */
inline Prop_err get_inherited_property(Fstr /*name*/)
		{ asm("get-inherited-property dup if 0 0 rot then"); }
		/* (-- true | prop-addr prop-len false) */
inline Prop_err get_my_property(Fstr /*name*/)
		{ asm("get-my-property dup if 0 0 rot then"); }
		/* (-- true | prop-addr prop-len false) */

extern void new_device(void);
extern void finish_device(void);

// macros for use outside of functions - Forth/Fcode-ism
//
#define DEV(path)			asm("\" dev " path "\" evaluate\n")
#define NEW_DEVICE()		asm("new-device\n")
#define NAME_DEVICE(name, type)	\
	asm("\" " name "\" device-name\n\" " type "\" device-type")
#define FINISH_DEVICE()		asm("finish-device\n")

extern void set_args(Fstr /*arg*/, Fstr /*unit*/);


// words that are needed but are not assigned FCodes
//
inline Instance open_dev(Fstr /*device*/)
		{ asm("\" open-dev\" evaluate"); }
inline void close_dev(Instance /*i*/) { asm("\" close-dev\" evaluate"); }


// Display device support
//
typedef struct font_info
{
	char *addr;
	int width, height;
	int advance, min, numglyphs;
} Font_info;

extern Font_info default_font(void);
extern void set_font(Font_info);
inline char *tofont(int /*chr*/) { asm(">font"); }

extern char *frame_buffer_adr(void);
inline void set_frame_buffer_adr(char *) { asm("to frame-buffer-adr"); }
extern int screen_height(void);
inline void set_screen_height(int) { asm("to screen-height"); }
extern int screen_width(void);
inline void set_screen_width(int) { asm("to screen-width"); }
extern int window_top(void);
inline void set_window_top(int) { asm("to window-top"); }
extern int window_left(void);
inline void set_window_left(int) { asm("to window-left"); }
extern int char_height(void);
inline void set_char_height(int) { asm("to char-height"); }
extern int char_width(void);
inline void set_char_width(int) { asm("to char-width"); }
extern int fontbytes(void);
inline void set_fontbytes(int) { asm("to fontbytes"); }

extern void is_install(Xtok);
extern void is_remove(Xtok);
extern void is_selftest(Xtok);
extern void fb8_install(int /*width*/, int /*height*/, 
		int /*columns*/, int /*lines*/);



// Other FCode words
//
typedef struct peek
{
	int val;
	Bool ok;
} Peek;

inline Peek cpeek(Byte * /*addr*/)
		{ asm("cpeek dup 0= if 0 then"); }
inline Peek wpeek(Doublet * /*addr*/)
		{ asm("wpeek dup 0= if 0 then"); }
inline Peek qpeek(Quadlet * /*addr*/)
		{ asm("qpeek dup 0= if 0 then"); }
extern Bool cpoke(Byte, Byte *addr);
extern Bool wpoke(Doublet, Doublet *addr);
extern Bool qpoke(Quadlet, Quadlet *addr);

inline Bool diagnostic_mode(void) { asm("diagnostic-mode?"); }

extern int get_msecs(void);
extern void ms(int);
extern void alarm(Xtok, int);
extern int user_abort(void);
extern int fcode_revision(void);
extern Fstr mac_address(void);
extern void display_status(int /*n*/);
extern Bool memory_test_suite(char * /*addr*/, int /*len*/);
extern unsigned int mask;

#ifndef DTYPE
#	ifdef DEBUG
#		define DTYPE(str)	type("\$" str "\r\n")
#	else
#		define DTYPE(str)	/* */
#	endif
#endif

#endif // __FCODE_H_
