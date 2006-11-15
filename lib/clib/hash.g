#ifndef __HASH_H_
#ifndef TABLE
#define __HASH_H_
#endif

#include <stdlibx.h>

#ifndef HASHPTR
#define HASHPTR(p) (((unsigned long)p) >> 4)
#endif

extern long hashtable_bumpfunc(long);

struct hashtable;
struct hashiter;
typedef void hashelt;

typedef unsigned long hash_func(hashelt *self);
typedef hashelt *init_func(hashelt *self);
typedef hashelt *copy_func(hashelt *self, hashelt *e);
typedef hashelt *destroy_func(hashelt *self);
typedef boolean compare_func(hashelt *self, hashelt *e);

struct hashinfo
{
	hash_func *hash;
	init_func *init;
	copy_func *copy;
	destroy_func *destroy;
	compare_func *compare;
	size_t size;
};

extern hashelt *lookup_hashtable(struct hashtable *self, hashelt *key);
extern struct hashtable *init_hashtable(struct hashtable *self,
	struct hashinfo *info);
extern struct hashtable *destroy_hashtable(struct hashtable *self);
extern struct hashtable *new_hashtable(struct hashinfo *info);
extern struct hashtable *dup_hashtable(struct hashtable *self);
extern void delete_hashtable(struct hashtable *self);
extern void clear_hashtable(struct hashtable *self);
extern long size_hashtable(struct hashtable *self);
extern hashelt *get_hashtable(struct hashtable *self, hashelt *key);
extern boolean find_hashtable(struct hashtable *self, hashelt *key);
extern boolean insert_hashtable(struct hashtable *self, hashelt *key);
extern boolean remove_hashtable(struct hashtable *self, hashelt *key);
extern hashelt *cur_hashtable(struct hashtable *self);
extern struct hashtable *copy_hashtable(struct hashtable *self,
	struct hashtable *t);
extern boolean compare_hashtable(struct hashtable *self, struct hashtable *t);
extern void join_hashtable(struct hashtable *self, struct hashtable *t);
extern void separate_hashtable(struct hashtable *self, struct hashtable *t);
extern void intersect_hashtable(struct hashtable *self, struct hashtable *t);

extern struct hashiter *init_hashiter(struct hashiter *self);
extern struct hashiter *destroy_hashiter(struct hashiter *self);
extern struct hashiter *new_hashiter(void);
extern struct hashiter *dup_hashiter(struct hashiter *self);
extern void delete_hashiter(struct hashiter *self);
extern void attach_hashiter(struct hashiter *self, struct hashtable *t);
extern void detach_hashiter(struct hashiter *self);
extern struct hashiter *copy_hashiter(struct hashiter *self,
	struct hashiter *i);
extern void table_hashiter(struct hashiter *self, struct hashtable *t);
extern boolean compare_hashiter(struct hashiter *self, struct hashiter *i);
extern hashelt *cur_hashiter(struct hashiter *self);
extern boolean finished_hashiter(struct hashiter *self);
extern hashelt *advance_hashiter(struct hashiter *self);
extern boolean remove_hashiter(struct hashiter *self);

/*$begin declare_hashtable(TABLE, ITER, ELEM, KEY)*/

typedef struct hashtable TABLE;
typedef struct hashiter ITER;

extern struct hashinfo name2(TABLE, _info);

extern TABLE *name2(init_, TABLE)(TABLE *self);
extern TABLE *name2(destroy_, TABLE)(TABLE *self);
extern TABLE *name2(new_, TABLE)(void);
extern TABLE *name2(dup_, TABLE)(TABLE *self);
extern void name2(delete_, TABLE)(TABLE *self);
extern void name2(clear_, TABLE)(TABLE *self);
extern long name2(size_, TABLE)(TABLE *self);
extern ELEM *name2(get_, TABLE)(TABLE *self, KEY key);
extern boolean name2(add_, TABLE)(TABLE *self, KEY key);
extern boolean name2(drop_, TABLE)(TABLE *self, KEY key);
extern boolean name2(find_, TABLE)(TABLE *self, ELEM *key);
extern boolean name2(insert_, TABLE)(TABLE *self, ELEM *key);
extern boolean name2(remove_, TABLE)(TABLE *self, ELEM *key);
extern ELEM *name2(cur_, TABLE)(TABLE *self);
extern TABLE *name2(copy_, TABLE)(TABLE *self, TABLE *t);
extern boolean name2(compare_, TABLE)(TABLE *self, TABLE *t);
extern void name2(join_, TABLE)(TABLE *self, TABLE *t);
extern void name2(separate_, TABLE)(TABLE *self, TABLE *t);
extern void name2(intersect_, TABLE)(TABLE *self, TABLE *t);

extern ITER *name2(init_, ITER)(ITER *self);
extern ITER *name2(destroy_, ITER)(ITER *self);
extern ITER *name2(new_, ITER)(void);
extern ITER *name2(dup_, ITER)(ITER *self);
extern void name2(delete_, ITER)(ITER *self);
extern void name2(attach_, ITER)(ITER *self, TABLE *t);
extern void name2(detach_, ITER)(ITER *self);
extern ITER *name2(copy_, ITER)(ITER *self, ITER *i);
extern void name2(table_, ITER)(ITER *self, TABLE *t);
extern boolean name2(compare_, ITER)(ITER *self, ITER *i);
extern ELEM *name2(cur_, ITER)(ITER *self);
extern boolean name2(finished_, ITER)(ITER *self);
extern ELEM *name2(advance_, ITER)(ITER *self);
extern boolean name2(remove_, ITER)(ITER *self);

