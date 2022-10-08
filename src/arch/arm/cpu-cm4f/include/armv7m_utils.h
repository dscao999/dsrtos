#ifndef ARMV7M_UTILS_DSCAO__
#define ARMV7M_UTILS_DSCAO__
#include <stdint.h>
#include "misc_utils.h"

struct Intr_Context {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t retadr;
	uint32_t xpsr;
};

struct Reg_Context {
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
};

static inline void intr_context_setup(void *frame, uint32_t retadr, void *param)
{
	struct Intr_Context *ctxt = frame;
	static const uint32_t xpsr = 0x61000000;

	memset(frame, 0, sizeof(struct Intr_Context));
	ctxt->retadr = retadr & (~1);
	ctxt->xpsr = xpsr;
	ctxt->r0 = (uint32_t)param;
}

static inline void reg_context_setup(void *frame)
{
	memset(frame, 0, sizeof(struct Reg_Context));
}

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
