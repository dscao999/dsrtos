#include "armv7m_utils.h"

void __attribute__((naked)) spin_lock(volatile int *lock, uint32_t lv)
{
	asm volatile   (     "mov	r2, r0\n"	\
			     "try:\n"		\
			     "\tldrex	r0, [r2]\n"	\
			     "\tcmp	r0, #0\n"	\
			     "\titte	eq\n"		\
			     "\tstrexeq	r0, r1, [r2]\n" \
			     "\tcmpeq	r0,#0\n"	\
			     "\tbne	try\n"	\
			     "\tbx	lr\n"
			);
}
