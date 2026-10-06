/* MCXN236 Linker Script for Arduino Core */
/* Based on MCUXpresso generated linker script; the memory map is the one
   the SDK 2.16.000 manifest gives for MCXN236 (PROGRAM_FLASH0 and
   PROGRAM_FLASH1 512K each, contiguous, SRAM 224K at 0x20000000, SRAMX
   96K at 0x4000000) */

/* The last 64K of the 1M flash is the EEPROM library's storage (two
   32K halves, used in turn), kept out of PROGRAM_FLASH so no sketch can
   grow into it. It sits in the second flash bank, so the program keeps
   running from the first while it is erased or written, as on MCXN947. */
MEMORY {
  PROGRAM_FLASH (rx) : ORIGIN = 0x0, LENGTH = 0xF0000 /* 1M - 64K */
  EEPROM_FLASH (r)   : ORIGIN = 0xF0000, LENGTH = 0x10000 /* 64K */
  SRAM (rwx)         : ORIGIN = 0x20000000, LENGTH = 0x38000 /* 224K */
  SRAMX0 (rwx)       : ORIGIN = 0x4000000, LENGTH = 0x18000 /* 96K (SRAMX) */
}

__base_PROGRAM_FLASH = 0x0;
__top_PROGRAM_FLASH  = 0x0 + 0xF0000;
__base_Flash = 0x0;
__top_Flash  = 0x0 + 0xF0000;
__base_EEPROM_FLASH = 0xF0000;
__top_EEPROM_FLASH  = 0xF0000 + 0x10000;
__base_SRAM  = 0x20000000;
__top_SRAM   = 0x20000000 + 0x38000;
__base_RAM   = 0x20000000;
__top_RAM    = 0x20000000 + 0x38000;
__base_SRAMX0 = 0x4000000;
__top_SRAMX0  = 0x4000000 + 0x18000;
__base_RAM2   = 0x4000000;
__top_RAM2    = 0x4000000 + 0x18000;

ENTRY(ResetISR)

SECTIONS
{
    .text : ALIGN(4)
    {
        FILL(0xff)
        __vectors_start__ = ABSOLUTE(.);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
        __section_table_start = .;
        __data_section_table = .;
        LONG(LOADADDR(.data));
        LONG(    ADDR(.data));
        LONG(  SIZEOF(.data));
        LONG(LOADADDR(.data_RAM2));
        LONG(    ADDR(.data_RAM2));
        LONG(  SIZEOF(.data_RAM2));
        __data_section_table_end = .;
        __bss_section_table = .;
        LONG(    ADDR(.bss));
        LONG(  SIZEOF(.bss));
        LONG(    ADDR(.bss_RAM2));
        LONG(  SIZEOF(.bss_RAM2));
        __bss_section_table_end = .;
        __section_table_end = .;

        *(.after_vectors*)
        *(.text*)
        *(.rodata .rodata.* .constdata .constdata.*)
        . = ALIGN(4);
        KEEP(*(.init))
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;
        . = ALIGN(4);
        __init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;
        KEEP(*(.fini));
        . = ALIGN(4);
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
        . = ALIGN(4);
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*crtend.o(.dtors))
        . = ALIGN(4);
    } > PROGRAM_FLASH

    .ARM.extab : ALIGN(4)
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > PROGRAM_FLASH

    .ARM.exidx : ALIGN(4)
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > PROGRAM_FLASH

    _etext = .;

    .data_RAM2 : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM2 = .);
        PROVIDE(__start_data_SRAMX0 = .);
        *(.ramfunc.$RAM2)
        *(.ramfunc.$SRAMX0)
        *(.data.$RAM2)
        *(.data.$SRAMX0)
        *(.data.$RAM2.*)
        *(.data.$SRAMX0.*)
        . = ALIGN(4);
        PROVIDE(__end_data_RAM2 = .);
        PROVIDE(__end_data_SRAMX0 = .);
    } > SRAMX0 AT>PROGRAM_FLASH

    .uninit_RESERVED (NOLOAD) : ALIGN(4)
    {
        _start_uninit_RESERVED = .;
        KEEP(*(.bss.$RESERVED*))
        . = ALIGN(4);
        _end_uninit_RESERVED = .;
    } > SRAM AT> SRAM

    .data : ALIGN(4)
    {
        FILL(0xff)
        _data = .;
        PROVIDE(__start_data_RAM = .);
        PROVIDE(__start_data_SRAM = .);
        *(vtable)
        *(.ramfunc*)
        KEEP(*(CodeQuickAccess))
        KEEP(*(DataQuickAccess))
        *(RamFunction)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
        PROVIDE(__end_data_RAM = .);
        PROVIDE(__end_data_SRAM = .);
    } > SRAM AT>PROGRAM_FLASH

    .bss_RAM2 (NOLOAD) : ALIGN(4)
    {
        PROVIDE(__start_bss_RAM2 = .);
        PROVIDE(__start_bss_SRAMX0 = .);
        *(.bss.$RAM2)
        *(.bss.$SRAMX0)
        *(.bss.$RAM2.*)
        *(.bss.$SRAMX0.*)
        . = ALIGN(. != 0 ? 4 : 1);
        PROVIDE(__end_bss_RAM2 = .);
        PROVIDE(__end_bss_SRAMX0 = .);
    } > SRAMX0 AT> SRAMX0

    .bss (NOLOAD) : ALIGN(4)
    {
        _bss = .;
        PROVIDE(__start_bss_RAM = .);
        PROVIDE(__start_bss_SRAM = .);
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        PROVIDE(__end_bss_RAM = .);
        PROVIDE(__end_bss_SRAM = .);
        PROVIDE(end = .);
    } > SRAM AT> SRAM

    .noinit_RAM2 (NOLOAD) : ALIGN(4)
    {
        PROVIDE(__start_noinit_RAM2 = .);
        PROVIDE(__start_noinit_SRAMX0 = .);
        *(.noinit.$RAM2)
        *(.noinit.$SRAMX0)
        *(.noinit.$RAM2.*)
        *(.noinit.$SRAMX0.*)
        . = ALIGN(4);
        PROVIDE(__end_noinit_RAM2 = .);
        PROVIDE(__end_noinit_SRAMX0 = .);
    } > SRAMX0 AT> SRAMX0

    .noinit (NOLOAD) : ALIGN(4)
    {
        _noinit = .;
        PROVIDE(__start_noinit_RAM = .);
        PROVIDE(__start_noinit_SRAM = .);
        *(.noinit*)
        . = ALIGN(4);
        _end_noinit = .;
        PROVIDE(__end_noinit_RAM = .);
        PROVIDE(__end_noinit_SRAM = .);
    } > SRAM AT> SRAM

    _HeapSize = 0x4000;
    .heap (NOLOAD) : ALIGN(4)
    {
        _pvHeapStart = .;
        . += _HeapSize;
        . = ALIGN(4);
        _pvHeapLimit = .;
    } > SRAM

    _StackSize = 0x1000;
    .heap2stackfill (NOLOAD) :
    {
        . += _StackSize;
    } > SRAM

    .stack ORIGIN(SRAM) + LENGTH(SRAM) - _StackSize - 0 (NOLOAD) : ALIGN(4)
    {
        _vStackBase = .;
        . = ALIGN(4);
        _vStackTop = . + _StackSize;
    } > SRAM

    _image_start = LOADADDR(.text);
    _image_end = LOADADDR(.data) + SIZEOF(.data);
    _image_size = _image_end - _image_start;
}
