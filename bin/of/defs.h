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

/* (c) Copyright 1996-1998 by CodeGen, Inc.  All Rights Reserved. */

#ifndef __DEFS_H__
#define __DEFS_H__

#include "stddef.h"
#include "stdlib.h"
#include "string.h"
#include "ctype.h"

#include "machdep.h"
#include "errs.h"

#define SMARTFIRMWARE_REV	"1.2"

/* in case it is not defined */
#ifndef NULL
#	define NULL 0
#endif

/* may not be defined in all <ctype.h> implementations */
#ifndef isblank
#	define isblank(c) ((c) == ' ' || (c) == '\t')
#endif


/* Forth definition of TRUE & FALSE */
#define FTRUE	-1
#define FFALSE	0

/* C definitions of boolean, true, and false */
#ifndef FALSE
#	define FALSE 0
#endif
#ifndef TRUE
#	define TRUE 1
#endif

/* convert an upper-case letter to its equivalent control-character,
   ie CTRL('L') returns ASCII ^L */
#define CTRL(x) ((x) & 0x1F)
#define ESC		033
#define DEL		0177

/* This is used as the base letter for all hexadecimal output.
   It may be overridden to display all numbers in lower-case. */
#ifndef HEX_A
#define HEX_A 'A'
#endif

/* This may be overridden to start nvramrc line numbering at zero */
#ifndef LINE_NUM_START
#define LINE_NUM_START	1
#endif


typedef struct environ Environ;
typedef Retcode (*Command)(Environ *self); /* Forth words & methods */
typedef int (Callback)(Cell *array);	/* callbacks for the client interface */

/* shorthand to declare large numbers of Commands */
#define EC(name) extern Retcode name(Environ *e)
#define CC(name) EC(name); Retcode name(Environ *e)

#ifdef HIDE_STATIC_FUNCS
#	define C(name) static Retcode name(Environ *e)
#else
#	define C(name) EC(name); Retcode name(Environ *e)
#endif


#define INVALID_FCODE	-1 /* invalid value, used to initialize fcode values
							at init time, or to mark uninited defer words */
#define INVALID_XTOK	INVALID_FCODE	/* these are the same */
#define NUM_FCODES		0x1000	/* largest number allowed by OF spec + 1 */
#define FCODE_IMMEDIATE	0x80000000		/* MSB marks immediate fcode */
#define FCODE_MASK		0x7FFFFFFF		/* extract just the fcode part */
#define MAGIC_FNEXT		1		/* defined in OF spec to load Fcode from RAM */


/* type of Entry below */
enum type
{
	T_NONE,
	T_FCODE,		/* sequence of compiled fcode bytes */
	T_EXECTOKEN,	/* an execution token - a single fcode */
	T_FORTH,		/* string containing Forth code to be interpreted */
	T_FUNC,			/* C function */
	T_ADDR,			/* pointer to a variable/field in a C struct */
	T_ADDRVAL,		/* pointer to field in C struct, but used as value word */
	T_CREATE,		/* Forth create word */
	T_DEFER,		/* Forth defer word - points to fcode */
	T_BUFFER,		/* Forth buffer - space is allocated via malloc */
	T_VAR,			/* Forth variable */
	T_CONST,		/* Forth constant */
	T_2CONST,		/* Forth 2constant - use "len" field for 2nd constant */
	T_FIELD,		/* Forth field */
	T_VALUE,		/* Forth value word */
	T_ALIAS,		/* alias - pointer to another Entry */
	T_STRING,		/* p-string */
	T_PROPERTY,		/* an encoded property entry */
	T_TEMP,			/* temporary name created while tokenizing */
	T_CODE,			/* machine-code */
	T_LABEL,		/* machine-code but also push its addr on stack */
	T_REGISTER,		/* machine-dependent register token/pointer */
#ifdef USER_DEF_TYPE
	USER_DEF_TYPE	/* user-defined type may be declared in machdep.h */
#endif
	NUM_TYPES
};

/* special flags for Entry below */
enum flags
{
	F_NONE		= 0x000,	/* no extras - compile the fcode entry directly */
	F_IMMEDIATE	= 0x001,	/* always execute word, even when compiling */
	F_DEBUG		= 0x002,	/* switch on debugging for this word */
	F_TOKENIZE	= 0x004,	/* legit T_FORTH sequence for tokenizer */
	F_INSTANCE	= 0x008,	/* entry is an instance - handle indirectly */
	F_MALLOC	= 0x010,	/* item in union was malloced - needs to be freed */
	F_FCODE32	= 0x020,	/* run entry in 32-bit Fcode compatibilty mode */
	F_STATIC_NAME = 0x040,	/* name is not malloc-ed - do not free */
	F_PAGINATE	= 0x080,	/* turn pagination on/off for this word only */
	F_USER_DEF	= 0x100,	/* user-defined flag may be defined in machdep.h */
	F_HEADERLESS = 0x200,	/* headerless - delete name at end of package */
	F_BUILTIN	= 0x400,	/* word is a builtin - do not delete it */
	END_FLAGS
};

