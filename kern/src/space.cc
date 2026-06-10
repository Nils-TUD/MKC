/*
 * Object Capability Space
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

#include "ec.h"
#include "space.h"

Space::Space() : table(), head(nullptr)
{
    table = static_cast<Capability *>(Kalloc::allocator.alloc_page(1, Kalloc::FILL_0));
}

bool Space::insert_root(Kobject *kobj)
{
    if (list_lookup(kobj->selector))
        return false;
    if (!table_insert(kobj, kobj->selector))
        return false;
    list_insert(kobj);
    return true;
}

bool Space::table_insert(Kobject *kobj, mword sel)
{
    if (sel >= MAX_CAPS)
        return false;
    table[sel] = Capability(kobj);

    printf("EC:%p table_insert(sel=%#lx)\n", Ec::current, sel);
    return true;
}

Mdb *Space::list_lookup(mword sel)
{
    Mdb *node = head;
    while (node) {
        if (node->selector == sel)
            return node;
        node = node->next;
    }
    return nullptr;
}

void Space::list_insert(Mdb *node)
{
    node->prev = nullptr;
    node->next = head;
    if (head)
        head->prev = node;
    head = node;
}
