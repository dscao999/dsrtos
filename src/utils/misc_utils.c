#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include "misc_utils.h"

static const uint32_t C = 429496729;
static inline uint32_t uint32_add(uint32_t a1, uint32_t a2, uint32_t *ov)
{
	uint32_t sum;

	sum = a1 + a2;
	if (sum < a1)
		*ov = *ov + 1;
	return sum;
}

static void divrem_u64(uint32_t *high, uint32_t *low, uint32_t *rem)
{
	uint32_t h, hrem, l, lrem;
	
	h = *high / 10;
	hrem = *high % 10;
	l = *low / 10;
	lrem = *low % 10;
	l = uint32_add(l, hrem * C, &h);
	l = uint32_add(l, (6*hrem + lrem) / 10, &h);
	*rem = (6*hrem + lrem) % 10;
	*high = h;
	*low = l;
}

static int long2dec(char *buf, int buflen, uint32_t high, uint32_t low)
{
	int pos = 0, tpos = 0;
	char tmpbuf[24];
	uint32_t H, L, digit;

	if (buflen <= 0)
		return pos;

	if (high == 0 && low == 0) {
		buf[pos++] = '0';
		return pos;
	}
	H = high;
	L = low;
	while (H != 0 || L != 0) {
		divrem_u64(&H, &L, &digit);
		tmpbuf[tpos++] = '0' + digit;
	}
	while (--tpos >= 0 && pos < buflen)
		buf[pos++] = tmpbuf[tpos];
	return pos;
}

static int int2dec(char *buf, int buflen, int num, int sign)
{
	int pos = 0, tpos = 0, digit;
	char tmpbuf[16];
	unsigned int tnum;

	if (buflen <= 0)
		return pos;

	if (num == 0) {
		buf[pos++] = '0';
		return pos;
	} else if (num < 0 && sign) {
		tnum = (~num) + 1;
		buf[pos++] = '-';
	} else
		tnum = (unsigned int)num;
	while (tnum != 0) {
		digit = tnum % 10;
		tmpbuf[tpos++] = '0' + digit;
		tnum = tnum / 10;
	}
	while (--tpos >= 0 && pos < buflen)
		buf[pos++] = tmpbuf[tpos];
	return pos;
}

static int int2hex(char *buf, int buflen, unsigned int num)
{
	int pos = 0, tpos = 0, digit;
	char tmpbuf[16];

	if (buflen <= 0)
		return pos;
	while (num != 0) {
		digit = num & 0x0f;
		if (digit > 9)
			tmpbuf[tpos++] = 'A' + digit - 10;
		else
			tmpbuf[tpos++] = '0' + digit;
		num >>= 4;
	}
	while (--tpos >= 0 && pos < buflen)
		buf[pos++] = tmpbuf[tpos];
	return pos;
}

static int print_one(char *buf, int buflen, const char mark, va_list args)
{
	union {
		uint64_t lnum;
		struct {
			uint32_t low;
			uint32_t high;
		};
		int32_t  snum;
		uint32_t unum;
		char    *str;
	} num;
	int pos;

	pos = 0;
	if (buflen <= 0)
		return pos;

	switch(mark) {
		case 'l':
			num.lnum = va_arg(args, uint64_t);
			pos = long2dec(buf, buflen, num.high, num.low);
			break;
		case 'd':
			num.snum = va_arg(args, int);
			pos = int2dec(buf, buflen, num.snum, 1);
			break;
		case 'u':
			num.unum = va_arg(args, unsigned int);
			pos = int2dec(buf, buflen, num.unum, 0);
			break;
		case 'x':
		case 'X':
			num.unum = va_arg(args, unsigned int);
			pos = int2hex(buf, buflen, num.unum);
			break;
		case 's':
			num.str = va_arg(args, char *);
			while (*num.str)
				buf[pos++] = *num.str++;
			break;
		default:
			buf[pos++] = '?';
			break;
	}

	return pos;
}

int vsnprintf(char *buf, const int buflen, const char *fmt, va_list args)
{
	const char *pcfmt;
	char c;
	int len;

	len = 0;
	pcfmt = fmt;
	do {
		c = *pcfmt++;
		switch(c) {
			case '\\':
				c = *pcfmt++;
				if (c == 'n')
					c = '\n';
				buf[len++] = c;
				break;
			case '%':
				len += print_one(buf+len, buflen-len, *pcfmt++,
						args);
				break;
			case 0:
				break;
			default:
				buf[len++] = c;
				break;
		}
	} while (len < buflen && c != 0);
	return len;
}

int snprintf(char *buf, const int buflen, const char *fmt, ...)
{
	int len;
	va_list args;

	va_start(args, fmt);

	len = vsnprintf(buf, buflen, fmt, args);

	va_end(args);
	return len;
}

void memcpy(char *dst, const char *src, int len)
{
	const char *curchr;

	for (curchr = src; curchr < src + len; curchr++, dst++)
		*dst = *curchr;
}

int memchr(const char *msg, int len, char c)
{
	int idx;

	if (len <= 0)
		return -1;
	idx = 0;
	do {
		if (msg[idx] == c)
			return idx;
		idx += 1;
	} while (idx < len);
	return -1;
}

void memset(char *dst, int val, int len)
{
	char *curchr;

	curchr = dst;
	while (curchr < dst + len)
		*curchr++ = val;
}

int memcmp(const char *op1, const char *op2, int len)
{
	int retv;
	const char *op;
	char v1, v2;

	retv = 0;
	if (len == 0)
		return retv;
	op = op1;
	while (op < op1 + len && retv == 0) {
		v1 = *op++;
		v2 = *op2++;
		if (v1 == v2)
			continue;
		else if (v1 < v2)
			retv = -1;
		else if (v1 > v2)
			retv = 1;
	}
	return retv;
}