/* one struct used for both Forth Dictionary and Property Lists */
struct entry
{
	Byte *name;				/* lstrdup-ed name of this Forth word */
	struct entry *link;		/* link Entries together into Tables */
	struct entry *hlink;	/* link Entries within hash chain */
	Int xtok;				/* execution token number */
	Short offset;			/* offset for instance vars */
	unsigned int flags : 11; /* really enum flags */
	unsigned int type : 5;	/* really enum type */
	Cell len;				/* length of array/string/packed parameter
								or other type-specific info, ie data for
								2const, or help for built-in words */
	union entryval
	{
		Command cfunc;	/* pointer to C function */
		Byte *fcode;	/* for Fcode and xtok sequences */
		Int xtok;		/* execution token */
		void *addr;		/* for buffers, addresses of variables, code */
		Cell val;		/* value of a variable/const/field/value/register */
		struct entry *alias;	/* pointer to another Entry */
		Byte *array;	/* packed parameter contents and strings */
	#ifdef USER_DEF_ENTRY
		USER_DEF_ENTRY	/* allow insertion of custom data field */
	#endif
	} v;
};
typedef struct entry Entry;

extern Entry *new_entry(void);
extern void delete_entry(Entry *self);
extern Entry *dup_entry(Entry *self);
extern Bool compare_entry(Entry *self, Byte *name, Int len);


/* a table is simply a singly-linked list with an additional hashtable to
   speed up searches */

#ifndef TABLE_HASH
#define TABLE_HASH 97		/* should be a prime to be most effective */
#endif

struct table
{
	Entry *list;
	Int num;
	Entry **hash;	/* if non-NULL, points to array of TABLE_HASH size */
};
typedef struct table Table;

extern Table *new_table(void);
extern void delete_table(Table *self);
extern Table *hash_table(Table *self);
extern Table *dup_table(Table *self);
extern Bool insert_table(Table *t, Entry *e);
extern Bool unhashent_table(Table *tab, Entry *ent);
extern Bool drop_table(Table *tab, Entry *ent);
extern Entry *find_table(Table *table, Byte *name, Int len);


struct package
{
	struct package *parent;		/* parent package */
	struct package *children;	/* list of children packages */
	struct package *link;		/* next sibling in linked-list */
	Table *dict;				/* package-level storage and forth wordlist */
	Table *props;				/* property list */
	struct instance *initinst;	/* instance to initialize new instances */
	struct display *disp;		/* ptr to C vars for display package */
	struct pself *self;			/* ptr to package-specific vars for C code */
};
typedef struct package Package;

extern Package *new_package(Package *parent);
extern void delete_package(Package *p);
extern Package *find_package(Package *pkg, Byte *name, Int len);
extern Package *new_pkg_name(Package *parent, char *name);


struct instance
{
	struct instance *parent;	/* parent instance */
	Cell unit[MAX_ADDR_CELLS];	/* unit address - phys.lo .. phys.hi */
	Cell probe[MAX_ADDR_CELLS];	/* probe address - phys.lo .. phys.hi */
	Int numunits;				/* num of address units in unit[] and probe[] */
	Byte *args;					/* PSTR */
	Package *package;			/* creating package */
	Table *dict;				/* instance-local storage/forth wordlist */
	Bool interposed;			/* instance interposed during open */
	struct self *self;			/* ptr to instance-specific vars for C code */
};
typedef struct instance Instance;

extern Instance *new_instance(Package *pkg);
extern void delete_instance(Instance *self);
extern Instance *dup_instance(Instance *self);


struct alarm
{
	Instance *inst;		/* instance to switch before executing xtok */
	Int xtok;			/* execution token of routine to call */
	uLong ms;			/* intervals between invokations in milliseconds */
	uLong lastms;		/* last time this alarm was executed */
};
typedef struct alarm Alarm;

enum input_type
{
	I_NONE,
	I_FCODE,		/* input is fcode sequence */
	I_STRING,		/* input is a string */
	I_STREAM		/* input is the stdin console */
};

struct input
{
	Int type;		/* really enum input_type */
	Byte *buf;		/* current input buffer */
	Int len;		/* length of current input buffer */
	Cell loc;		/* current location in buffer, if appropriate */
	Int fspread;	/* current spread value for parsing fcode, if fcode */
	Int fnext;		/* xtok to read next fcode byte from address, if fcode */
	Bool foffset;	/* if fcode, current offset value FALSE=8 bits, TRUE=16 */
	struct input *link;	/* previous input stream in stack */
};
typedef struct input Input;

extern void push_input(Environ *e, Input *i);
extern void pop_input(Environ *e);


/* this is to manage the ANSI terminal emulator control-sequence states */
enum emulator_state
{
	S_NONE,		/* have not yet parsed anything interesting */
	S_ESCAPE,	/* just saw an ESC character */
	S_NUM1,		/* just saw ESC [ and are now looking for a number */
	S_NUM2		/* just saw ESC [ # ; and are now looking for another number */
};

enum Flush_state
{
	FO_NONE,		/* do not flush output */
	FO_FLUSH,		/* flush output until the next prompt */
	FO_CAPTURE,		/* flush output capturing text in a ring buffer */
	FO_CONTINUE		/* continue output but do not paginate */
};

/* values for current security-mode */
enum Security_mode
{
	SM_NONE,
	SM_COMMAND,
	SM_FULL,
	SM_NVRAMRC		/* special mode for commands in nvramrc at bootup */
};

