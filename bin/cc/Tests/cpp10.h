#if defined(powerc) || defined(__powerc)
#define USES68KINLINES 0
#else
#define USES68KINLINES 1
#endif

#define USESCODEFRAGMENTS !USES68KINLINES

#if USES68KINLINES && !USESCODEFRAGMENTS
int foo;
#endif

