#include "hw_nvic.h"
#include "armv7m_utils.h"
#include "board_def.h"
#include "kernel_internal.h"
#include "kernel.h"
#include "task.h"

#define ENOMEM		1
#define ENORTASK	2
#define ETRESVD		3
#define ENOTASK		4
#define ENOTIMER	5
#define EINVAL		6
#define ENOTASK_SLOT	7
#define EBUSY		8

static const uint32_t MODULE = 0x10000;

#define NULL	((void *)0)
#define unlikely(x)	__builtin_expect((x), 0)

static struct Task_Timer ktimers[MAX_NUM_TIMERS];
static volatile uint32_t ktimers_lock = 0;
static volatile uint32_t task_slot_lock = 0;

static struct Sys_Tick sys_tick = {.tick_low = 0, .tick_high = 0};
volatile const struct Sys_Tick * const osticks = &sys_tick;

void task_slot_init(void)
{
	struct Task_Info *task;
	int i;

	for (i = 0; i < MAX_NUM_TASKS; i++) {
		task = (struct Task_Info *)(pstacks+i);
		task->stat = TASK_FREE;
		task->lock = 0;
	}
	for (i = 0; i < MAX_NUM_TIMERS; i++)
		ktimers[i].stat = TIMER_FREE;
	ktimers_lock = 0;
	task_slot_lock = 0;
}

static inline int get_task_slot(void)
{
	int i, retv;
	struct Task_Info *task;

	spin_lock(&task_slot_lock);
	for (i = 0; i < MAX_NUM_TASKS; i++) {
		task = (struct Task_Info *)(pstacks + i);
		if (task->stat == TASK_FREE)
			break;
	}
	if (i < MAX_NUM_TASKS)
		task->stat = TASK_STOP;
	un_lock(&task_slot_lock);

	if (i == MAX_NUM_TASKS) {
		klog("Failed to create new task, Too Many Tasks: %x.\n",
				MODULE+ENOMEM);
		retv = -(MODULE + ENOMEM);
	} else
		retv = i;
	return retv;
}

static struct Task_Info * setup_new_task(int slot, enum TASK_PRIORITY prival,
		void * (*task_entry)(void *), void *param)
{
	struct Task_Info *task;
	void *frame;

	task = (struct Task_Info *)(pstacks + slot);
	frame = ((char *)(pstacks + slot + 1)) - sizeof(struct Intr_Context);
	intr_context_setup(frame, (uint32_t)task_entry, param);
	((struct Intr_Context *)frame)->lr = (uint32_t)task_reaper;
	frame = ((char *)frame) - sizeof(struct Reg_Context);
	reg_context_setup(frame);
	task->lock = 0;
	task->psp = frame;
	task->bpri = prival;
	task->cpri = prival;
	task->acc_ticks = 0;
	task->time_slice = 0;
	task->timer = NULL;
	task->cp = NULL;
	return task;
}

static inline int priority_valid(enum TASK_PRIORITY prival)
{
	int retv = 0;

	switch(prival) {
		case PRIO_EMERGENCY:
		case PRIO_TOP:
		case PRIO_HIGH:
		case PRIO_MID:
		case PRIO_LOW:
		case PRIO_BOT:
		case PRIO_MAXLOW:
			break;
		default:
			klog("No such task priority value: %d\n", prival);
			retv = -(MODULE+EINVAL);
	}
	return retv;
}

struct Task_Info * select_next_task(struct Task_Info *current)
{
	struct Task_Info *task;
	enum TASK_PRIORITY pri;
       	int curseq, i, mask, candidate, sel;