enum Debug_mode		/* for Forth debugger */
{
	DB_NONE = 0,		/* no debugging - must be zero */
	DB_TRACE,			/* tracing */
	DB_STEP,			/* single-step */
	DB_TRACE_ALL,		/* tracing everything */
	DB_STEP_ALL			/* single-stepping everything */
};


/* This is used to chain the call stack in do_eval() for ftrace.
 */
struct exec_stack
{
	Entry *code;				/* pointer to Entry being executed */
	struct exec_stack *prev;	/* pointer to calling Entry */
};
typedef struct exec_stack Exec_stack;


/* Most enums are made "Int"s below so that all fields in Environ will be
	word-aligned.  Some compilers will goof up the alignment and subsequent
	offsetof() calculations when we go to store the offsets of the fields
	into T_ADDRs and T_ADDRVALs.  All such fields are made into "Cell"s
	in case cell-size is 64 but pointers and Ints are 32.
 */
struct environ
{
	Table *names;		/* global symbol table - Forth wordlist */
	Package *root;		/* root package for all devices */
	Package *mmu;		/* pointer to "/mmu" for convenience */
	Package *memory;	/* pointer to "/memory" for convenience */
	Package *aliases;	/* pointer to "/aliases" for convenience */
	Package *options;	/* pointer to "/options" for convenience */
	Package *chosen;	/* pointer to "/chosen" for convenience */
	Package *packages;	/* pointer to "/packages" for convenience */
	Package *client;	/* pointer to "/openprom/client-services" */
	Package *currpkg;	/* current open package, if any */
	
	Cell currinst;		/* current instance - "my-self" points to this */
	Cell keyboard;		/* input instance - set during bootup */
	Cell screen;		/* output instance - set during bootup */
	
	Byte *fmem;			/* start of Forth data space */
	Byte *fbrk;			/* current break value in Forth data space */
	Cell *sp;			/* stack pointer - points to topmost cell on stack */
	Cell *rsp;			/* return stack pointer - points to topmost cell */
	Exec_stack *estk;	/* pointer to current execution object */

#ifdef PROPERTY_MEM
	Byte *propmem;		/* start of property memory space */
	Int proplen;		/* currently allocated bytes for properties */
#endif

	Cell radix;			/* "base" - current radix for numeric conversions */
	Int numlen;			/* length of string in numbuf */
	Byte *numptr;		/* current pointer into numbuf -
							decremented as numbers are inserted */

	Cell linesout;		/* "#line" - number of lines since last user input */
	Cell outcol;		/* "#out" - current output column number */
	Cell linesperpage;	/* number of lines per page for pagination, 0 for none,
							negative values are added to (e->lines + 1) */
	Cell scrollstep;	/* number of lines to scroll at end of screen -
							used to speed up scrolling for slower displays */

	Entry **xtoks;		/* points to Entries within names table above */
	Int numxtoks;		/* current number of xtoks in use */
	Int maxxtoks;		/* currently allocated size of xtoks array */
	Int tokfcodes;		/* current number of user-defined fcodes allocated */

	Input in;			/* current input stream - points to previous streams */
						/* - not a pointer so that offsetof() can still work */
	Cell expect_len;	/* used by "expect" and "span" to stash the buf len */
	Int keyloc;			/* current keyboard buffer loc */
	Int keylen;			/* input keyboard buffer length */

	/* stick all Bools together in case they can be packed */
	Bool no_banner;		/* no "probe-all install-console banner" at bootup? */
	Bool showstack;		/* show stack as commands are executed */
	Bool instance;		/* seen magic "instance" keyword/fcode or not? */
	Bool assemble;		/* running assembler, if any is present */
	Bool istokenizing;	/* started tokenizer */
	Bool istemp;		/* temporary compilation in progress */
	Bool cursor;		/* is cursor on right now? */
	Bool fcode32;		/* interpreting Fcode in 32-bit compatibility mode */
	Bool inalarm;		/* currently handling alarms - do not re-execute */
	Bool paginate;		/* auto-pagination turned on or off */

	Int headertok;		/* tokenizing: named-token external-token new-token */

	Int numalarms;		/* current number of alarms set */

	enum Debug_mode debug;		/* Forth debug mode */

	enum Security_mode security;		/* current security mode */
	Bool got_password;	/* got a good password to unlock parser? */

	/* primarily for debugging - capture last N bytes of output before
	   a prompt or crash */
	enum Flush_state flush_output;	
	Byte *capture_buf;	/* pointer to malloc-ed ring buffer */
	Int capture_head;	/* begin reading from this location */
	Int capture_tail;	/* append flushed text at this location */
	#ifndef CAPTURE_BUF_SIZE
	#define CAPTURE_BUF_SIZE	4096
	#endif

	Bool iscompiling;	/* currently compiling? */
	Cell comp_state;	/* Forth bool for "state" - FTRUE if compiling word */
	
	Byte *tempbrk;		/* saved brk value - restored after temp execution */
	Cell *tempsp;		/* sp value to check when to execute temp compile */
	Entry *newdef;		/* new definition */
	
	Byte *load;			/* "load"ed code ready to "go" */
	Int loadlen;		/* length of "load"ed image for convenience */
	Byte *loadargs;		/* args for a "load"ed piece of code */
	struct exec_entry *loadentry;	/* which executable-format was selected */
	uLong entrypoint;	/* entrypoint of executable image - where to launch */
	Cell state_valid;	/* is saved-program-state valid? */
	Bool go_running;	/* TRUE when we "go", FALSE when it returns */

