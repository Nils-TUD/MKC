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

#include "bits.h"
#include "ec.h"
#include "assert.h"
#include "cpu.h"
#include "ptab.h"

Ec * Ec::current = 0;

// solely used for root_invoke()
Ec::Ec (void (*f)(), mword mbi) : cont (f)
{
    regs.eax = mbi;
    regs.cs  = SEL_USER_CODE;
    regs.ds  = SEL_USER_DATA;
    regs.es  = SEL_USER_DATA;
    regs.ss  = SEL_USER_DATA;
    regs.efl = 0x200;           // IF = 1
}

// only used by syscall create thread (EC+SC)
Ec::Ec (mword eip, mword esp)
{
    cont = ret_user_iret;
    regs.cs  = SEL_USER_CODE;
    regs.ds  = SEL_USER_DATA;
    regs.es  = SEL_USER_DATA;
    regs.ss  = SEL_USER_DATA;
    regs.efl = 0x200;           // IF = 1
    regs.eip = eip;
    regs.esp = esp;
}

void Ec::ret_user_sysexit()
{
    asm volatile ("lea %0, %%esp;"
                  "popa;"
                  "sti;"
                  "sysexit"
                  : : "m" (current->regs) : "memory");

    UNREACHED;
}

void Ec::ret_user_iret()
{
    asm volatile ("lea %0, %%esp;"
                  "popa;"
                  "pop %%gs;"
                  "pop %%fs;"
                  "pop %%es;"
                  "pop %%ds;"
                  "add $8, %%esp;"
                  "iret"
                  : : "m" (current->regs) : "memory");

    UNREACHED;
}

void Ec::root_invoke()
{
    FAIL;
}

void Ec::handle_tss()
{
    panic ("Task gate invoked\n");
}

void Ec::syscall_handler (uint8 n)
{
    printf ("syscall %d\n", n);

    ret_user_sysexit();

    UNREACHED;
}

bool Ec::handle_exc_ts (Exc_regs *r)
{
    if (r->user())
        return false;

    // SYSENTER with EFLAGS.NT=1 and IRET faulted
    r->efl &= ~0x4000; // nested task eflag

    return true;
}

void Ec::handle_exc (Exc_regs *r)
{
    if (r->vec == Cpu::EXC_TS && handle_exc_ts (r))
        return;

    if (r->vec == Cpu::EXC_GP)
        panic ("%s GP (EIP=%#lx CR2=%#lx)\n", r->eip < LINK_ADDR ? "User" : "Kernel", r->eip, r->cr2);

    if (r->vec == Cpu::EXC_PF)
        panic ("%s PF (EIP=%#lx CR2=%#lx)\n", r->eip < LINK_ADDR ? "User" : "Kernel", r->eip, r->cr2);

    panic ("%s EXC %#lx (EIP=%#lx CR2=%#lx)\n", r->eip < LINK_ADDR ? "User" : "Kernel", r->vec, r->eip, r->cr2);

    UNREACHED;
}
