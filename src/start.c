#include <stdint.h>
#include <stdbool.h>
#include <hw_nvic.h>
#include "misc_utils.h"
#include "armv7m_utils.h"
#include "kernel_internal.h"
#include "kernel.h"
#include "board_def.h"
#include "task.h"

void main(void);

volatile int errno = 0;

typedef void (*init_func_t)(void);

extern init_func_t __init_start[];
extern init_func_t __init_end[];

void idle_task(void);

void __attribute__((naked)) kernel_start(void)
{
	init_func_t init_f, *func;
	char *mstack, *pstack;
	struct Task_Info *task;

	func =  __init_start;
	while (func < __init_end) {
		init_f = *func++;
		init_f();
	}
	errno = 0;
	mstack = (char *)(main_stack + MAX_MSTACK_SIZE);
	pstack = (char *)(pstacks + MAX_NUM_TASKS);
	task_slot_init();

	task = (struct Task_Info *)(((uint32_t)(pstack - 1)) & PSTACK_MASK);
	if ((void *)task != (void *)(pstacks + MAX_NUM_TASKS - 1))
		death_flash(100);

	klog("Before stack switch\n");
	task->stat = RUN;
	switch_stack(mstack, pstack);
	/* old stack switched, no local variable usable */
	asm volatile (  "mov r0, #0\n"	\
			"\tsvc #0\n");
	main();

//	death_flash(80);
	idle_task();
}

static inline int exception_priority(int expnum)
{
	volatile uint32_t *sysint_pri;
	int idx, bitpos;
	uint32_t prival;

	if (expnum < 1 || expnum > 15)
		return -4;
	else if (expnum >=1 && expnum <= 3)
		return -4 + expnum;
	idx = expnum - 4;
	bitpos = idx % 4;
	sysint_pri = (volatile uint32_t *)(NVIC_SYS_PRI1 + (idx / 4) * 4);
	prival = *sysint_pri;
	return ((prival >> (bitpos*8)) & 0x0ff) >> 5;
}

static char conin[256];
void idle_task(void)
{
	int msgpos, expnum;
	uint32_t ctl, val;

	klog("Idle Task Entered\n");
	msgpos = 0;
	do {
		asm volatile (  "mov r0, #0\n"	\
				"\tsvc #0\n");
		if (msgpos >= sizeof(conin) - 1)
			msgpos = 0;
		msgpos += console_getstr(conin+msgpos, sizeof(conin) - msgpos);
		if (msgpos == 0 || memchr(conin, msgpos, '\r') == -1)
			continue;
		switch(conin[0]) {
			case '0':
				asm volatile ("mrs	%0, control\n":"=r"(ctl));
				klog("Control Reg: %x\n", ctl);
				break;
			case '1':
				asm volatile ("ldr	r2, =0xE000ED14\n"	\
						"ldr	%0, [r2]\n"	\
						:"=r"(ctl)::"r2");
				klog("Control Reg: %x\n", ctl);
				break;
			case '2':
				asm volatile ("mov	%0,sp\n":"=r"(ctl));
				klog("Current SP: %x\n", ctl);
				break;
			case '3':
				break;
			case '4': /* priority of exception 4 */
				expnum = conin[0] - '0';
			default:
				expnum = -1;
				if (conin[0] >= '0' && conin[0] <= '9')
					expnum = conin[0] - '0';
				else if (conin[0] >= 'A' && conin[0] <= 'F')
					expnum = conin[0] - 'A' + 10;
				if (expnum < 4 || expnum > 15)
					break;
				val = exception_priority(expnum);
				klog("Priority of exception %d: %u\n", expnum, val);
				break;
		}
		msgpos = 0;
	} while (errno == 0);
	death_flash(100);
}
