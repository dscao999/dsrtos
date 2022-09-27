/*
 * Default linker script for GCC tm4c
 */

/*
 * Memory Spaces Definitions.
 *
 * Need modifying for a specific board.  256KiB flash, 32KiB ram
 *   FLASH.ORIGIN: starting address of flash
 *   FLASH.LENGTH: length of flash
 *   RAM.ORIGIN: starting address of RAM bank 0
 *   RAM.LENGTH: length of RAM bank 0
 *
 * The values below can be addressed in further linker scripts
 * using functions like 'ORIGIN(RAM)' or 'LENGTH(RAM)'.
 */

MEMORY
{
  FLASH (rx)         : ORIGIN = 0x00000000, LENGTH = 0x40000
  SRAM  (rw)         : ORIGIN = 0x20000000, LENGTH = 0x8000
}

__SRAM_END = ORIGIN(SRAM) + LENGTH(SRAM);

/*
 * For Cortex-M devices, the beginning of the startup code is stored in
 * the .isr_vector section, which goes to ROM
 */

SECTIONS
{
	.text :
	{
		KEEP(startup.o(.isr_vector))
		KEEP(board_init.o(.isr_vector))
		__init_start = .;
		KEEP(board_init.o(.init_0))
		__init_end = .;
		. = ALIGN(4);
		*(.text*)
		*(.rodata*)
		. = ALIGN(4);
		__text_end = .;
	} > FLASH

	.data : AT(ADDR(.text) + SIZEOF(.text))
	{
		__data_start = .;
		*(vtable)
		*(.data*)
		. = ALIGN(4);
		__data_end = .;
	} > SRAM

	.bss :
	{
		__bss_start = .;
		*(.bss*)
		__bss_end = .;
	} > SRAM

	.os_tables (NOLOAD) :
	{
		. = ORIGIN(SRAM) + LENGTH(SRAM) - 2048 - 8 * 2048;
		KEEP(kernel.o(.proc_stacks))
		KEEP(board_init.o(.udma_table))
		KEEP(kernel.o(.main_stack))
	} > SRAM
}
