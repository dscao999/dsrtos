#ifndef TASK_DSCAO__ 
#define TASK_DSCAO__ 
#include "kernel_internal.h"
#include "armv7m_utils.h"

#define NULL ((void *)0)

enum TASK_STATE {TASK_FREE = 0, TASK_STOP = 1, TASK_SUSPEND = 2, TASK_SLEEP = 3,
	TASK_WEVENT = 4, TASK_READY = 5, TASK_RUN = 6};
enum TASK_PRIORITY {PRIO_EMERGENCY = 0, PRIO_TOP = 1, PRIO_HIGH = 2,
	PRIO_MID = 4, PRIO_LOW = 8, PRIO_BOT = 16, PRIO_MAXLOW = 255};
enum KTIMER_STATE {TIMER_FREE = 0, TIMER_STOP = 1, TIMER_ARMED = 3};

#define NO_TASK_READY	1 /* no task in ready/run state */

struct Task_Timer;
struct Completion;

struct Task_Info {
	volatile uint32_t lock;
	void *psp;
	struct Task_Timer *timer;
	struct Completion *cp;
	uint32_t acc_ticks;
	uint32_t retv;
	volatile enum TASK_STATE stat;
	enum TASK_STATE last_stat;
	enum TASK_PRIORITY cpri, bpri;
	uint8_t time_slice;
};

struct Task_Timer {
	struct Task_Info *task;
	int eticks;
	volatile enum KTIMER_STATE stat;
};

struct Completion {
	volatile uint32_t done;
	struct Task_Info *waiter;
};

static inline void completion_init(struct Completion *cp)
{
	cp->waiter = NULL;
	cp->done = 0;
}

static inline struct Task_Info * current_task(void)
{
	uint32_t curpsp;

	if (__builtin_expect(in_interrupt(), 1))
		asm volatile ("mrs %0, psp":"=r"(curpsp));
	else
		asm volatile ("mov %0, sp":"=r"(curpsp));
	return (struct Task_Info *)((curpsp - 1) & PSTACK_MASK);
}

void spin_lock(volatile uint32_t *lock);

static inline void un_lock(volatile uint32_t *lock)
{
	struct Task_Info *me;

	me = current_task();
	asm volatile ("dmb");
	*lock = 0;
	asm volatile ("dmb");
	me->cpri = me->bpri;
}

void task_slot_init(void);

int create_delay_task(struct Task_Info **handle, enum TASK_PRIORITY prival,
		void *(*task_entry)(void *), void *param, uint32_t msecs);
static inline int create_task(struct Task_Info **handle,
		enum TASK_PRIORITY prival,
		void *(*task_entry)(void *), void *param)
{
	return create_delay_task(handle, prival, task_entry, param, 0);
}

void mdelay(uint32_t msecs);
void sched_yield_specific(enum TASK_STATE nxt_stat);
static inline void sched_yield(void)
{
	sched_yield_specific(TASK_READY);
}

void task_info(const struct Task_Info *task);
int task_list(struct Task_Info *tasks[], int num);
int task_del(struct Task_Info *task);
int task_suspend(struct Task_Info *task);
int task_resume(struct Task_Info *task);

void complete(struct Completion *cp);
void wait_for_completion(struct Completion *cp);

void __attribute__((naked, noreturn)) task_reaper(void);

#endif  /* TASK_DSCAO__ */
