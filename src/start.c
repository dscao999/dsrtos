#include <stdint.h>
#include <stdbool.h>
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
		dead_flash(100);

	klog("Before stack switch\n");
	task->stat = RUN;
	switch_stack(mstack, pstack);
	/* old stack switched, no local variable usable */
	asm volatile (  "mov r0, #0\n"	\
			"\tsvc #0\n");
	main();

	asm volatile (  "mov r0, #0\n"	\
			"\tsvc #0\n");
	idle_task();
}


static char conin[256];
void idle_task(void)
{
	int msgpos;
	uint32_t ctl;

	klog("Idle Task Entered\n");
	msgpos = 0;
	do {
		if (errno) {
		}
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
			default:
				klog("current ticks: %u\n", osticks->tick_low);
				break;
		}
		msgpos = 0;
	} while (errno == 0);
	dead_flash(100);
}
