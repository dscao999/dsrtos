#ifndef TASK_DSCAO__ 
#define TASK_DSCAO__ 
#include "kernel_internal.h"

enum TASK_STATE {NONE = 0, BLOCKED = 1, READY = 2, RUN = 3};
enum TASK_PRIORITY {TOP = 1, HIGH = 2, MID = 4, LOW = 8, BOT = 16};

#define TASK_PRIO_MAXLOW	255

#define NO_TASK_READY	1 /* no task in ready/run state */

struct Task_Timer;

struct Task_Info {
	void *psp;
	struct Task_Timer *timer;
	uint32_t acc_ticks;
	enum TASK_STATE stat;
	enum TASK_PRIORITY cpri, bpri;
	uint8_t time_slice;
};

struct Task_Timer {
	int eticks;
	struct Task_Info *task;
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

void task_slot_init(void);

int create_task(struct Task_Info **handle, enum TASK_PRIORITY prival,
		void *(*task_entry)(void *), void *param);

void mdelay(uint32_t msecs);
void sched_yield(void);

void task_info(const struct Task_Info *task);
int task_list(struct Task_Info *tasks[], int num);
int task_del(struct Task_Info *task);

void __attribute__((naked, noreturn)) task_reaper(void);

#endif  /* TASK_DSCAO__ */
