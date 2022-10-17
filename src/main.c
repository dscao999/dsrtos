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
}
