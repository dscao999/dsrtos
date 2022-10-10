#include <stdint.h>
#include <stdbool.h>
#include "hw_memmap.h"
#include "hw_udma.h"
#include "hw_uart.h"
#include "gpio.h"
#include "rom.h"
#include "rom_map.h"
#include "uart.h"
#include "sysctl.h"
#include "udma.h"
#include "board_def.h"
#include "board_init.h"
#include "kernel.h"
#include "armv7m_utils.h"
#include "misc_utils.h"

void led_light(int led, int onoff)
{
	uint8_t rgb, onv, pinnum;

	rgb = (led % 3) + 1;
	pinnum = GPIO_PIN_1;
	switch(rgb) {
		case 1:
			pinnum = GPIO_PIN_1;
			break;
		case 2:
			pinnum = GPIO_PIN_2;
			break;
		case 3:
			pinnum = GPIO_PIN_3;
			break;
	}
	onv = onoff != 0? 0xff : 0;
	MAP_GPIOPinWrite(GPIO_PORTF_BASE, pinnum, onv);
}

#define MAX_LOOPS 200000

void death_flash()
{
	int led;
	volatile uint32_t loops;

	led = 0;
	led_off_all();
	do {
		led_light(led, 1);
		for (loops = 0; loops < MAX_LOOPS; loops++)
			;
		led_light(led, 0);
		for (loops = 0; loops < MAX_LOOPS; loops++)
			;
		led += 1;
	} while (1);
}

int console_getstr(char *buf, int buflen)
{
	uint8_t head, tail, val;
	int len;

	len = 0;
	if (cirbuf_empty(uart0_buf))
		return len;
	head = uart0_buf->head;
	tail = uart0_buf->tail;
	while (tail != head && len < buflen) {
		val = uart0_buf->items[tail++];
		*buf++ = val;
		len += 1;
		if (val == '\r')
			break;
	}
	uart0_buf->tail = tail;

	return len;
}

static char uart0_dmabuf[256];
int console_putstr(const char *buf, int len)
{
       	int dmach, dma_ctrl, lockv, i, olen;
      	volatile uint32_t *dma_enable_reg, *uart_dmactl_reg;

	if (len <= 0 || len > sizeof(uart0_dmabuf) - 1)
		return 0;

	lockv = 1; /* lockv should be the task id and other book keeping info */
	spin_lock(&uart0_lock, lockv);
	olen = 0;
	for (i = 0; i < len; i++) {
		uart0_dmabuf[olen++] = buf[i];
		if (buf[i] == '\n')
			uart0_dmabuf[olen++] = '\r';
	}
	dmach = UDMA_CHANNEL_UART0TX;
	udma_table[dmach].src = (uint32_t)uart0_dmabuf + olen - 1;
	dma_ctrl = UDMA_CHCTL_DSTINC_NONE|UDMA_CHCTL_DSTSIZE_8|
			UDMA_CHCTL_SRCINC_8|UDMA_CHCTL_SRCSIZE_8|UDMA_CHCTL_ARBSIZE_4|
			((olen - 1) << 4)|UDMA_CHCTL_XFERMODE_BASIC;
	udma_table[dmach].ctrl = dma_ctrl;
	asm volatile ("dsb");
        uart_dmactl_reg = (volatile uint32_t *)(UART0_BASE + UART_O_DMACTL);
        dma_ctrl = *uart_dmactl_reg;
        dma_ctrl |= UART_DMACTL_TXDMAE;
        *uart_dmactl_reg = dma_ctrl;
	dma_enable_reg = (volatile uint32_t *)UDMA_ENASET;
	*dma_enable_reg = (1 << dmach);
	return len;
}

void uDMAError_Handler(void)
{
	led_light(1, 1);
	while (1)
		;
}

void UART0_Handler(void)
{
	uint32_t intr_val, datval, flag, dma_mask, dmactl;
	volatile uint32_t *uart_mis_reg, *uart_icr_reg, *uart_dr, *uart_fr;
	volatile uint32_t *udma_chis_reg, *uart_dmactl_reg;

	uart_mis_reg = (volatile uint32_t *)(UART0_BASE + UART_O_MIS);
	intr_val = *uart_mis_reg;
	if (intr_val) {
		uart_icr_reg = (volatile uint32_t *)(UART0_BASE + UART_O_ICR);
		*uart_icr_reg = intr_val;
	}
	if (intr_val & (UART_IM_RTIM|UART_IM_RXIM)) {
		uart_dr = (volatile uint32_t *)(UART0_BASE + UART_O_DR);
		uart_fr = (volatile uint32_t *)(UART0_BASE + UART_O_FR);
		flag = *uart_fr;
		while ((flag & UART_FR_RXFE) == 0) {
			datval = *uart_dr;
			cirbuf_put(uart0_buf, datval);
			flag = *uart_fr;
			if ((flag & (UART_FR_TXFF)) == 0)
				*uart_dr = datval;
		}
	}
	dma_mask = 1 << UDMA_CHANNEL_UART0TX;
	udma_chis_reg = (volatile uint32_t *)UDMA_CHIS;
	intr_val = *udma_chis_reg;
	if (intr_val & dma_mask) {
		*udma_chis_reg = dma_mask;
		uart_dmactl_reg = (volatile uint32_t *)(UART0_BASE + UART_O_DMACTL);
	        dmactl = *uart_dmactl_reg;
		dmactl &= ~UART_DMACTL_TXDMAE;
		*uart_dmactl_reg = dmactl;
		uart0_lock = 0;
	}
}
