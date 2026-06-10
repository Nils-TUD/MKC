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
#include "kalloc.h"
#include "kobject.h"
#include "memory.h"
#include "pd.h"
#include "regs.h"
#include "stdio.h"
#include "tss.h"
#include "utcb.h"

class Ec : public Kobject
{
    private:
        enum State
        {
            READY,
            WAITING,
            BLOCKED,
        };

        void     (*cont)();
        Exc_regs regs;
        Ec      *prev, *next;

        Utcb *utcb;
        mword utcb_vaddr;

        State state;
        Ec   *caller;

        Pd *pd;

        static void handle_exc(Exc_regs *) asm("exc_handler");

        [[noreturn]]
        static void handle_tss() asm("tss_handler");

        static bool handle_exc_ts(Exc_regs *);

        [[gnu::always_inline]]
        inline Sys_regs *sys_regs()
        {
            return &regs;
        }

        [[gnu::always_inline]]
        inline Exc_regs *exc_regs()
        {
            return &regs;
        }

        void alloc_utcb(mword addr);
        void enqueue();

        [[noreturn]]
        static void schedule();

    public:
        static Ec *current;

        Ec(Pd *own, void (*)(), mword = 0);
        Ec(Pd *own, mword sel, Pd *pd, mword, mword, mword);

        [[gnu::always_inline, noreturn]]
        inline void make_current()
        {
            current      = this;

            Tss::run.sp0 = reinterpret_cast<mword>(exc_regs() + 1);

            asm volatile("mov %0, %%rsp;"
                         "jmp *%1"
                         :
                         : "g"(KSTCK_ADDR + PAGE_SIZE), "rm"(cont)
                         : "memory");
            UNREACHED;
        }

        [[gnu::hot, noreturn]]
        static void ret_user_sysexit();

        [[noreturn]]
        static void ret_user_iret() asm("ret_user_iret");

        [[noreturn]]
        static void root_invoke();

        [[gnu::hot, noreturn]]
        static void syscall_handler(uint8) asm("syscall_handler");

        static void sys_dump();

        static void sys_create_ec();

        static void sys_create_pt();

        static void sys_create_pd();

        static void sys_revoke();

        [[noreturn]]
        static void sys_yield();

        [[noreturn]]
        static void sys_call();

        [[noreturn]]
        static void sys_reply();

        [[noreturn]]
        static void recv_user();

        template <bool C> static void delegate();

        [[gnu::always_inline]]
        static inline void *operator new(size_t)
        {
            return Kalloc::allocator.alloc(sizeof(Ec));
        }

        [[gnu::always_inline]]
        static inline void operator delete(void *)
        { /* nop */
        }
};
