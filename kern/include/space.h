/*
 * Object Capability Space
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

#pragma once

#include "capability.h"
#include "kobject.h"
#include "mdb.h"
#include "memory.h"

const mword MAX_CAPS = PAGE_SIZE / sizeof(Capability);
const mword INV_CAP  = MAX_CAPS;

class Space
{
    private:
        Capability *table;
        Mdb        *head;

    public:
        Space();

        const Capability &lookup(mword sel)
        {
            static Capability invalid;
            return sel >= MAX_CAPS ? invalid : table[sel];
        }

        bool insert_root(Kobject *kobj);
        void update(Mdb *node) { table_insert(node->kobj, node->selector); }

        bool table_insert(Kobject *kobj, mword sel);
        Mdb *list_lookup(mword sel);
        void list_insert(Mdb *node);
};
