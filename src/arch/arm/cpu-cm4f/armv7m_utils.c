#include "armv7m_utils.h"

void main(void);

void switch_stack(void *mstack, void *pstack)
{
	uint32_t isrnum, control, primask;

	asm volatile ("mrs %0, ipsr\n" \
			"\tmrs %1, control\n" :"=r"(isrnum), "=r"(control));
	if ((isrnum & 0x1ff) != 0 || (control & 0x2) != 0)
		return;

	control |= 0x02;
	primask = 1;
	asm volatile (  "msr	primask, %0\n"	\
			"\tmsr	msp, %1\n"	\
			"\tmsr	psp, %2\n"	\
			"\tmsr	control, %3\n"	\
			"\tisb\n"		\
			"\tmov	%0, #0\n"	\
			"\tmsr	primask, %0\n"	\
			::"r"(primask), "r"(mstack), "r"(pstack), "r"(control));
	main();
}
