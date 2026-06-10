/* plugins/plugin.ld — Linker script for position-independent plugin binaries */
ENTRY(_plugin_entry)

SECTIONS
{
    /* Plugin header at offset 0 */
    .plugin_header 0x0 : {
        KEEP(*(.plugin_header))
    }

    /* Code section — position independent */
    .text : {
        *(.text .text.*)
        *(.rodata .rodata.*)
    }

    /* VTable section */
    .plugin_vtable : {
        KEEP(*(.plugin_vtable))
        _plugin_vtable_start = .;
    }

    /* Data sections */
    .data : {
        *(.data .data.*)
    }

    .bss : {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        _bss_end = .;
    }

    /* Discard unwanted sections */
    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
