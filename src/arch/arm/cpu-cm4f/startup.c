//*****************************************************************************
//
// startup_gcc.c - Startup code for use with GNU tools. dscao999@hotmail.com
//
//
//*****************************************************************************

#include <stdint.h>
#include <hw_nvic.h>
#include <board_def.h>
#include "hw_memmap.h"
#include "kernel.h"

static void __attribute__((noreturn, naked)) Reset_Handler(void);
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void __attribute__((naked, weak)) SVC_Handler(void);
void __attribute__((weak)) DebugMon_Handler(void);
void __attribute__((naked, weak)) PendSVC_Handler(void);
void SysTick_Handler(void);

//*****************************************************************************
//
// The entry point for the application.
//
//*****************************************************************************
extern void kernel_start(void);

extern void (* __SRAM_END[])(void);
//*****************************************************************************
//
// The vector table.  Note that the proper constructs must be placed on this to
// ensure that it ends up at physical address 0x0000.0000.
//
//*****************************************************************************
__attribute__ ((section(".isr_vector"), used))
static void (* const g_pfnVectors[])(void) =
{
    (void (*)(void))((uint32_t)(__SRAM_END)), /* Top of Stack at top of RAM */
                                            // The initial stack pointer
    Reset_Handler,                               // The reset handler
    NMI_Handler,                                  // The NMI handler
    HardFault_Handler,                               // The hard fault handler
    MemManage_Handler,                      // The MPU fault handler
    BusFault_Handler,                      // The bus fault handler
    UsageFault_Handler,                      // The usage fault handler
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    SVC_Handler,                      // SVCall handler
    DebugMon_Handler,                      // Debug monitor handler
    0,                                      // Reserved
    PendSVC_Handler,                      // The PendSV handler
    SysTick_Handler,                      // The SysTick handler
};

//*****************************************************************************
//
// The following are constructs created by the linker, indicating where the
// the "data" and "bss" segments reside in memory.  The initializers for the
// for the "data" segment resides immediately following the "text" segment.
//
//*****************************************************************************
extern uint32_t __text_end[];
extern uint32_t __data_start[];
extern uint32_t __data_end[];

extern uint32_t __bss_start[];
extern uint32_t __bss_end[];

//*****************************************************************************
//
// startup_gcc.c - Startup code for use with GNU tools.
//
// Copyright (c) 2012-2020 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.2.0.295 of the EK-TM4C123GXL Firmware Package.
//
//*****************************************************************************

//*****************************************************************************
//
// This is the code that gets called when the processor first starts execution
// following a reset event.  Only the absolutely necessary set is performed,
// after which the application supplied entry() routine is called.  Any fancy
// actions (such as making decisions based on the reset cause register, and
// resetting the bits in that register) are left solely in the hands of the
// application.
//
//*****************************************************************************

static void __attribute__((noreturn, naked)) Reset_Handler(void)
{
	uint32_t *pui32Src, *pui32Dst, scbv;
	volatile uint32_t *scbreg;

	pui32Src = __text_end;
	pui32Dst = __data_start;
	while (pui32Dst < __data_end)
		*pui32Dst++ = *pui32Src++;
	pui32Dst = __bss_start;
	while (pui32Dst < __bss_end)
		*pui32Dst++ = 0;

	/*
	 * Set Configuration and Control Register, ARM V7-M Page 604
	 * Stack alignment 8, trap divde by zero and unaligned access
	 */
	scbreg = (volatile uint32_t *)NVIC_CFG_CTRL;
	scbv = *scbreg;
	scbv |= (NVIC_CFG_CTRL_STKALIGN|NVIC_CFG_CTRL_DIV0|NVIC_CFG_CTRL_UNALIGNED);
	*scbreg = scbv;
	/*
	 * Set System Handler Control and State Register, ARM V7-M Page 607
	 * Enable Usage, Bus, MemManage fault
	 */
	scbreg = (volatile uint32_t *)NVIC_SYS_HND_CTRL;
	scbv = *scbreg;
	scbv |= (NVIC_SYS_HND_CTRL_USAGE|NVIC_SYS_HND_CTRL_BUS|NVIC_SYS_HND_CTRL_MEM);
	*scbreg = scbv;

	/* Set System Exception Priority, ARM V7-M Page 606
	 * Set SVC exception to the lowest priority
	 */
	scbreg = (volatile uint32_t *)NVIC_SYS_PRI2;
	scbv = *scbreg;
	scbv &= ~(0xffu << 24);
	scbv |= (PRIORITY7 << 24);
	*scbreg = scbv;
	/*
	 * Set SysTick priority to 1 and PendSVC to the lowest
	 */
	scbreg = (volatile uint32_t *)NVIC_SYS_PRI3;
	scbv = *scbreg;
	scbv &= ~(0xffffu << 16);
	scbv |= (PRIORITY1 << 24) | (PRIORITY7 << 16);
	*scbreg = scbv;

	scbreg = (volatile uint32_t *)NVIC_ST_RELOAD;
	*scbreg = CPU_HZ / TICK_HZ - 1;
	scbreg = (volatile uint32_t *)NVIC_ST_CURRENT;
	*scbreg = 0;

	kernel_start();
	while (1)
		;
}

void NMI_Handler(void)
{
	while(1)
		;
}

void HardFault_Handler(void)
{
	while(1)
		;
}

void MemManage_Handler(void)
{
	while (1)
		;
}

void BusFault_Handler(void)
{
	while (1)
		;
}

void UsageFault_Handler(void)
{
	while (1)
		;
}

void __attribute__((weak)) DebugMon_Handler(void)
{
	while (1)
		;
}

void __attribute__((weak)) SVC_Handler(void)
{
	while (1)
		;
}

void __attribute__((weak)) PendSVC_Handler(void)
{
	while (1)
		;
}
