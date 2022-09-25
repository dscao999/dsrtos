#ifndef MISC_UTILS_DSCAO__
#define MISC_UTILS_DSCAO__
#include <stdint.h>
#include <stdarg.h>

struct CirBuf256 {
	uint8_t head;
	uint8_t tail;
	uint8_t items[256];
};

struct CirBuf1K {
	uint16_t head;
	uint16_t tail;
	uint8_t items[1024];
};

static inline int cirbuf_empty(const struct CirBuf256 *cbuf)
{
	return cbuf->head == cbuf->tail;
}

static inline int cirbuf1K_empty(const struct CirBuf1K *cbuf)
{
	return cbuf->head == cbuf->tail;
}

static inline uint8_t cirbuf_get(struct CirBuf256 *cbuf)
{
	return cbuf->items[cbuf->tail++];
}

static inline uint8_t cirbuf1K_get(struct CirBuf1K *cbuf)
{
	uint8_t item;

	item = cbuf->items[cbuf->tail++];
	cbuf->tail &= 0x3ff;
	return item;
}

static inline void cirbuf1K_put(struct CirBuf1K *cbuf, uint8_t val)
{
	cbuf->items[cbuf->head++] = val;
	cbuf->head &= 0x3ff;
}

static inline void cirbuf_put(struct CirBuf256 *cbuf, uint8_t val)
{
	cbuf->items[cbuf->head++] = val;
}

int snprintf(char *buf, int buflen, const char *fmt, ...);
int vsnprintf(char *buf, int buflen, const char *fmt, va_list args);

static inline void memcpy(char *dst, const char *src, int len)
{
        const char *curchr;

	if (len <= 0)
		return;
        for (curchr = src; curchr < src + len; curchr++, dst++)
                *dst = *curchr;
}

static inline int memchr(const char *msg, int len, char c)
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

int memcmp(const char *op1, const char *op2, int len);

static inline void memset(char *dst, int val, int len)
{
        char *curchr;

	if (len <= 0)
		return;
        curchr = dst;
        while (curchr < dst + len)
                *curchr++ = val;
}

#endif  /* MISC_UTILS_DSCAO__ */
