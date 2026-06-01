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

#pragma once

#include "kalloc.h"
#include "stdio.h"
#include "ec.h"

class Pt
{
    private:
        Pt        *next;
        static Pt *head;

        void enqueue();

    public:
        int   id;
        mword rip;
        Ec   *recv;

        Pt(int id, mword rip, Ec *recv);

        static Pt *find_by_id(int id);

        [[gnu::always_inline]]
        static inline void *operator new(size_t)
        {
            return Kalloc::allocator.alloc(sizeof(Pt));
        }

        [[gnu::always_inline]]
        static inline void operator delete(void *)
        { /* nop */
        }
};
