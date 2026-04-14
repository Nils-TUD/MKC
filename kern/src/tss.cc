/*
 * Task State Segment (TSS)
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

#include "tss.h"
#include "memory.h"

[[gnu::aligned(8)]]
Tss Tss::run;

void Tss::build()
{
    run.sp0    = KSTCK_ADDR + PAGE_SIZE;
    run.ist[0] = KSTCK_ADDR + PAGE_SIZE; /* IST1: double-fault stack */
    run.iobm   = static_cast<uint16>(IOBMP_SADDR - reinterpret_cast<mword>(&run));
}
