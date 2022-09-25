#ifndef ARMV7M_UTILS_DSCAO__
#define ARMV7M_UTILS_DSCAO__
#include <stdint.h>

void spin_lock(volatile int *lock, uint32_t lv);

#endif  /* ARMV7M_UTILS_DSCAO__ */
