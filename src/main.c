#include <stdint.h>
#include <stdbool.h>
#include "misc_utils.h"
#include "armv7m_utils.h"
#include "kernel.h"
#include "board_def.h"

volatile int errno = 0;
extern volatile int dma_times;

typedef void (*init_func_t)(void);

extern init_func_t __init_start[];
extern init_func_t __init_end[];

static char conin[256];
void kernel_start(void)
{
	init_func_t init_f, *func;
	int msgpos;
	uint32_t ctl;

	func =  __init_start;
	while (func < __init_end) {
		init_f = *func++;
		init_f();
	}

	msgpos = 0;
	errno = 0;
	do {
		if (errno) {
			led_light(3, 1);
			while (1)
				;
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
	} while (1);
}