	mask = MAX_NUM_TASKS - 1;
	candidate = 0;
	curseq = -1;
	pri = PRIO_MAXLOW;
	for (i = 0; i < MAX_NUM_TASKS; i++) {
		task = (struct Task_Info *)(pstacks+i);
		if (task->stat != TASK_READY && task->stat != TASK_RUN)
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
		klog("No Runnable Task Exists: %x, current state: %x\n", MODULE + ENORTASK, (int)current->stat);
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

static void task_switch_isr(struct Reg_Context *frame, enum TASK_STATE nxt_stat)
{
	struct Task_Info *nxt, *cur;
	struct Reg_Context *cur_frame, *nxt_frame;

	cur = current_task();
	cur->stat = nxt_stat;
	nxt = select_next_task(cur);
	cur->acc_ticks += cur->time_slice;
	cur->time_slice = 0;
	if (cur == nxt) {
		cur->stat = TASK_RUN;
		return;
	}
	if (unlikely(nxt == NULL))
		death_flash();

	if (cur->stat == TASK_RUN)
		cur->stat = TASK_READY;
	cur_frame = getpsp();
	cur_frame = cur_frame - 1;
	*cur_frame = *frame;
	cur->psp = cur_frame;
	nxt_frame = nxt->psp;
	nxt->stat = TASK_RUN;
	*frame = *nxt_frame;
	nxt_frame = nxt_frame + 1;
	setpsp(nxt_frame);
}

static inline void setup_timer_isr(struct Task_Timer *timer)
{
	timer->task->stat = TASK_SLEEP;
	asm volatile ("dmb");
	timer->stat = TIMER_ARMED;
}

void __attribute__((naked)) PendSVC_Handler(void)
{
	struct Reg_Context *frame;
	struct Task_Info *me;

	asm volatile   ("push {r4-r11, lr}\n"	\
			"\tmov %0, sp\n":"=r"(frame));
	me = current_task();
	task_switch_isr(frame, me->stat);
	asm volatile   ("mov sp, %0\n"	\
			"\tpop {r4-r11, pc}\n"::"r"(frame));
}

void SysTick_Handler(void)
{
	volatile uint32_t *scbreg;
	uint32_t scbv;
	int i;
	struct Task_Info *task, *nxt_task;
	struct Task_Timer *timer;

	scbreg = (volatile uint32_t *)NVIC_ST_CTRL;
	scbv = *scbreg;
	if (__builtin_expect((scbv & (1 << 16)) == 0, 0))
		while (1)
			;
	sys_tick.tick_low += 1;
	if (__builtin_expect(sys_tick.tick_low == 0, 0))
		sys_tick.tick_high += 1;
	for (i = 0; i < MAX_NUM_TIMERS; i++) {
		if (ktimers[i].stat != TIMER_ARMED)
			continue;
		timer = ktimers + i;
		if (--timer->eticks == 0) {
			if (likely(timer->task->stat == TASK_SLEEP)) {
				timer->stat = TIMER_FREE;
				timer->task->timer = NULL;
				timer->task->stat = TASK_READY;
			}
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

void sched_yield_specific(enum TASK_STATE nxt_stat)
{
	struct Task_Info *task;

	task = current_task();
	task->acc_ticks += task->time_slice;
	task->time_slice = 0;
	svc_switch(nxt_stat);
}

static inline struct Task_Timer * get_ktimer(void)
{
	int i;
	struct Task_Timer *timer = NULL;

	spin_lock(&ktimers_lock);
	for (i = 0; i < MAX_NUM_TIMERS; i++) {
		if (ktimers[i].stat == TIMER_FREE)
			break;
	}
	if (i < MAX_NUM_TIMERS)
		ktimers[i].stat = TIMER_STOP;
	un_lock(&ktimers_lock);
	if (i == MAX_NUM_TIMERS)
		klog("No more task timers: %x\n", MODULE + ENOTIMER);
	else
		timer = ktimers + i;
	return timer;
}

void mdelay(uint32_t msec)
{
	int ticks, curtick, expired;
	struct Task_Info *me;
	struct Task_Timer *timer;

	ticks = (int)msec2tick(msec);
	curtick = (int)osticks->tick_low;
	expired = curtick + ticks;
	if (ticks < 5) {
		while (curtick < expired) {
			wait_interrupt();
			curtick = (int)osticks->tick_low;
		}
	} else {
		me = current_task();
		timer = get_ktimer();
		if (unlikely(timer == NULL))
			death_flash();
		timer->eticks = ticks;
		timer->task = me;
		me->timer = timer;
		asm volatile ("mov r0, %0\n"	\
				"svc #1"::"r"(timer));
	}
}

void __attribute__((naked, noreturn)) task_reaper(void)
{
	struct Task_Info *task;

	task = current_task();
	asm volatile ("str r0, [%0]"::"r"(&task->retv));
	klog("Task: %x ended. Return value: %x, Used sys ticks: %d\n",
			(uint32_t)task, task->retv, task->acc_ticks);
	sched_yield_specific(TASK_FREE);
	do
		wait_interrupt();
	while (1);
}

static inline int task_valid(const struct Task_Info *task)
{
	int i, retv = 1;
	struct proc_stack *pstack;

	for (i = 0; i < MAX_NUM_TASKS; i++) {
		pstack = pstacks + i;
		if ((void *)pstack == (void *)task)
			break;
	}
	if (i == MAX_NUM_TASKS || task->stat == TASK_FREE)
		retv = 0;
	return retv;
}

void task_info(const struct Task_Info *task)
{
	if (!task_valid(task)) {
		klog("No such task: %x\n", (uint32_t)task);
		return;
	}
	klog("State: %d, Current Priority: %d, Base Priority: %d, Ticks: %d\n",
		       (int)task->stat, (int)task->cpri, (int)task->bpri,
		       task->acc_ticks);

}

int task_list(struct Task_Info *tasks[], int num)
{
	int i, seq;
	struct proc_stack *pstack;
	struct Task_Info *task;

	for (i = 0, seq = 0; i < MAX_NUM_TASKS; i++) {
		pstack = pstacks + i;
		task = (struct Task_Info *)pstack;
		if (task->stat == TASK_FREE)
			continue;
		if (seq < num)
			tasks[seq++] = task;
	}
	return seq;
}

static int task_mutable(struct Task_Info *task)
{
	struct Task_Info *idle_task, *me;
	int retv = 0;

	if (!task_valid(task)) {
		retv = -(MODULE + ENOTASK);
		klog("No such task: %x\n", (uint32_t)task);
		return retv;
	}
	idle_task = (struct Task_Info *)(pstacks + MAX_NUM_TASKS - 1);
	if (task == idle_task) {
		klog("idle task: %x is reserved. Cannot be manipulated\n",
				(uint32_t)task);
		return -(MODULE + ETRESVD);
	}
	me = current_task();
	if (task == me) {
		klog("Cannot manipulate myself: %x\n", (uint32_t)me);
		return -(MODULE + EINVAL);
	}
	return 0;
}

int task_del(struct Task_Info *task)
{
	int retv;

	retv = task_mutable(task);
	if (retv == 0) {
		task->stat = TASK_FREE;
		klog("Task: %x deleted\n", (uint32_t)task);
	}
	return retv;
}

int task_suspend(struct Task_Info *task)
{
	int retv = 0;

	retv = task_mutable(task);
	if (retv != 0)
		return retv;
	if (task->stat == TASK_SUSPEND)
		goto exit_10;
	if (task->stat == TASK_WEVENT) {
		retv = -(MODULE + EINVAL);
		klog("Task %x in wait for event state, cannot be suspended\n",
				(uint32_t)task);
		goto exit_10;
	}
	spin_lock(&task->lock);
	if (task->stat == TASK_SLEEP)
		task->timer->stat = TIMER_STOP;
	task->last_stat = task->stat;
	task->stat = TASK_SUSPEND;
	un_lock(&task->lock);
exit_10:
	return retv;
}

int task_resume(struct Task_Info *task)
{
	int retv = 0;

	if (!task_valid(task)) {
		klog("No such task: %x\n", (uint32_t)task);
		retv = -(MODULE + ENOTASK);
		goto exit_10;
	}
	if (task->stat != TASK_SUSPEND)
		goto exit_10;
	spin_lock(&task->lock);
	task->stat = task->last_stat;
	if (task->stat == TASK_SLEEP)
		task->timer->stat = TIMER_ARMED;
	un_lock(&task->lock);
exit_10:
	return retv;
}

int create_delay_task(struct Task_Info **handle, enum TASK_PRIORITY prival,
		void *(*task_entry)(void *), void *param, uint32_t msecs)
{
	int retv, slot;
	struct Task_Timer *timer;
	struct Task_Info *task;
	uint32_t ticks;

	*handle = NULL;
	retv = priority_valid(prival);
	if (retv < 0)
		return retv;
	ticks = msec2tick(msecs);
	timer = get_ktimer();
	if (!timer) {
		retv = -(MODULE + ENOTIMER);
		return retv;
	}
	slot = get_task_slot();
	if (slot < 0) {
		timer->stat = TIMER_FREE;
		retv = -(MODULE + ENOTASK_SLOT);
		return retv;
	}
	task = setup_new_task(slot, prival, task_entry, param);
	task->stat = TASK_SLEEP;
	task->timer = timer;
	timer->task = task;
	timer->eticks = ticks == 0? 1 : ticks;
	*handle = task;
	asm volatile ("dmb");
	timer->stat = TIMER_ARMED;
	return retv;
}

void spin_lock(volatile uint32_t *lock)
{
	int status;
	struct Task_Info *holder, *waiter;

	waiter = current_task();
	status = try_lock(lock, (uint32_t)waiter);
	while (status != 0) {
		if (status != 1) {
			holder = (struct Task_Info *)status;
			if (waiter->cpri < holder->cpri)
				holder->cpri = waiter->cpri;
		}
		wait_event();
		status = try_lock(lock, (uint32_t)waiter);
	}
}

void complete(struct Completion *cp)
{
	int status;
	uint32_t mark;
	struct Task_Info *waiter;

	cp->done = 1;
	waiter = NULL;
	mark = in_interrupt();
	if (!mark)
		mark = ((uint32_t)current_task())|0x0ff;
	do
		status = try_compswap((uint32_t *)&cp->waiter,
				(uint32_t *)&waiter, mark);
	while (status == 1);
	if (!task_valid(waiter))
		return;
	if (waiter->cp == cp && waiter->stat == TASK_WEVENT)
		waiter->stat = TASK_READY;
}

int wait_for_completion(struct Completion *cp)
{
	int retv = 0, status;
	struct Task_Info *me, *waiter;

	if (cp->done == 1)
		return retv;

	waiter = NULL;
	me = current_task();
	me->cp = cp;
	do
		status = try_compswap((uint32_t *)&cp->waiter,
				(uint32_t *)&waiter, (uint32_t)me);
	while (status == 1 && waiter == NULL);
	if (waiter) {
	       	if (task_valid(waiter)) {
			retv = -(MODULE + EBUSY);
			klog("struct Completion %x is busy: %x\n", (uint32_t)cp, -retv);
		}
	} else {
			asm volatile (  "mov r0, %0\n"		\
					"\tsvc #2\n"::"r"(me));
	}
	me->cp = NULL;
	return retv;
}

static int wait_event_isr(struct Task_Info *waiter)
{
	int retv;

	retv = 0;
	disable_interrupt();
	if (waiter->cp->done != 1)
		waiter->stat = TASK_WEVENT;
	enable_interrupt();
	return retv;
}

void __attribute__((naked)) SVC_Handler(void)
{
	struct Reg_Context *frame;
	struct Intr_Context *intr_frame;
	uint16_t inst;
	uint32_t arg1;
	int retv = 0;

	asm volatile   ("push {r4-r11, lr}\n"	\
			"\tmov %0, sp\n":"=r"(frame));
	intr_frame = getpsp();
	arg1 = intr_frame->r0;
	inst = *((uint16_t *)(intr_frame->retadr - 2));
	switch(inst & 0x0ff) {
	case 0: /* task switch */
		task_switch_isr(frame, arg1);
		break;
	case 1:  /* setup delay timer */
		setup_timer_isr((struct Task_Timer *)arg1);
		arm_pendsvc();
		break;
	case 2:
		wait_event_isr((struct Task_Info *)arg1);
		arm_pendsvc();
		break;
	default:
		klog("No such svc call number: %d\n", (int)(inst & 0x0ff));
		death_flash();
	}
	intr_frame->r0 = (uint32_t)retv;
	asm volatile   ("mov sp, %0\n"	\
			"\tpop {r4-r11, pc}\n"::"r"(frame));
}
