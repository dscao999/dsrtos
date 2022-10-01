#ifndef BOARD_DSCAO__
#define BOARD_DSCAO__

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

void led_light(int led, int onoff);
void led_mark(void);
void dead_flash(int msecs);

int console_getstr(char *buf, int buflen);
int console_putstr(const char *buf, int len);

#endif  /* BOARD_DSCAO__ */
