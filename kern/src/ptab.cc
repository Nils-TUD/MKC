/*
 * Page Table Management
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

#include "ptab.h"
#include "kalloc.h"
#include "memory.h"
#include "assert.h"

// add 4k mappings only; 4-level page table walk (PML4 -> PDPT -> PD -> PT)
void Ptab::insert_mapping (mword virt, mword phys, mword attr)
{
    mword* pml4 = static_cast<mword*>(Kalloc::phys2virt(Cpu::cr3()));

    // Level 4 -> Level 3 (PDPT)
    unsigned i4 = (virt >> 39) & 0x1ff;
    if ((pml4[i4] & 1) == 0) {
        mword *p = static_cast<mword*>(Kalloc::allocator.alloc_page(1, Kalloc::FILL_0));
        pml4[i4] = Kalloc::virt2phys(p) | 0x23 | (attr & 4);
    }
    mword* pdpt = static_cast<mword*>(Kalloc::phys2virt(pml4[i4] & ~PAGE_MASK));

    // Level 3 -> Level 2 (PD)
    unsigned i3 = (virt >> 30) & 0x1ff;
    if ((pdpt[i3] & 1) == 0) {
        mword *p = static_cast<mword*>(Kalloc::allocator.alloc_page(1, Kalloc::FILL_0));
        pdpt[i3] = Kalloc::virt2phys(p) | 0x23 | (attr & 4);
    }
    mword* pd = static_cast<mword*>(Kalloc::phys2virt(pdpt[i3] & ~PAGE_MASK));

    // Level 2 -> Level 1 (PT)
    unsigned i2 = (virt >> 21) & 0x1ff;
    if ((pd[i2] & 1) == 0) {
        mword *p = static_cast<mword*>(Kalloc::allocator.alloc_page(1, Kalloc::FILL_0));
        pd[i2] = Kalloc::virt2phys(p) | 0x23 | (attr & 4);
    }
    mword* pt = static_cast<mword*>(Kalloc::phys2virt(pd[i2] & ~PAGE_MASK));

    // Level 1: 4K page entry
    unsigned i1 = (virt >> PAGE_BITS) & 0x1ff;
    assert ((phys & PAGE_MASK) == 0);
    pt[i1] = (phys & ~PAGE_MASK) | (attr & PAGE_MASK);
}

void * Ptab::remap (mword addr)
{
    mword* pml4 = static_cast<mword*>(Kalloc::phys2virt(Cpu::cr3()));

    // Walk PML4 -> PDPT -> PD for REMAP_SADDR
    mword* pdpt = static_cast<mword*>(Kalloc::phys2virt(pml4[(REMAP_SADDR >> 39) & 0x1ff] & ~PAGE_MASK));
    mword* pd   = static_cast<mword*>(Kalloc::phys2virt(pdpt[(REMAP_SADDR >> 30) & 0x1ff] & ~PAGE_MASK));

    unsigned i2 = (REMAP_SADDR >> 21) & 0x1ff;

    // Flush the current 2MB mapping.
    pd[i2] = 0;
    Cpu::flush(REMAP_SADDR);

    // Insert 2MB superpage: PS bit (bit 7) = 1, 0xe3 = P+R/W+A+D+PS
    pd[i2] = (addr & ~0x1fffffUL) | 0xe3;

    return reinterpret_cast<void *>(REMAP_SADDR + (addr & 0x1fffff));
}
