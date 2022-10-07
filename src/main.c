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
	} while (1);
	return (void *)0;
}

struct LED_FLASH ledspec;

void main(void)
{
	struct Task_Info *task_handle;
	int retv;

	ledspec.led = 3;
	ledspec.msecs = 200;
	task_handle = (void *)0;
	klog("Starting Tasks Now!\n");
	retv = create_task(&task_handle, led_flash, &ledspec);
	if (unlikely(retv == -1))
		death_flash(200);
	klog("New Task Created: %x\n", (uint32_t)task_handle);
}
