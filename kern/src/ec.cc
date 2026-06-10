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
#include "pd.h"
#include "pt.h"
#include "string.h"
#include "bits.h"

Ec *Ec::current = 0;

// solely used for root_invoke()
Ec::Ec(Pd *own, void (*f)(), mword mbi) : Kobject(EC, own), cont(f), pd(own)
{
    regs.rdi   = mbi; /* SysV ABI: first arg in rdi */
    regs.cs    = SEL_USER_CODE;
    regs.ss    = SEL_USER_DATA;
    regs.rfl   = 0x200; // IF = 1
    utcb       = nullptr;
    utcb_vaddr = 0;
    state      = READY;

    enqueue();
}

// only used by syscall create thread (EC+SC)
Ec::Ec(Pd *own, mword sel, Pd *p, mword rip, mword rsp, mword utcb_addr)
    : Kobject(EC, own, sel), pd(p)
{
    cont     = ret_user_iret;
    regs.cs  = SEL_USER_CODE;
    regs.ss  = SEL_USER_DATA;
    regs.rfl = 0x200; // IF = 1
    regs.rip = rip;
    if (rip == 0)
        regs.r11 = rsp;
    else
        regs.rsp = rsp;
    state  = rip == 0 ? WAITING : READY;
    caller = nullptr;

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

void Ec::schedule()
{
    Ec *n = current->next;
    while (n->state != READY) {
        if (n == current)
            panic("No runnable Ec found");
        n = n->next;
    }
    n->make_current();
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

    case 4:
        sys_call();
        break;

    case 5:
        sys_reply();
        break;

    case 6:
        sys_create_pd();
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
    mword ec_sel    = current->sys_regs()->rbx;
    mword pd_sel    = current->sys_regs()->r8;

    assert(utcb_addr >= PAGE_SIZE);
    assert(utcb_addr + PAGE_SIZE <= USER_ADDR);
    assert((utcb_addr & PAGE_MASK) == 0);

    Capability cap = current->pd->lookup(pd_sel);
    assert(cap.ptr && cap.ptr->type() == Kobject::PD);
    Pd *pd   = static_cast<Pd *>(cap.ptr);

    auto ec  = new Ec(current->pd, ec_sel, pd, rip, rsp, utcb_addr);
    bool res = current->pd->insert_root(ec);
    assert(res);

    printf("EC:%p SYS_CREATE_EC EC:%#lx (RIP=%#lx RSP=%#lx UTCB=%#lx, PTR=%p)\n",
           current,
           ec_sel,
           rip,
           rsp,
           utcb_addr,
           ec);
}

void Ec::sys_create_pt()
{
    mword pt_sel      = current->sys_regs()->rsi;
    mword rip         = current->sys_regs()->rdx;
    mword ec_sel      = current->sys_regs()->rax;

    Capability ec_cap = current->pd->lookup(ec_sel);
    assert(ec_cap.ptr && ec_cap.ptr->type() == Kobject::EC);
    Ec *recv = static_cast<Ec *>(ec_cap.ptr);
    assert(recv->utcb != nullptr);

    auto pt  = new Pt(current->pd, pt_sel, rip, recv);
    bool res = current->pd->insert_root(pt);
    assert(res);

    printf("EC:%p SYS_CREATE_PT PT:%#lx (RIP=%#lx RECV=%p)\n", current, pt_sel, pt->rip, pt->recv);
}

void Ec::sys_create_pd()
{
    mword pd_sel = current->sys_regs()->rsi;

    auto pd      = new Pd(current->pd, pd_sel);
    bool res     = current->pd->insert_root(pd);
    assert(res);

    printf("EC:%p SYS_CREATE_PD PD:%#lx\n", current, pd_sel);
}

void Ec::sys_yield()
{
    printf("EC:%p SYS_YIELD\n", current);

    current->cont = ret_user_sysexit;
    schedule();
}

void Ec::sys_call()
{
    mword pt_sel = current->sys_regs()->rsi;

    assert(current->utcb != nullptr);

    printf("EC:%p SYS_CALL (PT=%#lx)\n", current, pt_sel);

    Capability pt_cap = current->pd->lookup(pt_sel);
    assert(pt_cap.ptr && pt_cap.ptr->type() == Kobject::PT);
    Pt *pt   = static_cast<Pt *>(pt_cap.ptr);
    Ec *recv = pt->recv;

    if (recv->state == WAITING) {
        current->cont  = ret_user_sysexit;
        current->state = BLOCKED;
        recv->cont     = recv_user;
        recv->caller   = current;
        recv->state    = READY;
        recv->sys_regs()->set_ip(pt->rip);
        recv->make_current();
    }

    current->cont = sys_call;
    schedule();
}

void Ec::sys_reply()
{
    assert(current->utcb != nullptr);
    assert(current->caller != nullptr);

    printf("EC:%p SYS_REPLY\n", current);

    Ec *caller      = current->caller;

    current->caller = nullptr;
    current->state  = WAITING;
    memcpy(caller->utcb, current->utcb, PAGE_SIZE);

    caller->state = READY;
    caller->make_current();
}

void Ec::recv_user()
{
    memcpy(current->utcb, current->caller->utcb, PAGE_SIZE);
    ret_user_sysexit();
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
