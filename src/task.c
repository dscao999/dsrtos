#include "hw_nvic.h"
#include "armv7m_utils.h"
#include "board_def.h"
#include "kernel_internal.h"
#include "kernel.h"
#include "task.h"

//static const uint32_t TASK_MODULE = 0x50000;
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
		task->bpri = TASK_PRIO_MAXLOW;
		task->cpri = TASK_PRIO_MAXLOW;
	}
	for (i = 0; i < MAX_NUM_TIMERS; i++) {
		ktimers[i].eticks = 0;
		ktimers[i].task = (void *)0;
	}
}

int create_task(struct Task_Info **handle, uint32_t prival,
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
		klog("Failed to create new task: Too Many Tasks\n");
		return -1;
	}
	frame = ((char *)(pstacks + i + 1)) - sizeof(struct Intr_Context);
	intr_context_setup(frame, (uint32_t)task_entry, param);
	frame = ((char *)frame) - sizeof(struct Reg_Context);
	reg_context_setup(frame);
	task->psp = frame;
	if (prival > TASK_PRIO_MAXLOW)
		prival = TASK_PRIO_MAXLOW;
	task->bpri = prival;
	task->cpri = prival;
	asm volatile ("dmb");
	task->stat = READY;
	*handle = task;
	return 0;
}

struct Task_Info * select_next_task(struct Task_Info *current)
{
	struct Task_Info *task;
	uint8_t pri;
       	int curseq, i, mask, candidate, sel;

	mask = MAX_NUM_TASKS - 1;
	candidate = 0;
	curseq = -1;
	pri = TASK_PRIO_MAXLOW;
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
	if (candidate == 0)
		return (void *)0;

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

exit_10:
	asm volatile   ("mov sp, %0\n"	\
			"\tpop {r4-r11, pc}\n"::"r"(frame));
	return;
}

void __attribute__((naked)) PendSVC_Handler(void)
{
	SVC_Handler();
}

volatile uint32_t switched = 0;

static inline void switch_task()
{
	volatile uint32_t *int_ctrl;
	uint32_t val;

	int_ctrl = (volatile uint32_t *)NVIC_INT_CTRL;
	val = *int_ctrl;
	val |= (1 << 28);
	*int_ctrl = val;
	switched += 1;
}

void SysTick_Handler(void)
{
	volatile uint32_t *scbreg;
	uint32_t scbv;
	int i, do_switch;
	struct Task_Info *task, *nxt_task;
	static struct Task_Info *prev_task = (void *)0;

	scbreg = (volatile uint32_t *)NVIC_ST_CTRL;
	scbv = *scbreg;
	if (__builtin_expect((scbv & (1 << 16)) == 0, 0))
		while (1)
			;
	sys_tick.tick_low += 1;
	if (__builtin_expect(sys_tick.tick_low == 0, 0))
		sys_tick.tick_high += 1;
	for (i = 0; i < MAX_NUM_TIMERS; i++) {
		if (ktimers[i].task == (void *)0)
			continue;
		if (--ktimers[i].eticks == 0) {
			ktimers[i].task->stat = READY;
			ktimers[i].task = (void *)0;
		}
	}
	do_switch = 0;
	task = current_task();
	if (task == prev_task) {
		if ((sys_tick.tick_low & 7)  == 0) {
			do_switch = 1;
			switch_task();
		}
	} else
		prev_task = task;
	if (do_switch == 0) {
		nxt_task = select_next_task(task);
		if (unlikely(nxt_task == (void *)0))
			death_flash();
		if (nxt_task != task && nxt_task->cpri < task->cpri)
			switch_task();
	}
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
			sched_wait();
			curtick = (int)osticks->tick_low;
		}
	} else {
		for (i = 0; i < MAX_NUM_TIMERS; i++) {
			if (ktimers[i].task == (void *)0)
				break;
		}
		ktimers[i].eticks = ticks;
		task->stat = BLOCKED;
		asm volatile ("dmb");
		ktimers[i].task = task;
		sched_yield();
	}
}
