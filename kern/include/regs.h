/*
 * Register File
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

#include "atomic.h"
#include "compiler.h"
#include "types.h"

/*
 * x86-64 register save area layout (must match SAVE_GPR order in entry.S).
 *
 * Push order in SAVE_GPR: rax first (highest address), r15 last (lowest address).
 * So in memory (ascending addresses):
 *   r15 @ +0x00, r14 @ +0x08, r13 @ +0x10, r12 @ +0x18,
 *   r11 @ +0x20, r10 @ +0x28, r9  @ +0x30, r8  @ +0x38,
 *   rdi @ +0x40, rsi @ +0x48, rbp @ +0x50, cr2 @ +0x58,
 *   rbx @ +0x60, rdx @ +0x68, rcx @ +0x70, rax @ +0x78
 * Total Sys_regs: 16 * 8 = 128 = 0x80 bytes
 */
class Sys_regs
{
    public:
        union {
            struct {
                mword   r15;
                mword   r14;
                mword   r13;
                mword   r12;
                mword   r11;
                mword   r10;
                mword   r9;
                mword   r8;
                mword   rdi;
                mword   rsi;
                mword   rbp;
                mword   cr2;
                mword   rbx;
                mword   rdx;
                mword   rcx;
                mword   rax;
            };
            mword gpr[16];
        };
};

/*
 * Full exception frame.  Offsets from start of Sys_regs:
 *   +0x80: gs   +0x88: fs   +0x90: es   +0x98: ds
 *   +0xa0: err  +0xa8: vec
 *   +0xb0: rip  +0xb8: cs   +0xc0: rfl  +0xc8: rsp  +0xd0: ss
 */
class Exc_regs : public Sys_regs
{
    public:
        union {
            struct {
                mword   gs;
                mword   fs;
                mword   es;
                mword   ds;
                mword   err;
                mword   vec;
                mword   rip;
                mword   cs;
                mword   rfl;
                mword   rsp;
                mword   ss;
            };
        };

    public:
        ALWAYS_INLINE
        inline bool user() const { return cs & 3; }
};
