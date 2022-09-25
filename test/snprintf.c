#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "misc_utils.h"

static char buf[1024];
const char mymsg[] = "Do you really care?";

int main(int argc, char *argv)
{
	struct timespec curtime;
	unsigned int tv_sec, tv_msec;
	int len, mark;

	clock_gettime(CLOCK_MONOTONIC, &curtime);
	tv_sec = curtime.tv_sec;
	tv_msec = curtime.tv_nsec / 1000;
	len = TEST_RUNsnprintf(buf, sizeof(buf), "Current time: %u.%u\n", tv_sec, tv_msec);
	buf[len] = 0;
	printf("%s", buf);
	sleep(3);
	clock_gettime(CLOCK_MONOTONIC, &curtime);
	tv_sec = curtime.tv_sec;
	tv_msec = curtime.tv_nsec / 1000;
	len = TEST_RUNsnprintf(buf, sizeof(buf), "Current time: %u.%u\n", tv_sec, tv_msec);
	buf[len] = 0;
	printf("%s", buf);

	mark = -tv_sec;
	len = TEST_RUNsnprintf(buf, sizeof(buf), "Negative: %d\n", mark);
	buf[len] = 0;
	printf("%s", buf);

	len = TEST_RUNsnprintf(buf, sizeof(buf), "%s, Neg: %d, Current Time: %u.%u\n", mymsg,
			mark, tv_sec, tv_msec);
	buf[len] = 0;
	printf("%s", buf);

	len = TEST_RUNsnprintf(buf, sizeof(buf), "Hex: %x --- %x\n", tv_sec, tv_msec);
	buf[len] = 0;
	printf("%s", buf);
	printf("Hex: %x --- %x\n", tv_sec, tv_msec);

	return 0;
}