	struct sym_table *loadsyms;	/* symbol table of loaded image */
	struct sym_table *oursyms;	/* symbol table of SmartFirmware image */

	Callback *callback;		/* for "callback" and "sync" */
	Callback *sym2value;	/* callback for defer word "sym>value" */
	Callback *value2sym;	/* callback for defer word "value>sym" */

	Int logo_width;		/* used to specify a builtin logo, if any */
	Int logo_height;

	/* Interposition Recommended Practice - these three fields are stored
	   by the "interpose" word/fcode to be picked up and used when opening 
	   a device tree */
	Package *ipos_package;
	Int ipos_len;
	Byte *ipos_arg;
	
	Cell mask;			/* mask for memory tests */

	/* these are for the display, fb*, and the terminal emulator packages */
	Cell curline;	/* "line#" - current cursor line number in glyphs */
	Cell curcol;	/* "column#" - cursor cursor column number in glyphs */
	Cell inverse;	/* "inverse?" FFALSE or FTRUE - draw a glyph inverted? */
	Cell invscreen;	/* "inverse-screen?" <ditto> - draw background inverted?  */
	Cell lines;		/* "#lines" - number of lines in text display */
	Cell cols;		/* "#columns" - number of columns in text display */
	Cell framebuf;		/* address of frame buffer - driver must init this */
	Int pixsize;		/* pixel size, in bits - driver must init this */
	Int fg;				/* foreground color - driver must init this */
	Int bg;				/* background color - driver must init this */
	Cell swidth;		/* "screen-width" - width of display in pixels */
	Cell sheight;		/* "screen-height" - height of display in pixels */
	Cell wtop;			/* "window-top" - window top border in pixels */
	Cell wleft;			/* "window-left" - window left border in pixels */
	Cell cwidth;		/* "char-width" - width of glyph in pixels */
	Cell cheight;		/* "char-height" - height of glyph in pixels */
	Int cascent;		/* pixels of glyph above baseline - not standard */
	Cell fontbytes;		/* "fontbytes" - bytes between successive scan lines
							in font table */
	Cell escape;		/* escape character to poll user when in long loops */
	Byte *font;			/* pointer to current font table */
	Int glyphs;			/* number of glyphs in current font */
	Int minchar;		/* 1st character in the current font */
	Int emstate;		/* enum emulator_state - current esc-seq parse state */
	Int emval;			/* current number in esc-seq being parsed by emulator */
	Int emval2;			/* second argument in escape-sequence being parsed */
	Int embold;			/* if subsequent color is to be made bold or not */

	/* for editing the nvram script */
	Byte **editbuf;		/* editor buffer */
	Int etop;			/* current topmost line of buffer on screen */
	Int eleft;			/* current leftmost column of buffer on scren */
	Int elines;			/* current number of lines in the buffer */
	Int emax;			/* max number of lines allocated for the buffer */

	struct eself *self;		/* pointer to machdep-specific extras, if any */

	#ifdef CUSTOM_ENVIRON_H
		#include CUSTOM_ENVIRON_H
	#endif

	/* fixed-size buffers: */
	#ifndef MAX_PKG_STACK_SIZE
	#define MAX_PKG_STACK_SIZE	16	/* minimum specified by spec is 8 */
	#endif
	Int pkgtop;						/* current top of package stack */
	Package *pkgstk[MAX_PKG_STACK_SIZE];	/* for push/pop of packages */

	/* for command-line history */
	#ifndef MAX_CMD_HISTORY
	#define MAX_CMD_HISTORY	32		/* must be at least 8 to meet OF spec */
	#endif
	Int hcol;						/* current column in cmdline */
	Byte hyankbuf[STR_SIZE];		/* yank buffer */
	Byte *history[MAX_CMD_HISTORY];

	Int fcodes[NUM_FCODES];			/* map fcodes to xtoks */
	Cell stack[STACK_SIZE];			/* data stack */
	Cell rstack[RET_STACK_SIZE];	/* return stack */

	#ifndef EXEC_STACK_SIZE
	#define EXEC_STACK_SIZE RET_STACK_SIZE
	#endif
	Entry *estack[EXEC_STACK_SIZE];		/* ftrace call stack on error */
	Alarm alarms[MAX_ALARMS];			/* max number of pending alarms */
	Byte keybuf[STR_SIZE];				/* keyboard input buffer */
	Byte numbuf[STR_SIZE];				/* numeric/picture conversion buffer */

	#ifndef MACHINE_STATE_TYPE
	#define MACHINE_STATE_TYPE uInt
	#endif
	MACHINE_STATE_TYPE mstate;		/* machine state */
};

/* loop-sys info on rstack at runtime: (R: -- branch limit index) */
#define LOOP_SIZE		3
#define LOOP_BRANCH		2
#define LOOP_LIMIT		1
#define LOOP_INDEX		0

extern Environ *init_environ(Environ *self);
extern Environ *destroy_environ(Environ *self);


