//*****************************************************************************
// board init functions and vectors
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "sysctl.h"
#include "rom.h"
#include "rom_map.h"
#include "hw_memmap.h"
#include "hw_nvic.h"
#include "hw_uart.h"
#include "hw_ints.h"
#include "hw_udma.h"
#include "gpio.h"
#include "uart.h"
#include "udma.h"
#include "pin_map.h"
#include "board_def.h"
#include "board_init.h"
#include "kernel.h"

static void IntDefaultHandler(void)
{
	while (1)
		;
}

static struct CirBuf256 uart0_buffer;
volatile uint32_t uart0_lock;
struct CirBuf256 * const uart0_buf = &uart0_buffer;


__attribute__((section(".udma_table")))
struct DMATable udma_table[64];
//*****************************************************************************
//
// The vector table.  Note that the proper constructs must be placed on this to
// ensure that it ends up at physical address 0x0000.0000.
//
//*****************************************************************************
__attribute__ ((section(".isr_vector"), used))
static void (* const g_pfnVectors[])(void) =
{
    IntDefaultHandler,                      // GPIO Port A
    IntDefaultHandler,                      // GPIO Port B
    IntDefaultHandler,                      // GPIO Port C
    IntDefaultHandler,                      // GPIO Port D
    IntDefaultHandler,                      // GPIO Port E
    UART0_Handler,				// UART0 Rx and Tx
    IntDefaultHandler,                      // UART1 Rx and Tx
    IntDefaultHandler,                      // SSI0 Rx and Tx
    IntDefaultHandler,                      // I2C0 Master and Slave
    IntDefaultHandler,                      // PWM Fault
    IntDefaultHandler,                      // PWM Generator 0
    IntDefaultHandler,                      // PWM Generator 1
    IntDefaultHandler,                      // PWM Generator 2
    IntDefaultHandler,                      // Quadrature Encoder 0
    IntDefaultHandler,                      // ADC Sequence 0
    IntDefaultHandler,                      // ADC Sequence 1
    IntDefaultHandler,                      // ADC Sequence 2
    IntDefaultHandler,                      // ADC Sequence 3
    IntDefaultHandler,                      // Watchdog timer
    IntDefaultHandler,                      // Timer 0 subtimer A
    IntDefaultHandler,                      // Timer 0 subtimer B
    IntDefaultHandler,                      // Timer 1 subtimer A
    IntDefaultHandler,                      // Timer 1 subtimer B
    IntDefaultHandler,                      // Timer 2 subtimer A
    IntDefaultHandler,                      // Timer 2 subtimer B
    IntDefaultHandler,                      // Analog Comparator 0
    IntDefaultHandler,                      // Analog Comparator 1
    IntDefaultHandler,                      // Analog Comparator 2
    IntDefaultHandler,                      // System Control (PLL, OSC, BO)
    IntDefaultHandler,                      // FLASH Control
    IntDefaultHandler,                      // GPIO Port F
    IntDefaultHandler,                      // GPIO Port G
    IntDefaultHandler,                      // GPIO Port H
    IntDefaultHandler,                      // UART2 Rx and Tx
    IntDefaultHandler,                      // SSI1 Rx and Tx
    IntDefaultHandler,                      // Timer 3 subtimer A
    IntDefaultHandler,                      // Timer 3 subtimer B
    IntDefaultHandler,                      // I2C1 Master and Slave
    IntDefaultHandler,                      // Quadrature Encoder 1
    IntDefaultHandler,                      // CAN0
    IntDefaultHandler,                      // CAN1
    0,                                      // Reserved
    0,                                      // Reserved
    IntDefaultHandler,                      // Hibernate
    IntDefaultHandler,                      // USB0
    IntDefaultHandler,                      // PWM Generator 3
    IntDefaultHandler,                      // uDMA Software Transfer
    uDMAError_Handler,                      // uDMA Error
    IntDefaultHandler,                      // ADC1 Sequence 0
    IntDefaultHandler,                      // ADC1 Sequence 1
    IntDefaultHandler,                      // ADC1 Sequence 2
    IntDefaultHandler,                      // ADC1 Sequence 3
    0,                                      // Reserved
    0,                                      // Reserved
    IntDefaultHandler,                      // GPIO Port J
    IntDefaultHandler,                      // GPIO Port K
    IntDefaultHandler,                      // GPIO Port L
    IntDefaultHandler,                      // SSI2 Rx and Tx
    IntDefaultHandler,                      // SSI3 Rx and Tx
    IntDefaultHandler,                      // UART3 Rx and Tx
    IntDefaultHandler,                      // UART4 Rx and Tx
    IntDefaultHandler,                      // UART5 Rx and Tx
    IntDefaultHandler,                      // UART6 Rx and Tx
    IntDefaultHandler,                      // UART7 Rx and Tx
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    IntDefaultHandler,                      // I2C2 Master and Slave
    IntDefaultHandler,                      // I2C3 Master and Slave
    IntDefaultHandler,                      // Timer 4 subtimer A
    IntDefaultHandler,                      // Timer 4 subtimer B
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    IntDefaultHandler,                      // Timer 5 subtimer A
    IntDefaultHandler,                      // Timer 5 subtimer B
    IntDefaultHandler,                      // Wide Timer 0 subtimer A
    IntDefaultHandler,                      // Wide Timer 0 subtimer B
    IntDefaultHandler,                      // Wide Timer 1 subtimer A
    IntDefaultHandler,                      // Wide Timer 1 subtimer B
    IntDefaultHandler,                      // Wide Timer 2 subtimer A
    IntDefaultHandler,                      // Wide Timer 2 subtimer B
    IntDefaultHandler,                      // Wide Timer 3 subtimer A
    IntDefaultHandler,                      // Wide Timer 3 subtimer B
    IntDefaultHandler,                      // Wide Timer 4 subtimer A
    IntDefaultHandler,                      // Wide Timer 4 subtimer B
    IntDefaultHandler,                      // Wide Timer 5 subtimer A
    IntDefaultHandler,                      // Wide Timer 5 subtimer B
    IntDefaultHandler,                      // FPU
    0,                                      // Reserved
    0,                                      // Reserved
    IntDefaultHandler,                      // I2C4 Master and Slave
    IntDefaultHandler,                      // I2C5 Master and Slave
    IntDefaultHandler,                      // GPIO Port M
    IntDefaultHandler,                      // GPIO Port N
    IntDefaultHandler,                      // Quadrature Encoder 2
    0,                                      // Reserved
    0,                                      // Reserved
    IntDefaultHandler,                      // GPIO Port P (Summary or P0)
    IntDefaultHandler,                      // GPIO Port P1
    IntDefaultHandler,                      // GPIO Port P2
    IntDefaultHandler,                      // GPIO Port P3
    IntDefaultHandler,                      // GPIO Port P4
    IntDefaultHandler,                      // GPIO Port P5
    IntDefaultHandler,                      // GPIO Port P6
    IntDefaultHandler,                      // GPIO Port P7
    IntDefaultHandler,                      // GPIO Port Q (Summary or Q0)
    IntDefaultHandler,                      // GPIO Port Q1
    IntDefaultHandler,                      // GPIO Port Q2
    IntDefaultHandler,                      // GPIO Port Q3
    IntDefaultHandler,                      // GPIO Port Q4
    IntDefaultHandler,                      // GPIO Port Q5
    IntDefaultHandler,                      // GPIO Port Q6
    IntDefaultHandler,                      // GPIO Port Q7
    IntDefaultHandler,                      // GPIO Port R
    IntDefaultHandler,                      // GPIO Port S
    IntDefaultHandler,                      // PWM 1 Generator 0
    IntDefaultHandler,                      // PWM 1 Generator 1
    IntDefaultHandler,                      // PWM 1 Generator 2
    IntDefaultHandler,                      // PWM 1 Generator 3
    IntDefaultHandler                       // PWM 1 Fault
};

