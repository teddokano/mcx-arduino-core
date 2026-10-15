/* MCXC444 Linker Script for Arduino Core */
/* Based on the MCXA153 script. One 32K SRAM, from 0x1FFFE000. The Kinetis
   flash configuration field goes at 0x400, from startup_mcxc444.cpp's
   .FlashConfig (FSEC 0xFE: unsecured; FOPT 0x3B: NMI pin off).
   The last 8K of the 256K flash is the EEPROM library's storage: two
   halves of four 1K sectors, used in turn. Uploads only erase the sectors
   the program occupies, so what is stored there survives them. */
MEMORY {
  PROGRAM_FLASH (rx) : ORIGIN = 0x0, LENGTH = 0x3E000 /* 248K */
  EEPROM_FLASH (r)   : ORIGIN = 0x3E000, LENGTH = 0x2000 /* 8K */
  SRAM (rwx)         : ORIGIN = 0x1FFFE000, LENGTH = 0x8000 /* 32K */
}

__base_PROGRAM_FLASH = 0x0;
__top_PROGRAM_FLASH  = 0x0 + 0x3E000;
__base_Flash = 0x0;
__top_Flash  = 0x0 + 0x3E000;
__base_EEPROM_FLASH = 0x3E000;
__top_EEPROM_FLASH  = 0x3E000 + 0x2000;
__base_SRAM  = 0x1FFFE000;
__top_SRAM   = 0x1FFFE000 + 0x8000;
__base_RAM   = 0x1FFFE000;
__top_RAM    = 0x1FFFE000 + 0x8000;

ENTRY(ResetISR)

SECTIONS
{
    .text : ALIGN(4)
    {
        FILL(0xff)
        __vectors_start__ = ABSOLUTE(.);
        KEEP(*(.isr_vector))
        /* Kinetis flash configuration field */
        . = 0x400;
        PROVIDE(__FLASH_CONFIG_START__ = .);
        KEEP(*(.FlashConfig))
        PROVIDE(__FLASH_CONFIG_END__ = .);
        ASSERT(!(__FLASH_CONFIG_START__ == __FLASH_CONFIG_END__), "no .FlashConfig section");
        . = ALIGN(4);
        __section_table_start = .;
        __data_section_table = .;
        LONG(LOADADDR(.data));
        LONG(    ADDR(.data));
        LONG(  SIZEOF(.data));
        __data_section_table_end = .;
        __bss_section_table = .;
        LONG(    ADDR(.bss));
        LONG(  SIZEOF(.bss));
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

    _HeapSize = 0x2000;
    .heap (NOLOAD) : ALIGN(4)
    {
        _pvHeapStart = .;
        . += _HeapSize;
        . = ALIGN(4);
        _pvHeapLimit = .;
    } > SRAM

    _StackSize = 0x800;
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