/* this is used just to initialize the "names" table in an Environ */
struct initentry
{
	Byte *name;			/* C-string name of the Forth word */
	Command func;		/* pointer to function, or offset of field in struct */
	Short fcode;		/* -1 for no fcode, so select one dynamically */
	unsigned int flags : 11;	/* flags for Entry */
	unsigned int type : 5;		/* type for Entry, assume T_FUNC if zero */
#ifdef DETAILED_HELP
	Byte *help;			/* stack diagram and help string, if any */
#endif
};
typedef struct initentry Initentry;

/* optionally include help strings in output */
#ifdef DETAILED_HELP
#	define HELP(str)	,str
#else
#	define HELP(str)
#endif


/* initializing Forth words, Fcodes, methods, and packages */
extern Retcode init_entries(Environ *e, Table *table,
		const Initentry initnames[]);
extern Retcode init_names(Environ *e);
extern Retcode install_packages(Environ *e);

/* handle text input from whatever input buffer is currently being used */
extern Int read_char(Environ *e);
extern void unparse_char(Environ *e);
extern Retcode parse(Environ *e, Int delim, Bool whsp, Byte **str, Int *len);
extern Byte *parse_word(Environ *e);	/* deprecated */
extern Retcode parse_word_len(Environ *e, Byte **str, Int *len);
extern Retcode parse_line_len(Environ *e, Byte **str, Int *len);
extern Retcode parse_word_len_refill(Environ *e, Byte **rstr, Int *rlen,
		Bool refill);
extern Byte *parse_line(Environ *e);	/* deprecated */
extern void parse_number(Int radix, Byte **str, Int *len, Cell *val,
		Cell *error, Bool dosign);
extern Bool ask_exit(Environ *e);

/* compile into fcode */
extern Retcode compile_byte(Environ *e, Byte b);
extern Retcode compile_fcode(Environ *e, Int code);
extern Retcode compile_offset(Environ *e, Int o);
extern void patch_offset(Byte *ptr, Int o);
extern Retcode compile_num32(Environ *e, Int o);
#ifdef SF_64BIT
extern Retcode compile_num64(Environ *e, Cell o);
#endif
extern Retcode compile_pointer(Environ *e, Byte *ptr);
extern Retcode compile_str(Environ *e, Byte *str, Int len);
extern void begin_temp_compile(Environ *e);
extern Retcode resolve_temp_compile(Environ *e);
extern Retcode compile_branch(Environ *e, Int code, Int o);
extern Retcode resolve_branch(Environ *e);
extern Retcode compile_of(Environ *e);
extern Retcode compile_endof(Environ *e);
extern Retcode compile_do(Environ *e, Int code);
extern Retcode compile_loop(Environ *e, Int code);

/* dictionary stuff */
extern Byte *falloc(Environ *e, uInt size);	/* alloc Forth memory */
extern Retcode create_newdef(Environ *e);
extern Retcode do_to(Environ *e, Entry *ent);
extern Retcode set_defer(Environ *e, char *name, char *func);

/* parse fcode either during compilation or runtime */
extern Int next_fcode_byte(Environ *e);
extern Int next_fcode_num(Environ *e);
extern Int next_fcode_offset(Environ *e);
extern Int next_fcode_num32(Environ *e);
#ifdef SF_64BIT
extern Cell next_fcode_num64(Environ *e);
#endif
extern Byte *next_fcode_pointer(Environ *e);
extern void next_fcode_string(Environ *e, Byte **str);
extern void fcode_branch(Environ *e, Int o);

/* execution/interpretation runtime */
extern Retcode execute_xtok(Environ *e, Int xtok);
extern Retcode execute(Environ *e, Byte *code, Int len);
extern Retcode execute_method_name(Environ *e, Instance *inst,
		Byte *str, Int len);
extern Retcode execute_static_method_name(Environ *e, Package *pkg,
		Byte *str, Int len);
extern Retcode execute_method(Environ *e, Instance *inst, Int xtok);
extern Retcode execute_word(Environ *e, Byte *word);
extern Retcode interp_fcode(Environ *e, Byte *fcode, Int fnext);
extern Retcode interp_text(Environ *e, Byte *str, Int len);
extern Retcode interpret(Environ *e);

/* configuration variable management */
extern Retcode get_config_val(Environ *e, Byte *param, Int plen,
		Byte **str, Int *slen);
extern Byte *get_config(Environ *e, Byte *param, Int plen);
extern Retcode save_config(Environ *e, Byte *param, Int plen, Byte *value,
		Int vlen, Bool nvsave);
extern Int get_config_bool(Environ *e, Byte *param, Int plen);
extern Cell get_config_int(Environ *e, Byte *param, Int plen);

/* property list management */
extern Byte *prop_alloc(Environ *e, Int len);
extern Retcode prop_decode_int(Byte **arr, Int *len, Int *val);
extern void prop_encode_int(Byte *arr, Int *len, Int val);
extern Retcode prop_get_int(Table *props, Byte *name, Int len, Int *val);
extern Retcode prop_set_int(Table *props, Byte *name, Int len, Int val);
extern Retcode prop_get_ptr(Table *props, Byte *name, Int len, uPtr *val);
extern Retcode prop_set_ptr(Table *props, Byte *name, Int len, uPtr val);
extern Retcode prop_decode_str(Byte **arr, Int *len, Byte **str, Int *slen);
extern void prop_encode_str(Byte *arr, Int *alen, Byte *str, Int slen);
extern Retcode prop_get_str(Table *props, Byte *name, Int len,
		Byte **str, Int *slen);
