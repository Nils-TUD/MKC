/*
 * Initialization Code
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 * Copyright (C) 2026 Nils Asmussen, Barkhausen Institut
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

#include "assert.h"
#include "compiler.h"
#include "cpu.h"
#include "ec.h"
#include "extern.h"
#include "gdt.h"
#include "gsi.h"
#include "idt.h"
#include "io.h"
#include "kalloc.h"
#include "memory.h"
#include "msr.h"
#include "ptab.h"
#include "stdio.h"
#include "string.h"
#include "tss.h"
#include "types.h"

char const *version = "NOVA Microhypervisor 0.3 (Cleetwood Cove)";

extern "C" void init()
{
    for (void (**func)() = &CTORS_E; func != &CTORS_G; (*--func)())
        ;

    serial.init();

    // Now we're ready to talk to the world
    printf("\f%s: %s %s [%s]\n\n", version, __DATE__, __TIME__, COMPILER_STRING);

    mword iobm = Kalloc::virt2phys(Kalloc::allocator.alloc_page(2, Kalloc::FILL_1));
    Ptab::insert_mapping(KSTCK_ADDR, Kalloc::virt2phys(Kalloc::allocator.alloc_page(1)), 0x23);
    Ptab::insert_mapping(IOBMP_SADDR, iobm, 0x23);
    Ptab::insert_mapping(IOBMP_SADDR + PAGE_SIZE, iobm + PAGE_SIZE, 0x23);

    for (void (**func)() = &CTORS_G; func != &CTORS_L; (*--func)())
        ;

    Gdt::build();
    Tss::build();
    Idt::build();
    Gsi::setup();

    Gdt::load();
    Tss::load();
    Idt::load();

    // offset and mask all IRQs
    Io::out<uint8>(0x20, 0x11);
    Io::out<uint8>(0x21, VEC_GSI);
    Io::out<uint8>(0x21, 0x4);
    Io::out<uint8>(0x21, 0x1);
    Io::out<uint8>(0x21, 0xff);

    // setup syscall/sysret
    // STAR[47:32] = SEL_KERN_CODE  -> SYSCALL loads CS=0x08, SS=0x10
    // STAR[63:48] = SEL_KERN_DATA  -> SYSRET  loads SS=0x18|3=USER_DATA, CS=0x20|3=USER_CODE
    Msr::write<mword>(Msr::IA32_STAR,
                      (static_cast<mword>(SEL_KERN_DATA) << 48) |
                          (static_cast<mword>(SEL_KERN_CODE) << 32));
    Msr::write<mword>(Msr::IA32_LSTAR, reinterpret_cast<mword>(&entry_sysenter));
    Msr::write<mword>(Msr::IA32_FMASK, 0x200); /* mask IF on SYSCALL entry */
}

extern "C" [[noreturn]]
void bootstrap(mword addr)
{
    // Unmap the low identity mapping created by start.S.
    // The entire identity map lives under PML4[0] (covers 0–512 GB).
    mword *pml4 = static_cast<mword *>(Kalloc::phys2virt(Cpu::cr3()));
    pml4[0]     = 0;
    Cpu::flush();

    Ec::current = new Ec(Ec::root_invoke, addr);
    Ec::current->make_current();

    UNREACHED;
}
