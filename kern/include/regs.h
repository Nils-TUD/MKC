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
                        mword r15;
                        mword r14;
                        mword r13;
                        mword r12;
                        mword r11;
                        mword r10;
                        mword r9;
                        mword r8;
                        mword rdi;
                        mword rsi;
                        mword rbp;
                        mword cr2;
                        mword rbx;
                        mword rdx;
                        mword rcx;
                        mword rax;
                };
                mword gpr[16];
        };

        [[gnu::always_inline]]
        inline void set_ip(mword ip)
        {
            rcx = ip;
        }
};

/*
 * Full exception frame. Offsets from start of Sys_regs:
 *   +0x80: err  +0x88: vec
 *   +0x90: rip  +0x98: cs   +0xa0: rfl  +0xa8: rsp  +0xb0: ss
 */
class Exc_regs : public Sys_regs
{
    public:
        union {
                struct {
                        mword err;
                        mword vec;
                        mword rip;
                        mword cs;
                        mword rfl;
                        mword rsp;
                        mword ss;
                };
        };

    public:
        [[gnu::always_inline]]
        inline bool user() const
        {
            return cs & 3;
        }
};

static_assert(sizeof(Sys_regs) == 0x80, "Sys_regs must match SAVE_GPR layout");
static_assert(sizeof(Exc_regs) == 0xb8, "Exc_regs must match exception frame layout");