extern Retcode prop_set_str(Table *props, Byte *name, Int len,
		Byte *str, Int slen);
extern Int get_address_cells(Package *pkg);
extern Int get_size_cells(Package *pkg);
extern Retcode add_property(Table *props, Byte *name, Int len,
		Byte *paddr, Int plen);
extern Retcode set_property(Table *props, Byte *name, Int len,
		Byte *paddr, Int plen);
extern void delete_property(Table *props, Byte *name, Int len);
extern Retcode create_prop_alias(Environ *e, Byte *name, Int len);

/* device stuff */
extern Bool get_device_name(Environ *e, Package *p, Byte *str);
extern Bool get_interposed_device_name(Environ *e, Instance *i, Byte *str);
extern Package *find_device(Environ *e, Byte *str, Int len);
EC(f_find_device);
extern Retcode make_devalias(Environ *e, Byte *name, Int nlen,
		Byte *dev, Int dlen);
extern Retcode boot_load(Environ *e, Byte *dev, Int dlen, Byte *args, Int alen);

/* misc admin stuff */
extern Int diagnostic_mode(Environ *e);
extern Retcode do_testall(Environ *e, Package *node, Bool diag);
extern void spin_cursor(Environ *e);

/* tokenizer */
extern Retcode tokenize(Environ *e, const char *source,  const char *target);
extern Retcode load_file(Environ *e, const char *fname);
extern Retcode detok_file(Environ *e, const char *fname);

/* error handling */
extern int error(Retcode r);
extern Environ *g_e;
extern Retcode g_hwfault;	/* set on hardware or out-of-band fault */
extern const char *err2str(Retcode r);
extern Retcode do_debug(Environ *e, Byte *name, Int nlen, Entry *code);

/* call SF client-interface from OS-dependent wrapper code */
extern Int client_interface(Cell array[]);


/* magic length values to differentiate 3 types of strings */
#define PSTR	-1		/* Pascal-style - 1st byte is length -
							also the internal canonical string form */
#define CSTR	-2		/* C-style - '\0' terminated */
/* anything else is Forth style, with a separate length parameter */

extern void setstrlen(Byte **str, Int *len);
extern Bool compare_strs(Byte *s1, Int l1, Byte *s2, Int l2);

extern Byte *lstrdup(Byte *str, Int len);
#define pstrdup(str)	lstrdup(str, PSTR)
#define cstrdup(str)	lstrdup(str, CSTR)

extern void lstrcpy(Byte *nstr, Byte *str, Int len);
#define pstrcpy(buf, str)	lstrcpy(buf, str, PSTR)
#define cstrcpy(buf, str)	lstrcpy(buf, str, CSTR)

extern Int next_xtok(Environ *e);
extern void set_xtok(Environ *e, Entry *tok);
extern Entry *lookup_sym(Environ *e, Byte *str, Int len);
extern Retcode forget_sym(Environ *e, Byte *str, Int len, Bool all);


/* stack and return stack check & manipulation macros -
	watch out for side-effects - allow overriding these in machdep.h */
#ifndef CKSP
#	define CKSP(e, min, max)	\
		(((min) && (e)->sp < (e)->stack + (min)) || \
				((max) && (e)->sp + (max) >= (e)->stack + STACK_SIZE))
#endif
#ifndef CKRETSP
#	define CKRETSP(e, min, max)	\
		(((min) && (e)->rsp < (e)->rstack + (min)) || \
				((max) && (e)->rsp + (max) >= (e)->rstack + RET_STACK_SIZE))
#endif

#ifndef IFCKSP
#	define IFCKSP(e, min, max)		\
		if (((min) && (e)->sp < (e)->stack + (min))) \
			return E_STACK_UNDERFLOW; \
		else if ((max) && (e)->sp + (max) >= (e)->stack + STACK_SIZE) \
			return E_STACK_OVERFLOW
#endif
#ifndef IFCKRETSP
#	define IFCKRETSP(e, min, max)	\
		if ((min) && (e)->rsp < (e)->rstack + (min)) \
			return E_RSTACK_UNDERFLOW; \
		else if ((max) && (e)->rsp + (max) >= (e)->rstack + RET_STACK_SIZE) \
			return E_RSTACK_OVERFLOW
#endif

#define BUMPSP(e)		(++(e)->sp)
#define DROPSP(e)		(--(e)->sp)

#define BUMP_RETSP(e)	(++(e)->rsp)
#define DROP_RETSP(e)	(--(e)->rsp)

#define TOP(e)			(*(e)->sp)
#define RTOP(e)			(*(e)->rsp)

#define STACK(e, n)		((e)->sp[-(n)])
#define RSTACK(e, n)	((e)->rsp[-(n)])

#define PUSH(e, val)			(BUMPSP(e), (*(e)->sp = (Cell)(val)))
#define POP(e, var)				(((var) = TOP(e)), DROPSP(e))
#define POPTYPE(e, var, type)	(((var) = (type)(uPtr)TOP(e)), DROPSP(e))
#define DROP(e)					(DROPSP(e))
#define DROPN(e, n)				((e)->sp -= (n))

