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

#pragma once

#include "descriptor.h"
#include "vectors.h"

class Idt : public Descriptor
{
    private:
        /*
         * Interrupt gate layout:
         *   val[0] bits 31:16 = segment selector
         *   val[0] bits 15:0  = offset bits 15:0
         *   val[1] bits 31:16 = offset bits 31:16
         *   val[1] bit  15    = present
         *   val[1] bits 14:13 = DPL
         *   val[1] bits 11:8  = type (0xe = interrupt gate)
         *   val[1] bits 2:0   = IST index (0 = no IST, 1-7 = use ist[n-1])
         *   val[2] bits 31:0  = offset bits 63:32
         *   val[3]            = 0 (reserved)
         */
        uint32 val[sizeof(mword) / 2];

        [[gnu::always_inline]]
        inline void set(Type type, unsigned dpl, unsigned selector, mword offset, unsigned ist = 0)
        {
            val[0] = static_cast<uint32>(selector << 16 | (offset & 0xffff));
            val[1] = static_cast<uint32>((offset & 0xffff0000) | 1u << 15 | dpl << 13 | type | ist);
            val[2] = static_cast<uint32>(offset >> 32);
            val[3] = 0;
        }

    public:
        static Idt idt[VEC_MAX];

        [[gnu::section(".init")]]
        static void build();

        [[gnu::always_inline]]
        static inline void load()
        {
            Pseudo_descriptor pd(sizeof(idt) - 1, reinterpret_cast<mword>(idt));
            asm volatile("lidt %0" : : "m"(pd));
        }
};

static_assert(sizeof(Idt) == 16, "IDT entries must be 16 bytes in 64-bit mode");
