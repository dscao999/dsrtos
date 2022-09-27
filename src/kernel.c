#include <stdint.h>
#include "misc_utils.h"
#include "board_def.h"
#include "kernel.h"

struct proc_stack {
        uint32_t stack[512];
};
__attribute__ ((section(".proc_stacks"), used)) struct proc_stack pstacks[8];
__attribute__ ((section(".main_stack"), used)) uint32_t main_stack[256];

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
