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
	if (num == 0) {
		buf[pos++] = '0';
		return pos;
	}

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

int vsnprintf(char *buf, const int buflen, const char *fmt, va_list args)
{
	const char *pcfmt;
	char c, mark;
	int len, pos;
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
			mark = *pcfmt++;
			pos = 0;
			switch(mark) {
			case 'l':
				num.lnum = va_arg(args, uint64_t);
				pos = long2dec(buf+len, buflen-len, num.high, num.low);
				break;
			case 'd':
				num.snum = va_arg(args, int);
				pos = int2dec(buf+len, buflen-len, num.snum, 1);
				break;
			case 'u':
				num.unum = va_arg(args, unsigned int);
				pos = int2dec(buf+len, buflen-len, num.unum, 0);
				break;
			case 'x':
			case 'X':
				num.unum = va_arg(args, unsigned int);
				pos = int2hex(buf+len, buflen-len, num.unum);
				break;
			case 's':
				num.str = va_arg(args, char *);
				while (*num.str)
					buf[len+pos++] = *num.str++;
				break;
			default:
				buf[pos++] = '?';
				break;
			}
			len += pos;
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

uint32_t hexstr2num(const char *hexstr, int len)
{
	char hex;
	const char *src;
	uint32_t val, digit;

	val = 0;
	src = hexstr;
	while (src < hexstr + len) {
		hex = *src++;
		if ((hex < '0' || hex > '9')&&(hex < 'A' || hex > 'F')&&(hex < 'a' || hex > 'f'))
			break;
		if (hex >= '0' && hex <= '9')
			digit = hex - '0';
		else if (hex >= 'A' && hex <= 'F')
			digit = hex - 'A' + 10;
		else if (hex >= 'a' && hex <= 'f')
			digit = hex - 'a' + 10;
		val = (val << 4) + digit;
	}
	return val;
}
