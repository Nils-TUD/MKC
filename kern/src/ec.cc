/*
 * Execution Context
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

#include "bits.h"
#include "ec.h"
#include "assert.h"
#include "cpu.h"
#include "ptab.h"

Ec * Ec::current = 0;

// solely used for root_invoke()
Ec::Ec (void (*f)(), mword mbi) : cont (f)
{
    regs.rdi = mbi;             /* SysV ABI: first arg in rdi */
    regs.cs  = SEL_USER_CODE;
    regs.ds  = SEL_USER_DATA;
    regs.es  = SEL_USER_DATA;
    regs.ss  = SEL_USER_DATA;
    regs.rfl = 0x200;           // IF = 1
}

// only used by syscall create thread (EC+SC)
Ec::Ec (mword rip, mword rsp)
{
    cont = ret_user_iret;
    regs.cs  = SEL_USER_CODE;
    regs.ds  = SEL_USER_DATA;
    regs.es  = SEL_USER_DATA;
    regs.ss  = SEL_USER_DATA;
    regs.rfl = 0x200;           // IF = 1
    regs.rip = rip;
    regs.rsp = rsp;
}

void Ec::ret_user_sysexit()
{
    // Restore all 16 GPRs from Sys_regs, then:
    //   R11 (popped from Sys_regs.r11) holds the user RSP saved by LOAD_KSP in entry.S
    //   Move it to RSP, set R11 = desired RFLAGS (IF=1), then sysretq.
    // On sysretq: RIP <- RCX, RFLAGS <- R11, CS/SS loaded from STAR.
    asm volatile (
        "lea %0, %%rsp;"
        "pop %%r15; pop %%r14; pop %%r13; pop %%r12;"
        "pop %%r11; pop %%r10; pop %%r9;  pop %%r8;"
        "pop %%rdi; pop %%rsi; pop %%rbp;"
        "add $8, %%rsp;"        /* skip cr2 slot */
        "pop %%rbx; pop %%rdx; pop %%rcx; pop %%rax;"
        "mov %%r11, %%rsp;"     /* user RSP (saved in r11 by entry LOAD_KSP) */
        "mov $0x200, %%r11;"    /* RFLAGS for sysretq: IF=1 */
        "sysretq"
        : : "m" (current->regs) : "memory");

    UNREACHED;
}

void Ec::ret_user_iret()
{
    // Restore all 16 GPRs, skip the 6 saved segment/error/vector fields, then iretq.
    asm volatile (
        "lea %0, %%rsp;"
        "pop %%r15; pop %%r14; pop %%r13; pop %%r12;"
        "pop %%r11; pop %%r10; pop %%r9;  pop %%r8;"
        "pop %%rdi; pop %%rsi; pop %%rbp;"
        "add $8, %%rsp;"        /* skip cr2 slot */
        "pop %%rbx; pop %%rdx; pop %%rcx; pop %%rax;"
        "add $0x30, %%rsp;"     /* skip gs,fs,es,ds,err,vec (6*8 bytes) */
        "iretq"
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

    // SYSCALL with RFLAGS.NT=1 and IRET faulted
    r->rfl &= ~0x4000ul; // nested task flag

    return true;
}

void Ec::handle_exc (Exc_regs *r)
{
    if (r->vec == Cpu::EXC_TS && handle_exc_ts (r))
        return;

    if (r->vec == Cpu::EXC_GP)
        panic ("%s GP (RIP=%#lx CR2=%#lx)\n",
               r->rip < LINK_ADDR ? "User" : "Kernel", r->rip, r->cr2);
    if (r->vec == Cpu::EXC_PF)
        panic ("%s PF (RIP=%#lx CR2=%#lx)\n",
               r->rip < LINK_ADDR ? "User" : "Kernel", r->rip, r->cr2);

    panic ("%s EXC %#lx (RIP=%#lx CR2=%#lx)\n",
           r->rip < LINK_ADDR ? "User" : "Kernel", r->vec, r->rip, r->cr2);

    UNREACHED;
}