/* allow overriding these two macros to support mapping 64<->32 pointers */
#ifndef PUSHP
#  define PUSHP(e, val)			(BUMPSP(e), (*(e)->sp = (Cell)(uPtr)(val)))
#endif
#ifndef POPT
#  define POPT(e, var, type)	(((var) = (type)(uPtr)TOP(e)), DROPSP(e))
#endif

#define RPUSH(e, val)			(BUMP_RETSP(e), (*(e)->rsp = (Cell)(val)))
#define RPOP(e, var)			(((var) = RTOP(e)), DROP_RETSP(e))
#define RDROP(e)				(DROP_RETSP(e))
#define RDROPN(e, n)			((e)->rsp -= (n))

/* first of these macros must sign-extend after masking upper 32-bits -
   second macro must NOT sign-extend */
#ifdef SF_64BIT
#	define DO_MASK32(e, val)	\
		if (e->fcode32) (val) = (Quadlet)((val) & QUADLET_MASK)
#	define DO_UMASK32(e, val)	if (e->fcode32) (val) &= QUADLET_MASK
#else
#	define DO_MASK32(e, val)	/* noop */
#	define DO_UMASK32(e, val)	/* noop */
#endif


/* machine-independent struct used to call machine-dependent timing routines */
struct time_value
{
	uInt sec;		/* number of seconds */
	uInt nsec;		/* number of nano-seconds */
};
typedef struct time_value Time_value;


/* handle endian difficulties using macros - no 64-bit support yet */
#define SWAP2(v)	((((v) & BYTE_MASK) << BYTE_SIZE) | \
					(((v) & (BYTE_MASK << BYTE_SIZE)) >> BYTE_SIZE))
#define SWAP4(v)	((((v) & BYTE_MASK) << (BYTE_SIZE * 3)) | \
					(((v) & (BYTE_MASK << BYTE_SIZE)) << BYTE_SIZE) | \
					(((v) & (BYTE_MASK << (BYTE_SIZE * 2))) >> BYTE_SIZE) | \
					(((v) & (BYTE_MASK << (BYTE_SIZE * 3))) >> (BYTE_SIZE * 3)))
extern uShort swap2(uShort v);
extern uInt swap4(uInt v);

#ifndef LE2
#ifdef LITTLE_ENDIAN
#	define LE2(v)		((uShort)v)
#	define LE4(v)		((uInt)v)
#	define BE2(v)		swap2(v)
#	define BE4(v)		swap4(v)
#else /* BIG_ENDIAN */
#	define LE2(v)		swap2(v)
#	define LE4(v)		swap4(v)
#	define BE2(v)		((uShort)v)
#	define BE4(v)		((uInt)v)
#endif /* BIGENDIAN */
#endif


/* machine-dependent I/O interfaces */
extern Retcode machine_initialize(void);
extern Retcode machine_init_args(Environ *e, int argc, char *argv[]);
extern Retcode machine_reset_all(Environ *e);
extern void machine_led_write(Environ *e, Int n);
extern Bool machine_probe_read(Environ *e, Cell addr, Cell *value, int size);
extern Bool machine_probe_write(Environ *e, Cell addr, Cell value, int size);
extern Bool machine_pci_translate(Cell addr, Int *pci_addr, Bool *io);
extern void machine_reg_read(Environ *e, Cell addr, Cell *value, int size);
extern void machine_reg_write(Environ *e, Cell addr, Cell value, int size);
extern void machine_unalign_read(Environ *e, Cell addr, Cell *value, int size);
extern void machine_unalign_write(Environ *e, Cell addr, Cell value, int size);
extern void machine_gettime(Environ *e, Time_value *tv);
extern Bool machine_memory_test(Environ *e, Cell addr, Cell len,
		Cell mask, Bool diag);
extern Bool machine_diag_switch(Environ *e);
extern void machine_test_begin(Environ *e);
extern void machine_test_pass(Environ *e);
extern void machine_test_fail(Environ *e);
extern Byte *new_nvram(Environ *e, Byte *param, Int plen, Int maxlen);
extern Byte *get_nvram(Environ *e, Byte *param, Int plen);
extern Byte *get_nvram_default(Environ *e, Byte *param, Int plen);
extern Retcode set_nvram(Environ *e, Byte *param, Int plen,
		Byte *val, Int vlen);
extern Retcode reset_nvram(Environ *e);
extern Int failsafe_write(Byte *buf, Int len);
extern Int failsafe_read(Byte *buf, Int len);
extern uInt machine_nvram_size(Environ *e);
extern Retcode machine_nvram_read(Environ *e, uChar *buf, uInt *len);
extern Retcode machine_nvram_write(Environ *e, uChar *buf, uInt len);

/* machine dependent MMU interfaces */
extern Retcode mmu_map(Environ *e, uInt phys[], Cell virt, Cell size, uInt mode);
extern Retcode mmu_unmap(Environ *e, Cell virt, Cell size);
extern Retcode mmu_claim(Environ *e, Cell *virt, Cell align, Cell size);
extern Retcode mmu_release(Environ *e, Cell virt, Cell size);
extern Retcode mmu_modify(Environ *e, Cell virt, Cell size, uInt mode);
extern Retcode mmu_translate(Environ *e, Cell virt, uInt phys[], uInt *mode);

