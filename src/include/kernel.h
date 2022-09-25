#ifndef KERNEL_DSCAO__
#define KERNEL_DSCAO__
#include <stdint.h>
#include <stdarg.h>

struct Sys_Tick {
	uint32_t tick_low;
	uint32_t tick_high;
};

extern volatile const struct Sys_Tick * const osticks;
extern volatile int errno;

#define TICK_HZ	250

uint64_t __attribute__((leaf)) current_ticks(void);

static inline uint32_t msec2tick(uint32_t msec)
{
	return (msec * TICK_HZ)/1000;
}

void mdelay(uint32_t msec);

static inline void sched_yield(void)
{
	asm volatile ("wfi");
}

static inline uint16_t current_task_id(void)
{
	return 1;
}

int klog(const char *fmt, ...);

#endif  /* KERNEL_DSCAO__ */
