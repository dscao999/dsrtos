#include "armv7m_utils.h"

#define unlikely(x)	__builtin_expect((x), 0)
#define likely(x)	__builtin_expect((x), 1)

void main(void);

void switch_stack(void *mstack, void *pstack)
{
	uint32_t isrnum, control;

	asm volatile ("mrs %0, ipsr\n" \
			"\tmrs %1, control\n" :"=r"(isrnum), "=r"(control));
	if (unlikely((isrnum & 0x1ff) != 0 || (control & 0x2) != 0))
		return;

	control |= 0x02;
	asm volatile (  "cpsid	i\n"		\
			"\tmsr	msp, %0\n"	\
			"\tmsr	psp, %1\n"	\
			"\tmsr	control, %2\n"	\
			"\tisb\n"		\
			"\tcpsie i\n"	\
			::"r"(mstack), "r"(pstack), "r"(control));
	main();
}
