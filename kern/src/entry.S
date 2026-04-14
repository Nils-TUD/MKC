/*
 * Entry Functions
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

#include "memory.h"
#include "selectors.h"
#include "vectors.h"

.data
.global                 handlers
handlers:

.text

/*
 * Handler table entry: quad (64-bit) address of each handler
 */
.macro                  INTRGATE DPL
                        .align  4, 0x90
1:                      .data
                        .quad   1b + \DPL
                        .previous
.endm

.macro                  TASKGATE
                        .data
                        .quad   0
                        .previous
.endm

/*
 * Save/restore all 16 GPRs.
 * Push order: rax first (highest address in frame), r15 last (lowest address).
 * This matches Sys_regs layout: r15 at offset 0x00, rax at offset 0x78.
 */
.macro                  SAVE_GPR
                        push    %rax
                        push    %rcx
                        push    %rdx
                        push    %rbx
                        push    %rsp
                        push    %rbp
                        push    %rsi
                        push    %rdi
                        push    %r8
                        push    %r9
                        push    %r10
                        push    %r11
                        push    %r12
                        push    %r13
                        push    %r14
                        push    %r15
.endm

.macro                  LOAD_GPR
                        pop     %r15
                        pop     %r14
                        pop     %r13
                        pop     %r12
                        pop     %r11
                        pop     %r10
                        pop     %r9
                        pop     %r8
                        pop     %rdi
                        pop     %rsi
                        pop     %rbp
                        add     $8, %rsp        /* skip cr2 slot */
                        pop     %rbx
                        pop     %rdx
                        pop     %rcx
                        pop     %rax
.endm

/*
 * Reserve space for segment registers (gs, fs, es, ds) - 4 * 8 bytes
 */
.macro                  SAVE_SEG
                        sub     $0x20, %rsp
.endm

/*
 * Full exception state save. After SAVE_STATE:
 *   rbx = pointer to saved Sys_regs (bottom of GPR area)
 *   If from user: switches to kernel stack, reloads ds/es
 */
.macro                  SAVE_STATE
                        SAVE_SEG
                        SAVE_GPR
                        mov     %rsp, %rbx
                        cmp     $KSTCK_ADDR, %rsp
                        jae     1f
                        cld
                        mov     $(KSTCK_ADDR + PAGE_SIZE), %rsp
                        mov     $SEL_USER_DATA, %rax
                        mov     %rax, %ds
                        mov     %rax, %es
1:
.endm

.macro                  EXCEPTION VEC, DPL
INTRGATE                \DPL
                        push    $\VEC
                        jmp     entry_exc
.endm

.macro                  EXCEPTION_ERROR VEC, DPL
INTRGATE                \DPL
                        push    (%rsp)
                        movq    $\VEC, 8(%rsp)
                        jmp     entry_exc_error
.endm

/*
 * Exception Entries
 *
 * OFS_CR2 = 0x58 (r15=0,r14=8,r13=10h,r12=18h,r11=20h,r10=28h,r9=30h,r8=38h,
 *                  rdi=40h,rsi=48h,rbp=50h,cr2=58h)
 */
entry_exc:              push    $0
entry_exc_error:        SAVE_STATE
                        mov     %cr2, %rax
                        mov     %rax, 0x58(%rbx)        /* OFS_CR2 = 0x58 */
                        mov     %rbx, %rdi              /* arg1 = Exc_regs* (SysV ABI) */
                        call    exc_handler
                        jmp     ret_from_interrupt

EXCEPTION               0x0,    0       /* #DE Divide Error */
EXCEPTION               0x1,    0       /* #DB Debug */
TASKGATE                                /* #NMI placeholder */
EXCEPTION               0x3,    3       /* #BP Breakpoint */
EXCEPTION               0x4,    3       /* #OF Overflow */
EXCEPTION               0x5,    0       /* #BR Bound Range */
EXCEPTION               0x6,    0       /* #UD Undefined Opcode */
EXCEPTION               0x7,    0       /* #NM No Math Coprocessor */
TASKGATE                                /* #DF Double Fault (IST-based) */
TASKGATE                                /* Coprocessor Segment Overrun (reserved) */
EXCEPTION_ERROR         0xa,    0       /* #TS Invalid TSS */
EXCEPTION_ERROR         0xb,    0       /* #NP Segment Not Present */
EXCEPTION_ERROR         0xc,    0       /* #SS Stack Segment Fault */
EXCEPTION_ERROR         0xd,    0       /* #GP General Protection */
EXCEPTION_ERROR         0xe,    0       /* #PF Page Fault */
TASKGATE                                /* Reserved */
EXCEPTION               0x10,   0       /* #MF x87 FP Error */
EXCEPTION_ERROR         0x11,   0       /* #AC Alignment Check */
EXCEPTION               0x12,   0       /* #MC Machine Check */
EXCEPTION               0x13,   0       /* #XM SIMD FP Exception */
.rept                   NUM_EXC - 0x14
TASKGATE
.endr

/*
 * GSI Entries
 *
 * OFS_VEC = 0xa8: 16*8 (GPRs) + 4*8 (segs) + 8 (err) = 0xa8
 */
.set                    VEC, NUM_EXC
.rept                   NUM_GSI
INTRGATE                0
                        push    $VEC
                        jmp     entry_gsi
.set                    VEC, VEC + 1
.endr

entry_gsi:              push    $0
                        SAVE_STATE
                        mov     0xa8(%rbx), %rdi        /* OFS_VEC = 0xa8, arg via rdi */
                        call    gsi_vector
                        jmp     ret_from_interrupt

/*
 * LVT Entries
 */
.rept                   NUM_LVT
INTRGATE                0
                        push    $VEC
                        jmp     entry_lvt
.set                    VEC, VEC + 1
.endr

entry_lvt:              push    $0
                        SAVE_STATE
                        mov     0xa8(%rbx), %rdi        /* OFS_VEC = 0xa8, arg via rdi */
                        call    lvt_vector
                        jmp     ret_from_interrupt


/*
 * Interrupt Return
 *
 * OFS_CS = 0xb8: OFS_VEC + 8 (vec) + 8 (rip) = 0xa8 + 0x10 = 0xb8
 */
ret_from_interrupt:     testb   $3, 0xb8(%rbx)          /* test CPL bits in saved CS */
                        jnz     ret_user_iret
                        LOAD_GPR
                        add     $0x30, %rsp             /* skip gs,fs,es,ds,err,vec (6*8=0x30) */
                        iretq

/*
 * System-Call Entry (SYSCALL mechanism)
 *
 * On SYSCALL entry:
 *   RCX = user RIP (return address)
 *   R11 = user RFLAGS (saved by CPU)
 *   RSP = user RSP (unchanged!)
 *
 * LOAD_KSP: save user RSP -> r11, load kernel RSP from TSS.sp0
 * (r11 thus holds user RSP; the NOVA ABI uses r11 for user SP, rcx for user IP)
 */
.align                  4, 0x90
.globl                  entry_sysenter
entry_sysenter:
                        mov     %rsp, %r11              /* save user RSP in r11 */
                        mov     tss_run+4, %rsp         /* load kernel RSP from TSS.sp0 */
                        lea     -11*8(%rsp), %rsp       /* reserve space for seg+err+vec+iret */
                        SAVE_GPR                        /* push all 16 GPRs */
                        mov     $(KSTCK_ADDR + PAGE_SIZE), %rsp
                        and     $0xf, %rdi              /* syscall number in low nibble of rdi */
                        jmp     syscall_handler
