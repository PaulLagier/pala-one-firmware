ENTRY(app_main)
SECTIONS {
    .header   0 : { KEEP(*(.header)) }
    .literal    : { *(.literal) *(.literal.*) }
    .text       : { *(.text*) }
    .rodata     : { *(.rodata*) }
    .data       : { *(.data*) }
}
