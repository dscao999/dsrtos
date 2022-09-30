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

static void task_idle(void)
{
	main();
}

void kernel_start(void)
{
	init_func_t init_f, *func;
	char *mstack, *pstack;
	void *frame;
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

	frame = pstack - sizeof(struct Intr_Context);
	intr_context_setup(frame, task_idle);
	frame = ((char *)frame) - sizeof(struct Reg_Context);
	reg_context_setup(frame);
	task = (struct Task_Info *)(((uint32_t)frame) & PSTACK_MASK);
	if ((void *)task != (void *)(pstacks + MAX_NUM_TASKS - 1)) {
		errno = 123;
		error_flash();
	}

	task->stat = READY;
	task->psp = frame;
	task->bpri = TASK_PRIO_MAXNUM;
	task->cpri = TASK_PRIO_MAXNUM;

	switch_stack(mstack, pstack);
	/* old stack switched, no local variable usable */
	main();
}
