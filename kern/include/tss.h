/*
 * Task State Segment (TSS)
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

#include "selectors.h"
#include "types.h"

/*
 * 64-bit TSS layout (Intel Vol. 3A §8.7)
 *
 * sp0/sp1/sp2 sit at non-8-byte-aligned offsets (0x4, 0xc, 0x14) because
 * the struct opens with a reserved 32-bit field.  We use [[gnu::packed]]
 * on the uint64 fields to suppress GCC alignment padding.
 */
class [[gnu::packed]] Tss
{
    public:
        uint32 : 32;   // 0x00 reserved
        uint64 sp0;    // 0x04 RSP for ring 0 (PACKED: misaligned)
        uint64 sp1;    // 0x0c RSP for ring 1
        uint64 sp2;    // 0x14 RSP for ring 2
        uint64 : 64;   // 0x1c reserved
        uint64 ist[7]; // 0x24 IST1-IST7 (ist[0]=IST1 used for #DF)
        uint64 : 64;   // 0x5c reserved
        uint16 trap;   // 0x64 debug trap flag
        uint16 iobm;   // 0x66 I/O bitmap base offset

        /*
         * 'asm("tss_run")' gives the static variable the symbol name "tss_run"
         * so that entry.S can load sp0 with: mov tss_run+4, %rsp
         */
        static Tss run asm("tss_run");

        static void build();

        [[gnu::always_inline]]
        static inline void load()
        {
            asm volatile("ltr %w0" : : "rm"(SEL_TSS_RUN));
        }
};

static_assert(sizeof(Tss) == 0x68, "TSS must be 104 bytes");
