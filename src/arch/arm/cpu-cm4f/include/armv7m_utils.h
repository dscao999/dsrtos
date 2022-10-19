#ifndef ARMV7M_UTILS_DSCAO__
#define ARMV7M_UTILS_DSCAO__
#include <stdint.h>
#include "hw_nvic.h"
#include "misc_utils.h"

#define likely(x)	__builtin_expect((x), 1)

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
	if (retv == 0)
		asm volatile ("dmb");
	return retv;
}

static inline int comp_swap_uint32(uint32_t *dst, uint32_t *oval, uint32_t nval)
{
	uint32_t flag, retv;

	flag = 1;
	asm volatile (	"ldrex		%0, [%2]\n"	\
			"\tcmp		%0, %4\n"	\
			"\tite		eq\n"		\
			"\tstrexeq	%1, %3, [%2]\n"	\
			"\tclrexne\n"			\
			:"=r"(retv), "=r"(flag)		\
			:"r"(dst), "r"(nval), "r"(*oval));
	*oval = retv;
	return flag;
}

static inline int in_interrupt(void)
{
	uint32_t intrnum;

	asm volatile ("mrs %0, ipsr":"=r"(intrnum));
	return (intrnum & 0x01ff);
}

static inline void svc_switch(void)
{
	if (likely(in_interrupt() == 0))
		asm volatile ("svc #0");
}

static inline void wait_interrupt(void)
{
	asm volatile ("wfi");
}

static inline void start_systick(void)
{
	volatile uint32_t *st_ctrl;
	uint32_t stval;

        st_ctrl = (volatile uint32_t *)NVIC_ST_CTRL;
        stval = *st_ctrl;
        stval |= NVIC_ST_CTRL_CLK_SRC|NVIC_ST_CTRL_INTEN|NVIC_ST_CTRL_ENABLE;
        *st_ctrl = stval;
}

static inline void arm_pendsvc()
{
	volatile uint32_t *int_ctrl;
	uint32_t val;

	int_ctrl = (volatile uint32_t *)NVIC_INT_CTRL;
	val = *int_ctrl;
	val |= (1 << 28);
	*int_ctrl = val;
}

static inline void disable_interrupt(void)
{
	asm volatile ("cpsid  i");
}

static inline void enable_interrupt(void)
{
	asm volatile ("cpsie  i");
}

void switch_stack(void *mstack, void *pstack);

#endif  /* ARMV7M_UTILS_DSCAO__ */
