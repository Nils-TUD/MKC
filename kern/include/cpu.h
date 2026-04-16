/*
 * Central Processing Unit (CPU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "types.h"

class Cpu
{
    public:
        enum
        {
            EXC_NM = 7,
            EXC_TS = 10,
            EXC_GP = 13,
            EXC_PF = 14,
            EXC_AC = 17
        };

        [[gnu::always_inline]]
        static inline mword cr3()
        {
            mword cr3;
            asm volatile("mov  %%cr3, %0" : "=r"(cr3));
            return cr3;
        }

        [[gnu::always_inline]]
        static inline void flush()
        {
            mword cr3;
            asm volatile("mov %%cr3, %0; mov %0, %%cr3" : "=&r"(cr3));
        }

        [[gnu::always_inline]]
        static inline void flush(mword addr)
        {
            asm volatile("invlpg %0" : : "m"(*reinterpret_cast<mword *>(addr)));
        }

        [[gnu::always_inline]]
        static inline void preempt_disable()
        {
            asm volatile("cli" : : : "memory");
        }

        [[gnu::always_inline]]
        static inline void preempt_enable()
        {
            asm volatile("sti" : : : "memory");
        }

        [[gnu::always_inline, noreturn]]
        static inline void shutdown()
        {
            for (;;)
                asm volatile("cli; hlt");
        }

        [[gnu::always_inline]]
        static inline void cpuid(unsigned leaf, uint32 &eax, uint32 &ebx, uint32 &ecx, uint32 &edx)
        {
            asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(leaf));
        }

        [[gnu::always_inline]]
        static inline void
        cpuid(unsigned leaf, unsigned subleaf, uint32 &eax, uint32 &ebx, uint32 &ecx, uint32 &edx)
        {
            asm volatile("cpuid"
                         : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                         : "a"(leaf), "c"(subleaf));
        }
};
