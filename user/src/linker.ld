OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH("i386:x86-64")
ENTRY(__start)

PHDRS
{
    data    PT_LOAD FLAGS(6);
    text    PT_LOAD FLAGS(5);
}

SECTIONS
{
    . =  4K;

    .data : ALIGN(4K)
    {
        BYTE(42)
        FILL(42)
        . = ALIGN(4K);
        STACK_TOP = .;

        *(.data);
        *(.bss);
    } : data

    .text : ALIGN(4K)
    {
        *(.text);
        *(.text.*);
    } : text

    /DISCARD/ :
    {
	    *(.note.gnu.property);
    }
}
