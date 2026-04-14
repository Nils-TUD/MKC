/*
 * Global Descriptor Table (GDT)
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

#include "gdt.h"
#include "memory.h"
#include "tss.h"

// 2 kernel + 2 user segment descriptors should be within same cache line
ALIGNED(8) Gdt Gdt::gdt[SEL_MAX >> 3];

void Gdt::build()
{
    // Code/data segments: L=1, D=0.
    gdt[SEL_KERN_CODE >> 3].set32 (CODE_XRA, PAGES, BIT_16, true,  0, 0, ~0ul);
    gdt[SEL_KERN_DATA >> 3].set32 (DATA_RWA, PAGES, BIT_16, true,  0, 0, ~0ul);

    // USER_DATA before USER_CODE — required for SYSCALL/SYSRET selector layout
    gdt[SEL_USER_DATA >> 3].set32 (DATA_RWA, PAGES, BIT_16, true,  3, 0, ~0ul);
    gdt[SEL_USER_CODE >> 3].set32 (CODE_XRA, PAGES, BIT_16, true,  3, 0, ~0ul);

    // The TSS descriptor occupies two GDT slots.
    gdt[SEL_TSS_RUN >> 3].set64 (SYS_TSS, BYTES, BIT_16, false, 0,
                                  reinterpret_cast<mword>(&Tss::run),
                                  IOBMP_EADDR - reinterpret_cast<mword>(&Tss::run));
}
