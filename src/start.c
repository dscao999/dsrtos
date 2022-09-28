#include <stdint.h>
#include <stdbool.h>
#include "misc_utils.h"
#include "armv7m_utils.h"
#include "kernel_internal.h"
#include "kernel.h"
#include "board_def.h"

void main(void);

volatile int errno = 0;

typedef void (*init_func_t)(void);

extern init_func_t __init_start[];
extern init_func_t __init_end[];

void kernel_start(void)
{
	init_func_t init_f, *func;
	void *mstack, *pstack;

	func =  __init_start;
	while (func < __init_end) {
		init_f = *func++;
		init_f();
	}
	errno = 0;
	mstack = main_stack + MAX_MSTACK_SIZE;
	pstack = pstacks + MAX_NUM_TASKS;
	switch_stack(mstack, pstack);
}
