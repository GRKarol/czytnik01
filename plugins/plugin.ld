/* plugins/plugin.ld — Linker script for ESP32-S3 plugin binaries */

SECTIONS
{
    /* Plugin header must be at the very start */
    .plugin_header : {
        KEEP(*(.plugin_header))
    }

    /* Xtensa literal pools (must precede their .text counterparts) */
    .literal : {
        *(.literal .literal.*)
    }

    /* Code */
    .text : {
        *(.text .text.*)
    }

    /* Read-only data (string constants etc.) */
    .rodata : {
        *(.rodata .rodata.*)
    }

    /* Plugin VTable — loader reads entryOffset pointing here */
    .plugin_vtable : {
        _plugin_vtable_start = .;
        KEEP(*(.plugin_vtable))
    }

    /* Initialized data */
    .data : {
        *(.data .data.*)
    }

    /* BSS (zero-initialized) */
    .bss : {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        _bss_end = .;
    }

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
        *(.xt.lit)
        *(.xt.prop)
        *(.gnu.*)
    }
}