static void clock_setup(void)
{
	MAP_SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|
                       SYSCTL_XTAL_16MHZ);
}

static void led_setup(void)
{
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	while (!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF))
		;
	MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,
			GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
}

static void dma_enable(void)
{
	int i, intnum, offset, bitpos;
	volatile uint32_t *udmareg;
	uint32_t val;

	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
	while (!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_UDMA))
		;
	for (i = 0; i < sizeof(udma_table) / sizeof(struct DMATable); i++) {
		udma_table[i].src = 0;
		udma_table[i].dst = 0;
		udma_table[i].ctrl = 0;
		udma_table[i].unused = 0;
	}
	MAP_uDMAEnable();
	MAP_uDMAControlBaseSet(udma_table);

	intnum = INT_UDMA - 16;
	offset = (intnum / 4) * 4;
	bitpos = intnum % 4;
	udmareg = (volatile uint32_t *)(NVIC_PRI0+offset);
	val = *udmareg;
	val &= ~(PRIORITY_MASK << (bitpos * 8));
	*udmareg = val | (PRIORITY5 << (bitpos * 8));
	offset = (intnum / 32) * 4;
	bitpos = intnum % 32;
	udmareg = (volatile uint32_t *)(NVIC_EN0+offset);
	*udmareg = (1 << bitpos);
}

