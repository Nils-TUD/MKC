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

#include "compiler.h"
#include "types.h"
#include "extern.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "gdt.h"
#include "tss.h"
#include "gsi.h"
#include "idt.h"
#include "kalloc.h"
#include "assert.h"
#include "cpu.h"
#include "ptab.h"
#include "ec.h"
#include "io.h"
#include "msr.h"

char const *version = "NOVA Microhypervisor 0.3 (Cleetwood Cove)";

extern "C"
void init ()
{
    for (void (**func)() = &CTORS_E; func != &CTORS_G; (*--func)()) ;

    serial.init();

     // Now we're ready to talk to the world
    printf ("\f%s: %s %s [%s]\n\n", version, __DATE__, __TIME__, COMPILER_STRING);

    mword iobm = Kalloc::virt2phys (Kalloc::allocator.alloc_page(2,Kalloc::FILL_1));
    Ptab::insert_mapping (KSTCK_ADDR, Kalloc::virt2phys (Kalloc::allocator.alloc_page(1)), 0x23);
    Ptab::insert_mapping (IOBMP_SADDR, iobm, 0x23);
    Ptab::insert_mapping (IOBMP_SADDR + PAGE_SIZE, iobm + PAGE_SIZE, 0x23);

    // Ensure the PML4->PDPT->PD chain exists for the REMAP region.
    // insert_mapping() will create the PT too, but remap() uses 2MB superpages at
    // the PD level, so we just need the two upper levels.  We allocate a scratch
    // page into the REMAP region to trigger the walk; remap() will overwrite the
    // PD entry when first called.
    {
        mword* pml4 = static_cast<mword*>(Kalloc::phys2virt(Cpu::cr3()));
        unsigned i4 = (REMAP_SADDR >> 39) & 0x1ff;
        if ((pml4[i4] & 1) == 0) {
            mword *p = static_cast<mword*>(Kalloc::allocator.alloc_page(1, Kalloc::FILL_0));
            pml4[i4] = Kalloc::virt2phys(p) | 0x23;
        }
        mword* pdpt = static_cast<mword*>(Kalloc::phys2virt(pml4[i4] & ~PAGE_MASK));
        unsigned i3 = (REMAP_SADDR >> 30) & 0x1ff;
        if ((pdpt[i3] & 1) == 0) {
            mword *p = static_cast<mword*>(Kalloc::allocator.alloc_page(1, Kalloc::FILL_0));
            pdpt[i3] = Kalloc::virt2phys(p) | 0x23;
        }
    }

    for (void (**func)() = &CTORS_G; func != &CTORS_L; (*--func)()) ;

    Gdt::build();
    Tss::build();
    Idt::build();
    Gsi::setup();

    Gdt::load();
    Tss::load();
    Idt::load();

    // offset and mask all IRQs
    Io::out<uint8> (0x20, 0x11);
    Io::out<uint8> (0x21, VEC_GSI);
    Io::out<uint8> (0x21, 0x4);
    Io::out<uint8> (0x21, 0x1);
    Io::out<uint8> (0x21, 0xff);

    // setup syscall/sysret
    // STAR[47:32] = SEL_KERN_CODE  -> SYSCALL loads CS=0x08, SS=0x10
    // STAR[63:48] = SEL_KERN_DATA  -> SYSRET  loads SS=0x18|3=USER_DATA, CS=0x20|3=USER_CODE
    Msr::write<mword>(Msr::IA32_STAR,
                      (static_cast<mword>(SEL_KERN_DATA) << 48) |
                      (static_cast<mword>(SEL_KERN_CODE) << 32));
    Msr::write<mword>(Msr::IA32_LSTAR, reinterpret_cast<mword>(&entry_sysenter));
    Msr::write<mword>(Msr::IA32_FMASK, 0x200);   /* mask IF on SYSCALL entry */
}

extern "C" NORETURN
void bootstrap (mword addr)
{
    // Unmap the low identity mapping created by start.S.
    // The entire identity map lives under PML4[0] (covers 0–512 GB).
    mword* pml4 = static_cast<mword*>(Kalloc::phys2virt(Cpu::cr3()));
    pml4[0] = 0;
    Cpu::flush();

    Ec::current = new Ec (Ec::root_invoke, addr);
    Ec::current->make_current();

    UNREACHED;
}
