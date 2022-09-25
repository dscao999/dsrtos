#include "armv7m_utils.h"

int try_lock(volatile int *lv)
{
	int v;
	
	v = lock_lock(lv);
	return v;
}
