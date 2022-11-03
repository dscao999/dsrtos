#include "kernel.h"
#include "board_def.h"
#include "misc_utils.h"
#include "task.h"

#define unlikely(x) __builtin_expect((x), 0)
struct LED_FLASH {
	int led;
	int msecs;
};

static void * led_flash(void *param)
{
	struct LED_FLASH *ledinfo = param;

	do {
		led_light(ledinfo->led, 1);
		mdelay(ledinfo->msecs);
		led_light(ledinfo->led, 0);
		mdelay(ledinfo->msecs);
	} while (errno == 0);
	return (void *)0;
}

struct LED_FLASH ledspec;

void * timed_hello(void *param)
{
	uint32_t sec = (uint32_t)param;
	uint32_t tick = 0;

	do {
		klog("Ticking: %u\n", tick++);
		mdelay(sec*1000);
	} while (tick < 10);
	return (void *)0xceed;
}

struct Delay_Mesg {
	char *mesg;
	struct Task_Info *wait_for;
} dmesg;

void * delayed_output(void *param)
{
	struct Delay_Mesg *mesg = param;
	void *retval;
	int retv;

	klog("%s. Waiting for task %x\n", mesg->mesg, (uint32_t)mesg->wait_for);
	retv = wait_task(mesg->wait_for, &retval);
	if (retv == 1)
		klog("Task %x exited with %x\n", (uint32_t)mesg->wait_for,
				retval);
	else if (retv < 0)
		klog("Wait for task %x failed\n", (uint32_t)mesg->wait_for);
	else
		klog("Task %x already exited\n", (uint32_t)mesg->wait_for);
	return (void *)0xabcd;
}

void main(void)
{
	struct Task_Info *task_handle;
	int retv;

	ledspec.led = 3;
	ledspec.msecs = 500;
	task_handle = (void *)0;
	retv = create_task(&task_handle, PRIO_HIGH, led_flash, &ledspec);
	if (unlikely(retv < 0))
		death_flash();
	klog("New Task Created: %x\n", (uint32_t)task_handle);
	retv = create_task(&task_handle, PRIO_BOT, timed_hello, (void *)5);
	if (unlikely(retv < 0))
		death_flash();
	klog("New Task Created: %x\n", (uint32_t)task_handle);
	dmesg.wait_for = task_handle;
	dmesg.mesg = "It's wonderful";
	retv = create_delay_task(&task_handle, PRIO_TOP, delayed_output,
			&dmesg, 19000);
	if (unlikely(retv < 0))
		death_flash();
	klog("New Task Created: %x\n", (uint32_t)task_handle);
}