extern Retcode mmu_map_in(Environ *e, uInt phys[], Cell size, Int type,
	Cell *virt);
extern Retcode mmu_map_out(Environ *e, Cell virt, Cell size);
extern Retcode mmu_dma_alloc(Environ *e, Cell size, Cell *virt);
extern Retcode mmu_dma_free(Environ *e, Cell virt, Cell size);
extern Retcode mmu_dma_map_in(Environ *e, Cell virt, Cell size, Int type,
		uInt phys[]);
extern Retcode mmu_dma_map_out(Environ *e, Cell virt, uInt phys[], Cell size);
extern Retcode mmu_dma_sync(Environ *e, Cell virt, uInt phys[], Cell size);

#define MMU_MAP_IN_DATA			0x0001
#define MMU_MAP_IN_EXEC			0x0002
#define MMU_MAP_IN_IO			0x0004

#define MMU_DMA_MAP_IN_NOCACHE		0x0001
#define MMU_DMA_MAP_IN_CACHEABLE	0x0002

/* machine-dependent load/execution interfaces */
EC(machine_probe_all);
EC(machine_secondary_diag);
EC(machine_init_program);
EC(machine_go);
EC(machine_init_load);

#ifdef MACHINE_CODE
extern Retcode machine_code_jump(Environ *e, void *addr);
extern Retcode machine_compile_return(Environ *e);
#endif

#ifdef ASSEMBLER
extern Entry *lookup_asm(Environ *e, Byte *word, Int wlen);
#endif

#ifdef CPU_REGISTERS
/* CPU-register read/write routines - "reg" is machine-dependent token */
extern Retcode cpu_register_read(Environ *e, Cell reg, Cell *value);
extern Retcode cpu_register_write(Environ *e, Cell reg, Cell value);
#endif

/* must call this to setup a malloc memory pool */
extern Retcode init_malloc(void *memory, size_t max);
extern void meminfo(size_t *allocspc, size_t *freespc, size_t *allocblks,
	size_t *freeblks, void **endarena, size_t *arenasz);

/* these are defined and initialized in machdep.c and should be
	considered read-only vars */
extern Byte *g_machine_memory;
extern uInt g_machine_memory_size;
extern uInt g_machine_memory_offset;
extern uInt g_machine_memory_used;

/* machine-independent I/O */
extern int bprintf(char *buf, const char *fmt, ...);
extern char *rbprintf(const char *fmt, ...);
extern void cprintf(Environ *e, const char *fmt, ...);		/* console printf */
extern int key_down(Environ *e);
extern int get_key(Environ *e);
extern void expect_line(Environ *e, Byte *addr, Int len,
		Int *elen, Bool prompt);
extern void display_char(Environ *e, int c);
extern void display_text(Environ *e, Byte *text, Int len);
extern void display_stack(Environ *e);
extern void display_rstack(Environ *e);
extern void display_ftrace(Environ *e);
extern void display_exec_stack(Environ *e);
extern void display_memory(Environ *e, Byte *addr, Int len);
extern Retcode display_number(Environ *e, Int radix, Bool sign, Bool dowidth);

/* security functions */
extern enum Security_mode security_mode(Environ *e);
extern void get_password(Environ *e, Byte *addr, Int len, Int *elen);
extern Bool check_password(Environ *e);

/* misc machine-dependent stuff */
extern int machine_callback(Environ *e, Callback *func, Cell *array);
extern void dprintf(const char *fmt, ...);	/* for debugging, if necessary */

/* handy micro-second sleep routine for convenience of C drivers */
extern void u_sleep(uInt ms);

/* return current time since bootup in milliseconds - for C drivers */
extern uLong get_msecs(Environ *e);

/* general purpose allocator API */
struct allocator_block
{
	struct allocator_block *next;
	uInt addr[1];
};

typedef struct allocator_block Allocator_block;

struct allocator
{
	int acells;
	int scells;
	int bsize;
	Allocator_block *list;
	uInt *t1;
	uInt *t2;
	uInt *u1;
	uInt *u2;
	uInt *u3;
	uInt x[1];
};

typedef struct allocator Allocator;

extern Retcode alloc_init(Allocator **aptr, int acells, int scells);
extern Retcode alloc_fini(Allocator *a);
extern Retcode alloc_fixed(Allocator *a, uInt addr[], uInt size[]);
extern Retcode alloc_constrained(Allocator *a, uInt min[], uInt max[],
		uInt align[], uInt mask[], uInt size[], uInt addr[]);
extern Retcode alloc_aligned(Allocator *a, uInt align[], uInt size[],
		uInt addr[]);
extern Retcode alloc_block(Allocator *a, uInt size[], uInt addr[]);
extern Retcode free_block(Allocator *a, uInt addr[], uInt size[]);
extern Retcode find_constrained(Allocator *a, uInt min[], uInt max[],
		uInt align[], uInt mask[], uInt addr[], uInt size[]);
extern Retcode alloc_info(Allocator *a, Allocator_block **bptr, uInt addr[],
		uInt size[]);

/* define DEBUG before including "defs.h" to use this macro - allow
   machdep.h to override this as well */
#ifndef DPRINTF
#	ifdef DEBUG
#		define DPRINTF(args) dprintf args
#	else
#		define DPRINTF(args)
#	endif
#endif


#endif /* __DEFS_H__ */
