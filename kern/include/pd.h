/*
 * Protection Domain
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
#include "kobject.h"
#include "space.h"
#include "stdio.h"

class Pd : public Kobject, public Space
{
    public:
        static Pd root;

        Pd(Pd *own);
        Pd(Pd *own, mword selector);

        void del_cap(Pd *snd, mword snd_sel, mword rcv_sel);

        void revoke(mword sel);

        [[gnu::always_inline]]
        static inline void *operator new(size_t)
        {
            return Kalloc::allocator.alloc(sizeof(Pd));
        }

        [[gnu::always_inline]]
        static inline void operator delete(void *)
        { /* nop */
        }

    private:
        static void revoke_rec(Mdb *node);
};
