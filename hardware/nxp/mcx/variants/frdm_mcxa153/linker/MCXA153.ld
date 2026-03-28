/*
 * Linker script for FRDM-MCXA153
 * CPU: MCXA153VLH (Cortex-M33 nodsp)
 *
 * Memory map:
 *   PROGRAM_FLASH : 0x00000000  128KB
 *   SRAM          : 0x20000000   24KB
 *   SRAMX0        : 0x04000000    8KB
 *   SRAMX1        : 0x04002000    4KB
 */

MEMORY
{
  PROGRAM_FLASH (rx)  : ORIGIN = 0x0,          LENGTH = 0x20000 /* 128K */
  SRAM          (rwx) : ORIGIN = 0x20000000,    LENGTH = 0x6000  /*  24K */
  SRAMX0        (rwx) : ORIGIN = 0x4000000,     LENGTH = 0x2000  /*   8K */
  SRAMX1        (rwx) : ORIGIN = 0x4002000,     LENGTH = 0x1000  /*   4K */
}

ENTRY(ResetISR)

SECTIONS
{
    /* Vector table + code + read-only data → Flash */
    .text : ALIGN(4)
    {
        FILL(0xFF)
        __vectors_start__ = ABSOLUTE(.);
        KEEP(*(.isr_vector))
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        /* C++ constructors */
        __preinit_array_start = .;
        KEEP(*(.preinit_array))
        __preinit_array_end = .;
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
        __fini_array_start = .;
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        __fini_array_end = .;
        . = ALIGN(4);
    } > PROGRAM_FLASH

    /* Initialized data: stored in Flash, copied to SRAM at startup */
    .data : ALIGN(4)
    {
        _data = .;
        *(vtable)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT > PROGRAM_FLASH

    _data_flash = LOADADDR(.data);

    /* Zero-initialized data */
    .bss : ALIGN(4)
    {
        _bss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM

    /* Heap */
    .heap : ALIGN(8)
    {
        __heap_start__ = .;
        . += 0x800; /* 2KB default heap */
        __heap_end__ = .;
    } > SRAM

    /* Stack (grows downward from top of SRAM) */
    .stack : ALIGN(8)
    {
        . = ALIGN(8);
        __stack_start__ = .;
        . += 0x800; /* 2KB default stack */
        . = ALIGN(8);
        __stack_top__ = .;
    } > SRAM

    /* SRAMX0/SRAMX1: available for user allocation */
    .sramx0 (NOLOAD) : ALIGN(4) { } > SRAMX0
    .sramx1 (NOLOAD) : ALIGN(4) { } > SRAMX1

    /* Discard C++ exception tables (not used with -fno-exceptions) */
    /DISCARD/ :
    {
        *(.ARM.extab*)
        *(.ARM.exidx*)
    }
}
