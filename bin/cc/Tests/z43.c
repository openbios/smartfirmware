typedef void *Symbol;
typedef void *Env;
typedef void *Value;
typedef void *Node;
typedef void *Coordinate;
typedef void *Xinterface;

typedef struct metrics {
	unsigned char size, align, outofline;
} Metrics;
typedef struct interface {
	Metrics charmetric;
	Metrics shortmetric;
	Metrics intmetric;
	Metrics floatmetric;
	Metrics doublemetric;
	Metrics ptrmetric;
	Metrics structmetric;
	unsigned little_endian:1;
	unsigned mulops_calls:1;
	unsigned wants_callb:1;
	unsigned wants_argb:1;
	unsigned left_to_right:1;
	unsigned wants_dag:1;

void (*address)(Symbol p, Symbol q, int n);
void (*blockbeg)(Env *);
void (*blockend)(Env *);
void (*defaddress)(Symbol);
void (*defconst)  (int ty, Value v);
void (*defstring)(int n, char *s);
void (*defsymbol)(Symbol);
void (*emit)    (Node);
void (*export)(Symbol);
void (*function)(Symbol, Symbol[], Symbol[], int);
Node (*gen)     (Node);
void (*global)(Symbol);
void (*import)(Symbol);
void (*local)(Symbol);
void (*progbeg)(int argc, char *argv[]);
void (*progend)(void);
void (*segment)(int);
void (*space)(int);
void (*stabblock)(int, int, Symbol*);
void (*stabend)  (Coordinate *, Symbol, Coordinate **,                   	Symbol *, Symbol *);
void (*stabfend) (Symbol, int);
void (*stabinit) (char *, int, char *[]);
void (*stabline) (Coordinate *);
void (*stabsym)  (Symbol);
void (*stabtype) (Symbol);
	Xinterface x;
} Interface;

static void address(Symbol, Symbol, int);
static void blockbeg0(Env *);
static void blockend0(Env *);
static void defaddress(Symbol);
static void defconst(int, Value);
static void defstring(int, char *);
static void defsymbol(Symbol);
static void emit0(Node);
static void export(Symbol);
static void function(Symbol, Symbol [], Symbol [], int);
static Node gen0(Node);
static void global(Symbol);
static void import(Symbol);
static void local(Symbol);
static void progbeg(int, char *[]);
static void progend(void);
static void segment(int);
static void space(int);

static Node gen0(p) Node p; { return p; }
static void address(q, p, n) Symbol q, p; int n; {}
static void blockbeg0(e) Env *e; {}
static void blockend0(e) Env *e; {}
static void defaddress(p) Symbol p; {}
static void defconst(ty, v) int ty; Value v; {}
static void defstring(len, s) int len; char *s; {}
static void defsymbol(p) Symbol p; {}
static void emit0(p) Node p; {}
static void export(p) Symbol p; {}
static void function(f, caller, callee, ncalls) Symbol f, caller[], callee[]; int ncalls; {}
static void global(p) Symbol p; {}
static void import(p) Symbol p; {}
static void local(p) Symbol p; {}
static void progbeg(argc, argv) int argc; char *argv[]; {}
static void progend(void) {}
static void segment(s) int s; {}
static void space(n) int n; {}

Interface nullIR = {
	{1, 1, 0},	/* char */
	2, {2}, 0,	/* short */
	{{4}, 4, {0}},	/* int */
	4, 4, 1,	/* float */
	8, 8, 1,	/* double */
	4, 4, 0,	/* T* */
	0, 4, 0,	/* struct */
	1,		/* little_endian */
	0,		/* mulops_calls */
	0,		/* wants_callb */
	0,		/* wants_argb */
	1,		/* left_to_right */
	0,		/* wants_dag */
	address,
	blockbeg0,
	blockend0,
	defaddress,
	defconst,
	defstring,
	defsymbol,
	emit0,
	export,
	function,
	gen0,
	global,
	import,
	local,
	progbeg,
	progend,
	segment,
	space
};
