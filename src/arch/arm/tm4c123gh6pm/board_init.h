#ifndef BOARD_INIT_DSCAO__
#define BOARD_INIT_DSCAO__
#include <stdint.h>
#include "misc_utils.h"

struct DMATable {
	uint32_t src;
	uint32_t dst;
	volatile uint32_t ctrl;
	volatile uint32_t unused;
};

extern struct DMATable udma_table[];
extern struct CirBuf256 * const uart0_buf;
extern volatile int uart0_lock;

void UART0_Handler(void);
void uDMAError_Handler(void);

#endif  /* BOARD_INIT_DSCAO__ */
