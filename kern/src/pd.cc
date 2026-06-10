/*
 * Portal
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
 *
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

#include "initprio.h"
#include "ec.h"
#include "pd.h"

[[gnu::init_priority(PRIO_CONSOLE)]]
Pd Pd::root(&Pd::root);

Pd::Pd(Pd *own) : Kobject(PD, own) {}

Pd::Pd(Pd *own, mword sel) : Kobject(PD, own, sel) {}

void Pd::del_cap(Pd *snd, mword snd_sel, mword rcv_sel)
{
    printf("EC:%p del_cap(snd=%#lx, snd_sel=%#lx, rcv_sel=%#lx)\n",
           Ec::current,
           snd->selector,
           snd_sel,
           rcv_sel);

    // TODO find mdb node
    // TODO check if rcv_sel is free
    // TODO create new mdb node
    // TODO insert and update
    // TODO remember delegation
}
