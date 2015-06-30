/*
 * Linker Script
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of the NOVA microhypervisor.
 *
 * NOVA is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NOVA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License version 2 for more details.
 */

#include "memory.h"

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH("i386:x86-64")
ENTRY(__start)

PHDRS
{
    init    PT_LOAD FLAGS(5);
    user    PT_LOAD FLAGS(5);
    kern_rx PT_LOAD FLAGS(5);
    kern_rw PT_LOAD FLAGS(6);
}

SECTIONS
{
    .init LOAD_ADDR :
    {
        *(.init)
        *(.initdata)
    } : init

    . = ALIGN(4K) + 4K; PROVIDE (STACK = .);

    PROVIDE (LOAD_E = .);

    /* USER_P is the physical (LMA) address where .user bytes are stored.
     * Place it right here (at LOAD_E, before the 4M-aligned kernel image)
     * so it does NOT collide with the .text LMA which uses ALIGN(4M).
     * OFFSET_U is computed from this explicit physical base. */
    PROVIDE (USER_P = LOAD_E);
    OFFSET_U = USER_ADDR - USER_P;

    /* OFFSET must be assigned while . is still a low (< 4M) address so that
     * ALIGN(4M) returns a sensible value.  Assigning after .user would make
     * ALIGN(4M) snap to 2^47 (the next 4M boundary above USER_ADDR). */
    OFFSET = LINK_ADDR - ALIGN (4M);

    .user USER_ADDR : AT (USER_P)
    {
        . = ALIGN(4K);
        KEEP (*(.user))
    } : user
    PROVIDE (LINK_P = LINK_ADDR - OFFSET);

    .text LINK_ADDR : AT (ADDR (.text) - OFFSET)
    {
        *(.text.hot)
        *(.text .text.* .gnu.linkonce.t.*)
    } : kern_rx

    .rodata : AT (ADDR (.rodata) - OFFSET)
    {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    } : kern_rx

    .fixup : AT (ADDR (.fixup) - OFFSET)
    {
        PROVIDE (FIXUP_S = .);
        KEEP (*(.fixup))
        PROVIDE (FIXUP_E = .);
    } : kern_rx

    .init_array : AT (ADDR (.init_array) - OFFSET)
    {
        PROVIDE (CTORS_L = .);
        KEEP (*(SORT_BY_INIT_PRIORITY (.init_array.65534) SORT_BY_INIT_PRIORITY (.ctors.00001)))
        PROVIDE (CTORS_C = .);
        KEEP (*(SORT_BY_INIT_PRIORITY (.init_array.65533) SORT_BY_INIT_PRIORITY (.ctors.00002)))
        PROVIDE (CTORS_G = .);
        KEEP (*(SORT_BY_INIT_PRIORITY (.init_array.*) SORT_BY_INIT_PRIORITY (.ctors.*)))
        KEEP (*(.init_array .ctors))
        PROVIDE (CTORS_E = .);
    } : kern_rw

    .data : AT (ADDR (.data) - OFFSET)
    {
        *(.data .data.* .gnu.linkonce.d.*)
    } : kern_rw

    .bss : AT (ADDR (.bss) - OFFSET)
    {
        *(.bss .bss.* .gnu.linkonce.b.*)

        . = ALIGN(4K);
        PROVIDE (PDBR   = . - OFFSET);
        PROVIDE (LVL4   = . - OFFSET); . += 4K;
        PROVIDE (LVL3L  = . - OFFSET); . += 4K;
        PROVIDE (LVL3H  = . - OFFSET); . += 4K;
        PROVIDE (LVL2L  = . - OFFSET); . += 4K;
        PROVIDE (LVL2H  = . - OFFSET); . += 4K;
        _mempool_f = .;

        . += 16M;
        . = ALIGN(4M);

        PROVIDE (LINK_E = . - OFFSET);

        PROVIDE (_mempool_e = .);

    } : kern_rw

    /DISCARD/ :
    {
        *(.note.GNU-stack)
        *(.comment*)
    }
}
