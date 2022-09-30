#ifndef TASK_DSCAO__ 
#define TASK_DSCAO__ 
#include "kernel_internal.h"

enum TASK_STATE {NONE = 0, BLOCKED = 1, READY = 2, RUN = 3};

#define TASK_PRIO_MAXNUM	255

#define NO_TASK_READY	1 /* no task in ready/run state */

struct Task_Info {
	void *psp;
	uint8_t stat;
	uint8_t cpri;
	uint8_t bpri;
};

static inline struct Task_Info * current_task(void)
{
	uint32_t curpsp, intrnum;

	asm volatile ("mrs %0, ipsr":"=r"(intrnum));
	intrnum &= 0x01ff;
	if (intrnum)
		asm volatile ("mrs %0, psp":"=r"(curpsp));
	else
		asm volatile ("mov %0, sp":"=r"(curpsp));
	return (struct Task_Info *)(curpsp & PSTACK_MASK);
}

void task_slot_init(void);

#endif  /* TASK_DSCAO__ */
