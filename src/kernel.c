#include <stdint.h>
#include "misc_utils.h"
#include "kernel_internal.h"
#include "board_def.h"
#include "kernel.h"

__attribute__ ((section(".proc_stacks"))) struct proc_stack pstacks[MAX_NUM_TASKS];
__attribute__ ((section(".main_stack"))) uint32_t main_stack[MAX_MSTACK_SIZE];

void mdelay(uint32_t msec)
{
	int ticks, curtick, expired;

	ticks = (int)msec2tick(msec);
	curtick = (int)osticks->tick_low;
	expired = curtick + ticks;
	while (curtick < expired) {
		asm volatile ("wfi");
		curtick = (int)osticks->tick_low;
	}
}

uint64_t __attribute__((leaf)) current_ticks(void)
{
        union {
                struct Sys_Tick cur;
                uint64_t tmstmp;
        } ticks;

        ticks.cur.tick_high = osticks->tick_high;
        ticks.cur.tick_low = osticks->tick_low;
        if (__builtin_expect((osticks->tick_high == ticks.cur.tick_high), 1))
                return ticks.tmstmp;
        else {
                ticks.cur.tick_low = osticks->tick_low;
                ticks.cur.tick_high = osticks->tick_high;
        }
        return ticks.tmstmp;
}

int klog(const char *fmt, ...)
{
	char logbuf[256];
	int len, i, revlen;
	uint64_t tmstmp;
	va_list args;

	tmstmp = current_ticks();
	len = snprintf(logbuf, sizeof(logbuf), "[%l] ", tmstmp);
	revlen = 14;
	if (len < revlen) {
		for (i = len - 1; i > 0; i--)
			logbuf[--revlen] = logbuf[i];
		while (--revlen > 0)
			logbuf[revlen] = ' ';
	}
	len = 14;
	va_start(args, fmt);
	len += vsnprintf(logbuf+len, sizeof(logbuf)-len, fmt, args);
	va_end(args);

	console_putstr(logbuf, len);

	return len;
}

void death_flash(int msecs)
{
	int led;

	led = 0;
	if (errno != 0)
		msecs = 100;
	led_off_all();
	do {
		led_light(led, 1);
		mdelay(msecs);
		led_light(led, 0);
		mdelay(msecs);
		led += 1;
	} while (1);
}
