/*
 * Mapping Database
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

#include "kalloc.h"

class Kobject;
class Space;
class Pd;

class Mdb
{
    protected:
        friend class Space;
        friend class Pd;
        Mdb *prev;
        Mdb *next;
        Mdb *first_del;
        Mdb *next_del;

    public:
        Space *const space;
        mword        selector;
        Kobject     *kobj;

        explicit Mdb(Space *s, mword sel, Kobject *k)
            : prev(this), next(this), first_del(), next_del(), space(s), selector(sel), kobj(k)
        {
        }

        [[gnu::always_inline]]
        inline void add_del(Mdb *node)
        {
            node->next_del = first_del;
            first_del      = node;
        }

        [[gnu::always_inline]]
        static inline void *operator new(size_t)
        {
            return Kalloc::allocator.alloc(sizeof(Mdb));
        }

        [[gnu::always_inline]]
        static inline void operator delete(void *)
        { /* nop */
        }
};
