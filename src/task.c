#include "hw_nvic.h"
#include "armv7m_utils.h"
#include "board_def.h"
#include "kernel_internal.h"
#include "kernel.h"
#include "task.h"

#define ENOMEM		1
#define ENORTASK	2
#define ETRESVD		3
#define ENOSTASK	4

static const uint32_t MODULE = 0x10000;

#define NULL	((void *)0)
#define unlikely(x)	__builtin_expect((x), 0)

static struct Task_Timer ktimers[MAX_NUM_TIMERS];
static struct Sys_Tick sys_tick = {.tick_low = 0, .tick_high = 0};
volatile const struct Sys_Tick * const osticks = &sys_tick;

void task_slot_init(void)
{
	struct Task_Info *task;
	int i;

	for (i = 0; i < MAX_NUM_TASKS; i++) {
		task = (struct Task_Info *)(pstacks+i);
		task->stat = NONE;
		task->bpri = BOT;
		task->cpri = BOT;
		task->acc_ticks = 0;
		task->time_slice = 0;
		task->timer = NULL;
	}
	for (i = 0; i < MAX_NUM_TIMERS; i++) {
		ktimers[i].eticks = 0;
		ktimers[i].task = (void *)0;
	}
}

int create_task(struct Task_Info **handle, enum TASK_PRIORITY prival,
		void *(*task_entry)(void *), void *param)
{
	int i;
	struct Task_Info *task;
	void *frame;

	*handle = (struct Task_Info *)0;
	for (i = 0; i < MAX_NUM_TASKS; i++) {
		task = (struct Task_Info *)(pstacks + i);
		if (task->stat == NONE)
			break;
	}
	if (i == MAX_NUM_TASKS) {
		klog("Failed to create new task, Too Many Tasks: %x.\n",
				MODULE+ENOMEM);
		return -(MODULE + ENOMEM);
	}
	frame = ((char *)(pstacks + i + 1)) - sizeof(struct Intr_Context);
	intr_context_setup(frame, (uint32_t)task_entry, param);
	((struct Intr_Context *)frame)->lr = (uint32_t)task_reaper;
	frame = ((char *)frame) - sizeof(struct Reg_Context);
	reg_context_setup(frame);
	task->psp = frame;
	task->bpri = prival;
	task->cpri = prival;
	task->acc_ticks = 0;
	task->time_slice = 0;
	asm volatile ("dmb");
	task->stat = READY;
	*handle = task;
	return 0;
}

struct Task_Info * select_next_task(struct Task_Info *current)
{
	struct Task_Info *task;
	enum TASK_PRIORITY pri;
       	int curseq, i, mask, candidate, sel;

	mask = MAX_NUM_TASKS - 1;
	candidate = 0;
	curseq = -1;
	pri = BOT;
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
	if (candidate == 0) {
		klog("No Runnable Task Exists: %x\n", MODULE + ENORTASK);
		return NULL;
	}

