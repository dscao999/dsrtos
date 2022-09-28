#ifndef ARMV7M_UTILS_DSCAO__
#define ARMV7M_UTILS_DSCAO__
#include <stdint.h>

static inline int32_t try_lock(volatile int *lock, uint32_t lv)
{
	uint32_t retv;

	asm volatile (       "ldrex	%0,[%1]\n"	\
			     "\tcmp	%0, #0\n"	\
			     "\tite	eq\n"		\
			     "\tstrexeq	%0, %2, [%1]\n"	\
			     "\tclrexne\n"		\
			      :"=r"(retv): "r"(lock), "r"(lv));
	return retv;
}

static inline void spin_lock(volatile int *lock, uint32_t lv)
{
	while(try_lock(lock, lv))
		;
}

void switch_stack(void *mstack, void *pstack);

#endif  /* ARMV7M_UTILS_DSCAO__ */
