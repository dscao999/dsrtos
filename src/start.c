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
		death_flash();

	task->stat = TASK_RUN;
	task->bpri = PRIO_MAXLOW;
	task->cpri = PRIO_MAXLOW;
	task->acc_ticks = 0;
	task->time_slice = 0;
	task->timer = NULL;
	switch_stack(mstack, pstack);
	/* old stack switched, no local variable usable */
	sched_yield();
	start_systick();

	main();

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

extern volatile uint32_t switched;

static int copy_cmd(char *cmd, const char *rawstr, int len)
{
	char *dst;
	const char *src;

	dst = cmd;
	src = rawstr;
	while (src < rawstr + len) {
		switch(*src) {
			case '\b':
				dst -= 2;
				if (dst < cmd)
					dst = cmd - 1;
				break;
			case ' ':
				*dst = *src;
				while (*(src+1) == ' ')
					src += 1;
				break;
			default:
				*dst = *src;
		}
		dst += 1;
		src += 1;
	}
	return (dst - cmd);
}

static char conin[256], cmd[256];
void idle_task(void)
{
	int msgpos, len, num_tasks, i;
	const char *arg;
	struct Task_Info *task, *tasks[8];

	klog("Idle Task Entered\n");
	msgpos = 0;
	do {
		if (msgpos >= sizeof(conin) - 1)
			msgpos = 0;
		len = console_getstr(conin+msgpos, sizeof(conin) - msgpos);
		if (len == 0)
			continue;
		msgpos += len;
		if (memchr(conin, msgpos, '\r') == -1)
			continue;
		len = copy_cmd(cmd, conin, msgpos);
		cmd[len] = 0;
		if (memcmp(cmd, "tdel ", 5) == 0) {
			arg = cmd + 5;
			task = (struct Task_Info *)hexstr2num(arg, len - 5);
			if (task_del(task) != 0)
				klog("Cannot delete task: %x\n", (uint32_t)task);
		} else if (memcmp(cmd, "tinfo ", 6) == 0) {
			arg = cmd + 6;
			task = (struct Task_Info *)hexstr2num(arg, len - 6);
			task_info(task);
		} else if (memcmp(cmd, "tlist", 5) == 0) {
			num_tasks = task_list(tasks, 8);
			len = snprintf(cmd, sizeof(cmd), "%x", (uint32_t)tasks[0]);
			for (i = 1; i < num_tasks; i++)
				len += snprintf(cmd+len, sizeof(cmd)-len, ", %x", (uint32_t)tasks[i]);
			cmd[len] = 0;
			klog("Current Tasks: %s\n", cmd);
		} else if (memcmp(cmd, "tsuspend ", 9) == 0) {
			arg = cmd + 9;
			task = (struct Task_Info *)hexstr2num(arg, len - 6);
			task_suspend(task);
		} else if (memcmp(cmd, "tresume ", 8) == 0) {
			arg = cmd + 8;
			task = (struct Task_Info *)hexstr2num(arg, len - 6);
			task_resume(task);
		}
		msgpos = 0;
	} while (errno == 0);
	death_flash();
}
