#include <stdint.h>
#include <stdbool.h>
#include "misc_utils.h"
#include "kernel.h"
#include "board_def.h"

volatile int errno = 0;
extern volatile int dma_times;

typedef void (*init_func_t)(void);

extern init_func_t __init_start[];
extern init_func_t __init_end[];

static struct {
	char ln;
	char dmesg[256];
} msgstr;



void main(void)
{
	init_func_t init_f, *func;
	int msgpos;
	char *imsg;

	func =  __init_start;
	while (func < __init_end) {
		init_f = *func++;
		init_f();
	}

	msgstr.ln = '\n';
	msgpos = 0;
	errno = 0;
	imsg = msgstr.dmesg;
	do {
		if (msgpos >= sizeof(msgstr.dmesg) - 1)
			msgpos = 0;
		msgpos += console_getstr(imsg+msgpos, sizeof(msgstr.dmesg) - 1 - msgpos);
		if (msgpos > 0 && memchr(imsg, msgpos, '\r') != -1) {
			klog("current ticks: %u\n", osticks->tick_low);
			msgpos = 0;
		}
		if (errno) {
			led_light(3, 1);
			while (1)
				;
		}
	} while (1);
}
