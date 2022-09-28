#include "kernel.h"
#include "board_def.h"
#include "misc_utils.h"

static char conin[256];

void main(void)
{
	int msgpos;
	uint32_t ctl;

	msgpos = 0;
	do {
		if (errno) {
		}
		if (msgpos >= sizeof(conin) - 1)
			msgpos = 0;
		msgpos += console_getstr(conin+msgpos, sizeof(conin) - msgpos);
		if (msgpos == 0 || memchr(conin, msgpos, '\r') == -1)
			continue;
		switch(conin[0]) {
			case '0':
				asm volatile ("mrs	%0, control\n":"=r"(ctl));
				klog("Control Reg: %x\n", ctl);
				break;
			case '1':
				asm volatile ("ldr	r2, =0xE000ED14\n"	\
						"ldr	%0, [r2]\n"	\
						:"=r"(ctl)::"r2");
				klog("Control Reg: %x\n", ctl);
				break;
			default:
				klog("current ticks: %u\n", osticks->tick_low);
				break;
		}
		msgpos = 0;
	} while (errno == 0);
	led_light(3, 1);
	while (1);
}
