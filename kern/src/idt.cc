/*
 * Interrupt Descriptor Table (IDT)
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

#include "idt.h"
#include "extern.h"
#include "selectors.h"

[[gnu::aligned(8)]]
Idt Idt::idt[VEC_MAX];

void Idt::build()
{
    // tss_handler is Ec::handle_tss() and uses IST=1 so the CPU switches to
    // Tss::run.ist[0] on entry.
    extern char tss_handler;

    mword *ptr = handlers;

    for (unsigned vector = 0; vector < VEC_MAX; vector++, ptr++) {
        if (*ptr)
            idt[vector].set(SYS_INTR_GATE, *ptr & 3, SEL_KERN_CODE, *ptr & ~3ul);
        else {
            idt[vector].set(SYS_INTR_GATE,
                            0,
                            SEL_KERN_CODE,
                            reinterpret_cast<mword>(&tss_handler),
                            1);
        }
    }
}
