/*
 * Execution Context
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

#include "compiler.h"
#include "regs.h"
#include "tss.h"
#include "kalloc.h"
#include "memory.h"
#include "stdio.h"

class Ec
{
    private:
        void        (*cont)();
        Exc_regs    regs;

        [[gnu::regparm(1)]]
        static void handle_exc (Exc_regs *) asm ("exc_handler");

        [[gnu::noreturn]]
        static void handle_tss() asm ("tss_handler");

        static bool handle_exc_ts (Exc_regs *);

        [[gnu::always_inline]]
        inline Sys_regs *sys_regs() { return &regs; }

        [[gnu::always_inline]]
        inline Exc_regs *exc_regs() { return &regs; }

    public:
        static Ec * current;

        Ec (void (*)(), mword = 0);
        Ec (mword, mword);

        [[gnu::always_inline, noreturn]]
        inline void make_current()
        {
            current = this;

            Tss::run.sp0 = reinterpret_cast<mword>(exc_regs() + 1);

            asm volatile ("mov %0, %%rsp;"
                          "jmp *%1"
                          : : "g" (KSTCK_ADDR + PAGE_SIZE), "rm" (cont) : "memory"); UNREACHED;
        }

        [[gnu::hot, noreturn]]
        static void ret_user_sysexit();

        [[gnu::noreturn]]
        static void ret_user_iret() asm ("ret_user_iret");

        [[noreturn]]
        static void root_invoke();

        [[gnu::hot, gnu::noreturn, gnu::regparm(1)]]
        static void syscall_handler (uint8) asm ("syscall_handler");

        [[gnu::always_inline]]
        static inline void *operator new (size_t) { return Kalloc::allocator.alloc(sizeof (Ec)); }

        [[gnu::always_inline]]
        static inline void operator delete (void *) { /* nop */ }
};
