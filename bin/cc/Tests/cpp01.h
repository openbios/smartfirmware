#ifdef THINK_C
#define INTERRUPT_CHECK() if (--ipoll_counter < 0) full_interrupt_poll(&ipoll_counter)
#else
#define INTERRUPT_CHECK()
#endif

INTERRUPT_CHECK();
extern char *stack_limit_ptr;
__LINE__ "should be 9"
