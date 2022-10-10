#ifndef BOARD_DSCAO__
#define BOARD_DSCAO__
#include <stdbool.h>
#include "hw_memmap.h"
#include "gpio.h"
#include "rom.h"
#include "rom_map.h"

#define CPU_HZ	80000000

#define PRIORITY_BITS	3
#define PRIORITY_MASK	0xE0
#define PRIORITY0	0
#define PRIORITY1	(1 << 5)
#define PRIORITY2	(2 << 5)
#define PRIORITY3	(3 << 5)
#define PRIORITY4	(4 << 5)
#define PRIORITY5	(5 << 5)
#define PRIORITY6	(6 << 5)
#define PRIORITY7	(7 << 5)

extern struct CirBuf256 * const uart0_buf;

static inline void led_off_all(void)
{
	MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
}

void led_light(int led, int onoff);
void led_mark(void);
void death_flash();

int console_getstr(char *buf, int buflen);
int console_putstr(const char *buf, int len);

#endif  /* BOARD_DSCAO__ */
