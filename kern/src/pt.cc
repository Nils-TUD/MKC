/*
 * Portal
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

#include "pt.h"
#include <memory.h>

Pt *Pt::head = nullptr;

Pt::Pt(int _id, mword _rip, Ec *_recv) : id(_id), rip(_rip), recv(_recv)
{
    enqueue();
}

void Pt::enqueue()
{
    next = head;
    head = this;
}

Pt *Pt::find_by_id(int id)
{
    Pt *p = head;
    while (p != nullptr) {
        if (p->id == id)
            return p;
        p = p->next;
    }
    return nullptr;
}
