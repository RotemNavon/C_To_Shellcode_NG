OUTPUT_FORMAT("binary");

SECTIONS
{
    . = 0x00;
    entry : { *(.text) }
    func  : { *(.func) }
    data  : { *(.data) }
    bss   : { *(.bss)  }
}