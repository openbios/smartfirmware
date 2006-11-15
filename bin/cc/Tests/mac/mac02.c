extern void *malloc(unsigned sz);
extern void free(void *);

extern int foo();	// the function to be pointed to by pf

int (*pf)();		// pointer to a function returning an int
__pascal__ int (**ppf)();	// pointer to a pointer to a function returning an int

void
bar()
{
  ppf = (int (**)()) malloc(sizeof (int (*)()));  // allocate pointer 
  pf = foo;
  *ppf = foo;		// point to the function

  (*pf)(); 		// do whatever nonsense
  (**ppf)(); 		// do whatever nonsense
  (pf)(); 		// do whatever nonsense
  (*ppf)(); 		// do whatever nonsense

  free(ppf);		// free the area containing the pointer
}
