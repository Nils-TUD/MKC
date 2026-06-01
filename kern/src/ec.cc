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

#include "assert.h"
#include "ec.h"
#include "arch.h"
#include "cpu.h"
#include "kalloc.h"
#include "ptab.h"
#include "multiboot.h"
#include "elf.h"
#include "pt.h"
#include "bits.h"

Ec *Ec::current = 0;

// solely used for root_invoke()
Ec::Ec(void (*f)(), mword mbi) : cont(f)
{
    regs.rdi   = mbi; /* SysV ABI: first arg in rdi */
    regs.cs    = SEL_USER_CODE;
    regs.ss    = SEL_USER_DATA;
    regs.rfl   = 0x200; // IF = 1
    utcb       = nullptr;
    utcb_vaddr = 0;

    enqueue();
}

// only used by syscall create thread (EC+SC)
Ec::Ec(mword rip, mword rsp, mword utcb_addr)
{
    cont     = ret_user_iret;
    regs.cs  = SEL_USER_CODE;
    regs.ss  = SEL_USER_DATA;
    regs.rfl = 0x200; // IF = 1
    regs.rip = rip;
    regs.rsp = rsp;

    alloc_utcb(utcb_addr);
    enqueue();
}

void Ec::alloc_utcb(mword addr)
{
    utcb       = Kalloc::allocator.alloc_page(1, Kalloc::FILL_0);
    utcb_vaddr = addr;
    Ptab::insert_mapping(addr, Kalloc::virt2phys(utcb), 0x7);
}

void Ec::enqueue()
{
    if (!current) {
        next = prev = this;
    }
    else {
        next       = current;
        prev       = current->prev;
        next->prev = prev->next = this;
    }
}

Ec *Ec::find_by_utcb(mword utcb)
{
    Ec *ec = current->next;
    while (ec->utcb_vaddr != utcb) {
        if (ec == current)
            return nullptr;
        ec = ec->next;
    }
    return ec;
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
    Multiboot *mbi = static_cast<Multiboot *>(Ptab::remap(current->regs.rdi));

    if (!(mbi->flags & Multiboot::MODULES) || (mbi->mods_count != 1))
        panic("exactly ONE multi boot module is required.\n");

    Multiboot_module mod = *static_cast<Multiboot_module *>(Ptab::remap(mbi->mods_addr));

    printf("load module from %x - %x (%u bytes) : ",
           mod.mod_start,
           mod.mod_end,
           mod.mod_end - mod.mod_start);
    char *cmd = static_cast<char *>(Ptab::remap(mod.cmdline));
    printf("%s\n", cmd);

    // remap elf header
    Eh *e = static_cast<Eh *>(Ptab::remap(mod.mod_start));
    if (e->ei_magic != 0x464c457f || e->ei_data != 1 || e->type != 2)
        panic("No ELF\n");

    unsigned count    = e->ph_count;
    current->regs.rip = e->entry;

    // remap program headers
    Ph *p = static_cast<Ph *>(Ptab::remap(mod.mod_start + e->ph_offset));

    for (; count--; p++) {
        if (p->type == Ph::PT_LOAD) {
            unsigned attr = p->flags & Ph::PF_W ? 7 : 5;

            if (p->f_size != p->m_size || p->v_addr % PAGE_SIZE != p->f_offs % PAGE_SIZE)
                panic("Bad ELF\n");

            mword phys = align_dn(p->f_offs + mod.mod_start, PAGE_SIZE);
            mword virt = align_dn(p->v_addr, PAGE_SIZE);
            mword size = align_up(p->f_size, PAGE_SIZE);

            while (size) {
                Ptab::insert_mapping(virt, phys, attr);
                virt += PAGE_SIZE;
                phys += PAGE_SIZE;
                size -= PAGE_SIZE;
            }
        }
    }

    printf("iret to user ...\n");
    ret_user_iret();

    FAIL;
}

void Ec::handle_tss()
{
    panic("Task gate invoked\n");
}

void Ec::syscall_handler(uint8 n)
{
    switch (n) {
    case 0:
        sys_dump();
        break;

    case 1:
        sys_create_ec();
        break;

    case 2:
        sys_yield();
        break;

    case 3:
        sys_create_pt();
        break;

    default:
        printf("syscall %d - unknown\n", n);
        break;
    }

    ret_user_sysexit();

    UNREACHED;
}

void Ec::sys_dump()
{
    printf("EC:%p SYS_DUMP : %#lx, %#lx\n",
           current,
           current->sys_regs()->rsi,
           current->sys_regs()->rdx);
}

void Ec::sys_create_ec()
{
    mword rip       = current->sys_regs()->rsi;
    mword rsp       = current->sys_regs()->rdx;
    mword utcb_addr = current->sys_regs()->rax;

    assert(utcb_addr >= PAGE_SIZE);
    assert(utcb_addr + PAGE_SIZE <= USER_ADDR);
    assert((utcb_addr & PAGE_MASK) == 0);
    assert(find_by_utcb(utcb_addr) == nullptr);

    Ec *ec = new Ec(rip, rsp, utcb_addr);

    printf("EC:%p SYS_CREATE_EC EC:%p (RIP=%#lx RSP=%#lx UTCB=%#lx)\n",
           current,
           ec,
           rip,
           rsp,
           utcb_addr);
}

void Ec::sys_create_pt()
{
    mword id        = current->sys_regs()->rsi;
    mword rip       = current->sys_regs()->rdx;
    mword recv_utcb = current->sys_regs()->rax;
    assert(Pt::find_by_id(id) == nullptr);

    Ec *recv = find_by_utcb(recv_utcb);
    assert(recv != nullptr);

    auto pt = new Pt(id, rip, recv);

    printf("EC:%p SYS_CREATE_PT PT:%p (ID=%u RIP=%#lx RECV=%p)\n",
           current,
           pt,
           pt->id,
           pt->rip,
           pt->recv);
}

void Ec::sys_yield()
{
    printf("EC:%p SYS_YIELD to EC:%p\n", current, current->next);

    current->cont = ret_user_sysexit;
    current->next->make_current();
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
