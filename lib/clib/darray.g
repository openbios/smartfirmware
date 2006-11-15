#ifndef __DARRAY_H_
#ifndef ARRAY
#define __DARRAY_H_
#endif

#include <memory.h>
#include <stdlibx.h>
#include <expand.h>

/*$begin declare_array(ARRAY, TYPE, BUMP)*/

typedef struct ARRAY
{
	TYPE *array;
	long arrlen;
	long size;
} ARRAY;

extern ARRAY *name2(init_, ARRAY)(ARRAY *self);
extern ARRAY *name2(destroy_, ARRAY)(ARRAY *self);
extern ARRAY *name2(new_, ARRAY)(void);
extern ARRAY *name2(dup_, ARRAY)(ARRAY *self);
extern void name2(delete_, ARRAY)(ARRAY *self);
extern void name2(expand_, ARRAY)(ARRAY *self, long leng);
extern ARRAY *name2(copy_, ARRAY)(ARRAY *self, ARRAY *from);
extern boolean name2(compare_, ARRAY)(ARRAY *arr1, ARRAY *arr2);
extern long name2(size_, ARRAY)(ARRAY *self);
extern void name2(reset_, ARRAY)(ARRAY *self, long newlen);
extern void name2(resize_, ARRAY)(ARRAY *self, long newlen);
extern TYPE *name2(elem_, ARRAY)(ARRAY *self, long e);
extern TYPE *name2(elt_, ARRAY)(ARRAY *self, long e);
extern TYPE *name2(end_, ARRAY)(ARRAY *self);
extern TYPE *name2(getarr_, ARRAY)(ARRAY *self);

/*$end*/

/*$begincomment*/
#ifdef IMPLEMENT
/*$endcomment*/

/*$begin implement_array(ARRAY, TYPE, BUMP)*/

long
name2(size_, ARRAY)(ARRAY *self)
{
	return self->size;
}

TYPE *
name2(elem_, ARRAY)(ARRAY *self, long e)
{
	if (e < 0)
		return NULL;

	if (e >= self->size)
		name2(expand_, ARRAY)(self, e + 1);

	return &self->array[e];
}

TYPE *
name2(elt_, ARRAY)(ARRAY *self, long e)
{
	return &self->array[e];
}

TYPE *
name2(end_, ARRAY)(ARRAY *self)
{
	long sz = self->size;
	name2(expand_, ARRAY)(self, sz + 1);
	return &self->array[sz];
}

TYPE *name2(getarr_, ARRAY)(ARRAY *self)
{
	return self->array;
}

void
name2(expand_, ARRAY)(ARRAY *self, long size)
{
	if (size >= self->arrlen)
		bufexpand((void **)&self->array, &self->arrlen, size + 1,
				sizeof (TYPE), BUMP);

	self->size = size;
}

ARRAY *
name2(init_, ARRAY)(ARRAY *self)
{
	self->array = NULL;
	self->arrlen = 0;
	self->size = 0;
	return self;
}

ARRAY *
name2(new_, ARRAY)(void)
{
	return name2(init_, ARRAY)((ARRAY *)malloc(sizeof (ARRAY)));
}

ARRAY *
name2(dup_, ARRAY)(ARRAY *self)
{
	ARRAY *dup = name2(new_, ARRAY)();
	return name2(copy_, ARRAY)(dup, self);
}

ARRAY *
name2(destroy_, ARRAY)(ARRAY *self)
{
	if (self == NULL)
		return NULL;

	if (self->array)
		free(self->array);

	return self;
}

void
name2(delete_, ARRAY)(ARRAY *self)
{
	if (self == NULL)
		return;

	free(name2(destroy_, ARRAY)(self));
}

ARRAY *
name2(copy_, ARRAY)(ARRAY *self, ARRAY *a)
{
	name2(reset_, ARRAY)(self, a->size);
	memcpy(self->array, a->array, self->size * sizeof (TYPE));
	return self;
}

boolean
name2(compare_, ARRAY)(ARRAY *self, ARRAY *a)
{
	long i;

	if (self->size != a->size)
		return FALSE;

	for (i = 0; i < self->size; i++)
		if (memcmp(&self->array[i], &a->array[i], sizeof (TYPE)) != 0)
			return FALSE;

	return TRUE;
}

void
name2(reset_, ARRAY)(ARRAY *self, long newlen)
{
	if (newlen > 0)
		name2(expand_, ARRAY)(self, newlen);
	else
	{
		if (self->array != NULL)
			free(self->array);

		self->array = NULL;
		self->arrlen = 0;
	}

	self->size = newlen;
}

void
name2(resize_, ARRAY)(ARRAY *self, long newlen)
{
	name2(expand_, ARRAY)(self, newlen);
	self->size = newlen;
}
/*$end*/

/*$begincomment*/
#endif
#undef ARRAY
#undef TYPE
#undef BUMP
/*$endcomment*/

declare_array(charbuf, char, 1024);

#endif /*__DARRAY_H_*/
