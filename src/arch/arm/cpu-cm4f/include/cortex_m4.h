#ifndef CORTEX_M4_DSCAO__
#define CORTEX_M4_DSCAO__
#include <stdint.h>
#include <hw_nvic.h>
#include <kernel.h>
#include <board.h>

static inline void fpu_enable(void)
{
	uint32_t cpac;
	volatile uint32_t *nvic_cpac = (volatile uint32_t *)NVIC_CPAC;

	cpac = *nvic_cpac;
	cpac &=  ~(NVIC_CPAC_CP10_M | NVIC_CPAC_CP11_M);
	cpac |= (NVIC_CPAC_CP10_FULL | NVIC_CPAC_CP11_FULL);
	*nvic_cpac = cpac;
}

#endif  /* CORTEX_M4_DSCAO__ */
