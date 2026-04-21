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

#include "ec.h"
#include "arch.h"
#include "cpu.h"

Ec *Ec::current = 0;

// solely used for root_invoke()
Ec::Ec(void (*f)(), mword mbi) : cont(f)
{
    regs.rdi = mbi; /* SysV ABI: first arg in rdi */
    regs.cs  = SEL_USER_CODE;
    regs.ss  = SEL_USER_DATA;
    regs.rfl = 0x200; // IF = 1
}

// only used by syscall create thread (EC+SC)
Ec::Ec(mword rip, mword rsp)
{
    cont     = ret_user_iret;
    regs.cs  = SEL_USER_CODE;
    regs.ss  = SEL_USER_DATA;
    regs.rfl = 0x200; // IF = 1
    regs.rip = rip;
    regs.rsp = rsp;
}

void Ec::ret_user_sysexit()
{
    asm volatile("lea %0," EXPAND(PREG(sp); LOAD_GPR RET_USER_SYSC)
                 :
                 : "m"(current->regs)
                 : "memory");

    UNREACHED;
}

void Ec::ret_user_iret()
{
    asm volatile("lea %0," EXPAND(PREG(sp); LOAD_GPR DROP_EXC RET_USER_EXC)
                 :
                 : "m"(current->regs)
                 : "memory");

    UNREACHED;
}

void Ec::root_invoke()
{
    FAIL;
}

void Ec::handle_tss()
{
    panic("Task gate invoked\n");
}

void Ec::syscall_handler(uint8 n)
{
    printf("syscall %d\n", n);

    ret_user_sysexit();

    UNREACHED;
}

bool Ec::handle_exc_ts(Exc_regs *r)
{
    if (r->user())
        return false;

    // SYSCALL with RFLAGS.NT=1 and IRET faulted
    r->rfl &= ~0x4000ul; // nested task flag

    return true;
}

void Ec::handle_exc(Exc_regs *r)
{
    if (r->vec == Cpu::EXC_TS && handle_exc_ts(r))
        return;

    if (r->vec == Cpu::EXC_GP) {
        panic("%s GP (RIP=%#lx CR2=%#lx)\n",
              r->rip < LINK_ADDR ? "User" : "Kernel",
              r->rip,
              r->cr2);
    }
    if (r->vec == Cpu::EXC_PF) {
        panic("%s PF (RIP=%#lx CR2=%#lx)\n",
              r->rip < LINK_ADDR ? "User" : "Kernel",
              r->rip,
              r->cr2);
    }

    panic("%s EXC %#lx (RIP=%#lx CR2=%#lx)\n",
          r->rip < LINK_ADDR ? "User" : "Kernel",
          r->vec,
          r->rip,
          r->cr2);

    UNREACHED;
}