/*$end*/

/*$begincomment*/
#ifdef IMPLEMENT
/*$endcomment*/

/*$begin implement_hashtable(TABLE, ITER, ELEM, KEY)*/

extern unsigned long name2(hash_, ELEM)(ELEM *self);
extern ELEM *name2(init_, ELEM)(ELEM *self);
extern ELEM *name2(initkey_, ELEM)(ELEM *self, KEY e);
extern ELEM *name2(copy_, ELEM)(ELEM *self, ELEM *e);
extern ELEM *name2(destroy_, ELEM)(ELEM *self);
extern boolean name2(compare_, ELEM)(ELEM *self, ELEM *e);

struct hashinfo name2(TABLE, _info) =
{
	(hash_func *)name2(hash_, ELEM),
	(init_func *)name2(init_, ELEM),
	(copy_func *)name2(copy_, ELEM),
	(destroy_func *)name2(destroy_, ELEM),
	(compare_func *)name2(compare_, ELEM),
	sizeof (ELEM)
};

long
name2(size_, TABLE)(TABLE *self)
{
	return size_hashtable(self);
}

ELEM *
name2(get_, TABLE)(TABLE *self, KEY key)
{
	ELEM ekey, *ret;

	name2(initkey_, ELEM)(&ekey, key);
	ret = (ELEM *)get_hashtable(self, (hashelt *)&ekey);
	name2(destroy_, ELEM)(&ekey);
	return ret;
}

boolean
name2(find_, TABLE)(TABLE *self, ELEM *key)
{
	return find_hashtable(self, (hashelt *)key);
}

ELEM *
name2(cur_, TABLE)(TABLE *self)
{
	return cur_hashtable(self);
}

TABLE *
name2(init_, TABLE)(TABLE *self)
{
	return init_hashtable(self, &name2(TABLE, _info));
}

TABLE *
name2(new_, TABLE)(void)
{
	return new_hashtable(&name2(TABLE, _info));
}

TABLE *
name2(dup_, TABLE)(TABLE *self)
{
	return dup_hashtable(self);
}

TABLE *
name2(destroy_, TABLE)(TABLE *self)
{
	return destroy_hashtable(self);
}

void
name2(delete_, TABLE)(TABLE *self)
{
	delete_hashtable(self);
}

boolean
name2(add_, TABLE)(TABLE *self, KEY key)
{
	ELEM ekey;
	boolean ret;

	name2(initkey_, ELEM)(&ekey, key);
	ret = insert_hashtable(self, (hashelt *)&ekey);
	name2(destroy_, ELEM)(&ekey);
	return ret;
}

boolean
name2(insert_, TABLE)(TABLE *self, ELEM *key)
{
	return insert_hashtable(self, (hashelt *)key);
}

boolean
name2(drop_, TABLE)(TABLE *self, KEY key)
{
	ELEM ekey;
	boolean ret;

	name2(initkey_, ELEM)(&ekey, key);
	ret = remove_hashtable(self, (hashelt *)&ekey);
	name2(destroy_, ELEM)(&ekey);
	return ret;
}

boolean
name2(remove_, TABLE)(TABLE *self, ELEM *key)
{
	return remove_hashtable(self, (hashelt *)key);
}

void
name2(clear_, TABLE)(TABLE *self)
{
	clear_hashtable(self);
}

TABLE *
name2(copy_, TABLE)(TABLE *self, TABLE *t)
{
	return copy_hashtable(self, t);
}

boolean
name2(compare_, TABLE)(TABLE *self, TABLE *t)
{
	return compare_hashtable(self, t);
}

void
name2(join_, TABLE)(TABLE *self, TABLE *t)
{
	join_hashtable(self, t);
}

void
name2(separate_, TABLE)(TABLE *self, TABLE *t)
{
	separate_hashtable(self, t);
}

void
name2(intersect_, TABLE)(TABLE *self, TABLE *t)
{
	intersect_hashtable(self, t);
}

ELEM *
name2(cur_, ITER)(ITER *self)
{
	return (ELEM *)cur_hashiter(self);
}

boolean
name2(finished_, ITER)(ITER *self)
{
	return finished_hashiter(self);
}

void
name2(attach_, ITER)(ITER *self, TABLE *t)
{
	attach_hashiter(self, t);
}

void
name2(detach_, ITER)(ITER *self)
{
	detach_hashiter(self);
}

ITER *
name2(init_, ITER)(ITER *self)
{
	return init_hashiter(self);
}

ITER *
name2(new_, ITER)(void)
{
	return new_hashiter();
}

ITER *
name2(destroy_, ITER)(ITER *self)
{
	return destroy_hashiter(self);
}

void
name2(delete_, ITER)(ITER *self)
{
	delete_hashiter(self);
}

ITER *
name2(copy_, ITER)(ITER *self, ITER *i)
{
	return copy_hashiter(self, i);
}

void
name2(table_, ITER)(ITER *self, TABLE *t)
{
	table_hashiter(self, t);
}

boolean
name2(compare_, ITER)(ITER *self, ITER *i)
{
	return compare_hashiter(self, i);
}

ELEM *
name2(advance_, ITER)(ITER *self)
{
	return (ELEM *)advance_hashiter(self);
}

boolean
name2(remove_, ITER)(ITER *self)
{
	return remove_hashiter(self);
}

/*$end*/

/*$begincomment*/
#endif
#undef TABLE
#undef ITER
#undef ELEM
/*$endcomment*/

#endif /*__HASH_H_*/
