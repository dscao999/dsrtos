#ifndef KERNEL_INTERNAL_DSCAO__
#define KERNEL_INTERNAL_DSCAO__
#include <stdint.h>

#define MAX_NUM_TASKS	8
#define PSTACK_MASK	0xfffff800
#define MAX_PSTACK_SIZE	512  /* task stack size in 4-byte words */
#define MAX_MSTACK_SIZE 256  /* main stack size in 4-byte words */

struct proc_stack {
        uint32_t stack[MAX_PSTACK_SIZE];
};

extern struct proc_stack pstacks[MAX_NUM_TASKS];
extern uint32_t main_stack[MAX_MSTACK_SIZE];


#endif  /* KERNEL_INTERNAL_DSCAO__ */
