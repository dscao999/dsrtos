#ifndef TASK_DSCAO__ 
#define TASK_DSCAO__ 
#include "kernel_internal.h"
#include "armv7m_utils.h"

#define NULL ((void *)0)

enum TASK_STATE {TASK_FREE = 0, TASK_SUSPEND = 1, TASK_SLEEP = 2,
	TASK_READY = 3, TASK_RUN = 4};
enum TASK_PRIORITY {PRIO_EMERGENCY = 0, PRIO_TOP = 1, PRIO_HIGH = 2,
	PRIO_MID = 4, PRIO_LOW = 8, PRIO_BOT = 16, PRIO_MAXLOW = 255};
enum KTIMER_STATE {TIMER_FREE = 0, TIMER_STOP = 1, TIMER_ARMED = 3};

#define NO_TASK_READY	1 /* no task in ready/run state */

struct Task_Timer;

struct Task_Info {
	volatile int lock;
	void *psp;
	struct Task_Timer *timer;
	uint32_t acc_ticks;
	enum TASK_STATE stat, last_stat;
	enum TASK_PRIORITY cpri, bpri;
	uint8_t time_slice;
};

struct Task_Timer {
	struct Task_Info *task;
	int eticks;
	volatile enum KTIMER_STATE stat;
};

static inline struct Task_Info * current_task(void)
{
	uint32_t curpsp, intrnum;

	asm volatile ("mrs %0, ipsr":"=r"(intrnum));
	intrnum &= 0x01ff;
	if (__builtin_expect(intrnum, 1))
		asm volatile ("mrs %0, psp":"=r"(curpsp));
	else
		asm volatile ("mov %0, sp":"=r"(curpsp));
	return (struct Task_Info *)((curpsp - 1) & PSTACK_MASK);
}

static inline void spin_lock(volatile int *lock)
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
		status = try_lock(lock, (uint32_t)waiter);
	}
}

static inline void un_lock(volatile int *lock)
{
	struct Task_Info *me;

	me = current_task();
	asm volatile ("dmb");
	*lock = 0;
	asm volatile ("dmb");
	me->cpri = me->bpri;
}

void task_slot_init(void);

int create_task(struct Task_Info **handle, enum TASK_PRIORITY prival,
		void *(*task_entry)(void *), void *param);

void mdelay(uint32_t msecs);
void sched_yield(void);

void task_info(const struct Task_Info *task);
int task_list(struct Task_Info *tasks[], int num);
int task_del(struct Task_Info *task);
int task_suspend(struct Task_Info *task);
int task_resume(struct Task_Info *task);

void __attribute__((naked, noreturn)) task_reaper(void);

#endif  /* TASK_DSCAO__ */
