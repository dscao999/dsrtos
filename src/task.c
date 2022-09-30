#include "armv7m_utils.h"
#include "kernel_internal.h"
#include "kernel.h"
#include "task.h"

//static const uint32_t TASK_MODULE = 0x50000;

void task_slot_init(void)
{
	struct Task_Info *task;
	int i;

	for (i = 0; i < MAX_NUM_TASKS; i++) {
		task = (struct Task_Info *)(pstacks+i);
		task->stat = NONE;
		task->bpri = TASK_PRIO_MAXNUM;
		task->cpri = TASK_PRIO_MAXNUM;
	}
}

struct Task_Info * select_next_task(struct Task_Info *current)
{
	struct Task_Info *task;
	uint8_t pri;
       	int curseq, i, mask, candidate, sel;

	mask = MAX_NUM_TASKS - 1;
	candidate = 0;
	curseq = -1;
	pri = TASK_PRIO_MAXNUM;
	for (i = 0; i < MAX_NUM_TASKS; i++) {
		task = (struct Task_Info *)(pstacks+i);
		if (task->stat != READY && task->stat != RUN)
			continue;
		if (task == current)
			curseq = i;
		if (task->cpri < pri) {
			pri = task->cpri;
			candidate = (1 << i);
		} else if (task->cpri == pri)
			candidate |= (1 << i);
	}
	sel = (curseq + 1) & mask;
	while (sel != curseq && (candidate & (1 << sel)) == 0)
		sel = (sel + 1) & mask;
	task = (struct Task_Info *)(pstacks + sel);
	return task;
}

static inline void * getpsp(void)
{
	void *psp;

	asm volatile ("mrs %0, psp":"=r"(psp));
	return psp;
}

static inline void setpsp(void *psp)
{
	asm volatile ("msr psp, %0"::"r"(psp));
}

void __attribute__((naked)) SVC_Handler(void)
{
	struct Task_Info *nxt, *cur;
	struct Reg_Context *frame, *cur_frame, *nxt_frame;

	asm volatile   ("push {r4-r11, lr}\n"	\
			"\tmov %0, sp\n":"=r"(frame));

	cur = current_task();
	nxt = select_next_task(cur);
	if (cur == nxt)
		goto exit_10;

	cur->stat = READY;
	cur_frame = getpsp();
	cur_frame = cur_frame - 1;
	*cur_frame = *frame;
	cur->psp = cur_frame;
	nxt_frame = nxt->psp;
	nxt->stat = RUN;
	*frame = *nxt_frame;
	nxt_frame = nxt_frame + 1;
	setpsp(nxt_frame);

exit_10:
	asm volatile   ("mov sp, %0\n"	\
			"\tpop {r4-r11, pc}\n"::"r"(frame));
	return;
}