	sel = (curseq + 1) & mask;
	while ((candidate & (1 << sel)) == 0)
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

static void task_switch(struct Reg_Context *frame)
{
	struct Task_Info *nxt, *cur;
	struct Reg_Context *cur_frame, *nxt_frame;

	cur = current_task();
	nxt = select_next_task(cur);
	cur->acc_ticks += cur->time_slice;
	cur->time_slice = 0;
	if (cur == nxt)
		return;
	if (unlikely(nxt == NULL))
		death_flash();

	if (cur->stat == RUN)
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
}

void __attribute__((naked)) SVC_Handler(void)
{
	struct Reg_Context *frame;
	struct Intr_Context *intr_frame;
	uint16_t inst;

	asm volatile   ("push {r4-r11, lr}\n"	\
			"\tmov %0, sp\n":"=r"(frame));
	intr_frame = getpsp();
	inst = *((uint16_t *)(intr_frame->retadr - 2));
	if ((inst & 0x0ff) == 0)
		task_switch(frame);
	asm volatile   ("mov sp, %0\n"	\
			"\tpop {r4-r11, pc}\n"::"r"(frame));
}

void __attribute__((naked)) PendSVC_Handler(void)
{
	struct Reg_Context *frame;

	asm volatile   ("push {r4-r11, lr}\n"	\
			"\tmov %0, sp\n":"=r"(frame));
	task_switch(frame);
	asm volatile   ("mov sp, %0\n"	\
			"\tpop {r4-r11, pc}\n"::"r"(frame));
}

void SysTick_Handler(void)
{
	volatile uint32_t *scbreg;
	uint32_t scbv;
	int i;
	struct Task_Info *task, *nxt_task;

	scbreg = (volatile uint32_t *)NVIC_ST_CTRL;
	scbv = *scbreg;
	if (__builtin_expect((scbv & (1 << 16)) == 0, 0))
		while (1)
			;
	sys_tick.tick_low += 1;
	if (__builtin_expect(sys_tick.tick_low == 0, 0))
		sys_tick.tick_high += 1;
	for (i = 0; i < MAX_NUM_TIMERS; i++) {
		if (ktimers[i].task == NULL)
			continue;
		if (--ktimers[i].eticks == 0) {
			ktimers[i].task->stat = READY;
			ktimers[i].task->timer = NULL;
			ktimers[i].task = NULL;
		}
	}
	task = current_task();
	task->time_slice += 1;
	if (task->time_slice >= 5)
		arm_pendsvc();
	else {
		nxt_task = select_next_task(task);
		if (unlikely(nxt_task == (void *)0))
			death_flash();
		if (nxt_task != task && nxt_task->cpri < task->cpri)
			arm_pendsvc();
	}
}

void sched_yield(void)
{
	struct Task_Info *task;

	task = current_task();
	task->acc_ticks += task->time_slice;
	task->time_slice = 0;
	svc_switch();
}

void mdelay(uint32_t msec)
{
	int ticks, curtick, expired, i;
	struct Task_Info *task;

	task = current_task();
	ticks = (int)msec2tick(msec);
	curtick = (int)osticks->tick_low;
	expired = curtick + ticks;
	if (ticks < 5) {
		while (curtick < expired) {
			wait_interrupt();
			curtick = (int)osticks->tick_low;
		}
	} else {
		for (i = 0; i < MAX_NUM_TIMERS; i++) {
			if (ktimers[i].task == (void *)0)
				break;
		}
		ktimers[i].eticks = ticks;
		task->stat = BLOCKED;
		task->timer = ktimers + i;
		asm volatile ("dmb");
		ktimers[i].task = task;
		sched_yield();
	}
}

void __attribute__((naked, noreturn)) task_reaper(void)
{
	struct Task_Info *task;

	task = current_task();
	klog("Task: %x ended. Used sys ticks: %d\n", (uint32_t)task,
			task->acc_ticks);
	task->stat = NONE;
	asm volatile ("dmb");
	task->bpri = BOT;
	task->cpri = BOT;
	sched_yield();
	do {
		wait_interrupt();
	} while (1);
}

void task_info(const struct Task_Info *task)
{
	int i;
	struct proc_stack *pstack;

	for (i = 0; i < MAX_NUM_TASKS; i++) {
		pstack = pstacks + i;
		if ((void *)pstack == (void *)task)
			break;
	}
	if (i == MAX_NUM_TASKS || task->stat == NONE) {
		klog("No such task: %x\n", (uint32_t)task);
		return;
	}
	klog("State: %d, Current Priority: %d, Base Priority: %d, Ticks: %d\n",
		       (int)task->stat, (int)task->cpri, (int)task->bpri, task->acc_ticks);

}

int task_list(struct Task_Info *tasks[], int num)
{
	int i, seq;
	struct proc_stack *pstack;
	struct Task_Info *task;

	for (i = 0, seq = 0; i < MAX_NUM_TASKS; i++) {
		pstack = pstacks + i;
		task = (struct Task_Info *)pstack;
		if (task->stat == NONE)
			continue;
		if (seq < num)
			tasks[seq++] = task;
	}
	return seq;
}

int task_del(struct Task_Info *task)
{
	struct Task_Info *itask;
	struct proc_stack *pstack;
	int i, retv;

	retv = 0;
	itask = (struct Task_Info *)(pstacks + MAX_NUM_TASKS - 1);
	if (task == itask) {
		klog("idle task: %x is reserved. Cannot be deleted\n", (uint32_t)itask);
		return -(MODULE + ETRESVD);
	}
	for (i = 0; i < MAX_NUM_TASKS; i++) {
		pstack = pstacks + i;
		itask = (struct Task_Info *)pstack;
		if (task == itask)
			break;
	}
	if (i == MAX_NUM_TASKS) {
		retv = -(MODULE + ENOSTASK);
		klog("No such task: %x\n", (uint32_t)task);
	} else {
		if (task->timer) {
			task->timer->task = NULL;
			task->timer = NULL;
		}
		task->stat = NONE;
		task->bpri = BOT;
		task->cpri = BOT;
		klog("Task: %x deleted\n", (uint32_t)task);
	}
	return retv;
}