static void uart0_enable(void)
{
	uint32_t datfmt, val, intr_mask, flag, dmactl;
	volatile uint32_t *udma_reg, *uart_fr, *uart_dr, *uart_reg;
	volatile uint32_t *uart_dmactl_reg;
	int offset, bitpos, intnum;

	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	while (!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_UART0))
		;
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	while (!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA))
		;
	MAP_GPIOPinConfigure(GPIO_PA0_U0RX|GPIO_PA1_U0TX);
	MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0|GPIO_PIN_1);

	*((volatile uint32_t *)(UART0_BASE + UART_O_CC)) = UART_CLOCK_PIOSC;
	datfmt = UART_CONFIG_WLEN_8|UART_CONFIG_STOP_ONE|UART_CONFIG_PAR_NONE;
	MAP_UARTConfigSetExpClk(UART0_BASE, 16000000, 115200, datfmt);

	uart_reg = (volatile uint32_t *)(UART0_BASE + UART_O_LCRH);
	val = *uart_reg;
	*uart_reg = val | UART_LCRH_FEN;
	uart_reg = (volatile uint32_t *)(UART0_BASE+UART_O_IFLS);
	val = *uart_reg;
	*uart_reg = (val & ~0x03fu) | UART_FIFO_RX4_8|UART_FIFO_TX4_8;

	intnum = INT_UART0 - 16;
	offset = (intnum / 4) * 4;
	bitpos = intnum % 4;
	uart_reg = (volatile uint32_t *)(NVIC_PRI0+offset);
	val = *uart_reg;
	val &= ~(PRIORITY_MASK << (bitpos * 8));
	*uart_reg = val | (PRIORITY6 << (bitpos * 8));
	offset = (intnum / 32) * 4;
	bitpos = intnum % 32;
	uart_reg = (volatile uint32_t *)(NVIC_EN0+offset);
	*uart_reg = (1 << bitpos);

	uart_reg = (volatile uint32_t *)(UART0_BASE + UART_O_CTL);
	val = *uart_reg;
	*uart_reg = val|UART_CTL_UARTEN|UART_CTL_TXE|UART_CTL_RXE|UART_CTL_EOT;

	uart_fr = (volatile uint32_t *)(UART0_BASE + UART_O_FR);
	uart_dr = (volatile uint32_t *)(UART0_BASE + UART_O_DR);
	flag = *uart_fr;
	while ((flag & UART_FR_RXFE) == 0 || (flag & UART_FR_BUSY) != 0) {
		if ((flag & UART_FR_RXFE) == 0)
			val = *uart_dr;
		flag = *uart_fr;
	}

	/* disable TX interrupts, to use DMA. Enable RX interrupts */
	uart_reg = (volatile uint32_t *)(UART0_BASE+UART_O_IM);
	intr_mask = *uart_reg;
	intr_mask |= UART_IM_OEIM|UART_IM_BEIM|UART_IM_PEIM|UART_IM_FEIM;
	intr_mask |= UART_IM_RTIM|UART_IM_RXIM;
	intr_mask &= ~(UART_IM_EOTIM|UART_IM_TXIM);
	*uart_reg = intr_mask;

	uart0_buf->head = 0;
	uart0_buf->tail = 0;
	uart0_lock = 0;

	/*
	 * Setup uDMA channel 9, UART0 TX
	 */
	val = (1 << UDMA_CHANNEL_UART0TX);
	udma_reg = (volatile uint32_t *)UDMA_PRIOCLR;
	*udma_reg = val;
	udma_reg = (volatile uint32_t *)UDMA_ALTCLR;
	*udma_reg = val;
	udma_reg = (volatile uint32_t *)UDMA_USEBURSTCLR;
	*udma_reg = val;
	udma_reg = (volatile uint32_t *)UDMA_REQMASKCLR;
	*udma_reg = val;
	udma_table[UDMA_CHANNEL_UART0TX].dst = UART0_BASE + UART_O_DR;

	uart_dmactl_reg = (volatile uint32_t *)(UART0_BASE + UART_O_DMACTL);
	dmactl = *uart_dmactl_reg;
	dmactl |= UART_DMACTL_DMAERR;
	*uart_dmactl_reg = dmactl;
}

__attribute__ ((section(".init_0"), used))
static void (* const init_funcs[])(void) =
{
	clock_setup,
	led_setup ,
	dma_enable,
	uart0_enable
};
