/*
 * User Thread Control Block (UTCB)
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
#include "types.h"

class Utcb
{
    private:
        mword rcv_typed;
        mword snd_typed;
        mword untyped;
        mword mr[];

    public:
        [[gnu::always_inline]]
        inline void save(Utcb *dst)
        {
            dst->untyped = untyped;
            for (mword i = 0; i < untyped; i++)
                dst->mr[i] = mr[i];
        }

        [[gnu::always_inline]]
        inline mword recv_typed() const
        {
            return rcv_typed;
        }

        [[gnu::always_inline]]
        inline mword send_typed() const
        {
            return snd_typed;
        }

        [[gnu::always_inline]]
        static inline void *operator new(size_t)
        {
            return Kalloc::allocator.alloc_page(1, Kalloc::FILL_0);
        }

        [[gnu::always_inline]]
        static inline void operator delete(void *)
        { /* nop */
        }
};
