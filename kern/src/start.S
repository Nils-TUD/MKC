/*
 * Startup Code
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

/*
 * Initialization Code
 */
.section .init

.globl                  __start
.globl                  __serial_base

/*
 * Multiboot2 Header (CPU starts in 32-bit protected mode per spec)
 * QEMU -kernel supports Multiboot2 for 64-bit ELF.
 */
.align                  8
__boot_header:          .long   0xe85250d6          /* magic */
                        .long   0                   /* arch: i386 protected mode */
                        .long   __boot_header_end - __boot_header   /* header_length */
                        .long   -(0xe85250d6 + 0 + (__boot_header_end - __boot_header))  /* checksum */
                        /* end tag */
                        .word   0                   /* type */
                        .word   0                   /* flags */
                        .long   8                   /* size */
__boot_header_end:

/*
 * Boot GDT - 64-bit long-mode descriptors (L=1, D=0)
 */
.align                  8, 0x90
__boot_gdt:             .word   0
                        .word   __boot_gdt__ - __boot_gdt - 1
                        .long   __boot_gdt
                        .quad   0x00a09b0000000000      /* Kernel Code: L=1, D=0, DPL=0 */
                        .quad   0x00a0930000000000      /* Kernel Data: L=1, D=0, DPL=0 */
                        .quad   0x00a0fb0000000000      /* User Code:   L=1, D=0, DPL=3 */
                        .quad   0x00a0f30000000000      /* User Data:   L=1, D=0, DPL=3 */
__boot_gdt__:

.code32

__start:
                        /* GET SERIAL PORT BASE ADDRESS (runs in 32-bit mode) */
                        mov     0x400, %eax
                        cmp     $0, %ax
                        jne     1f
                        shr     $16, %eax
1:                      mov     %ax, __serial_base

                        /*
                         * SETUP 4-LEVEL BOOT PAGE TABLES
                         *
                         * Layout:
                         *   PML4 (LVL4)  -> PDPT_low  (LVL3L) -> PD_low  (LVL2L): 2MB identity map
                         *                -> PDPT_high (LVL3H) -> PD_high (LVL2H): 2MB kernel map
                         *
                         * BSS is zero-initialized so all upper 32-bit halves of entries are 0.
                         * We only need to write the low 32 bits of each 8-byte PTE.
                         */

                        /* PML4[idx for LOAD_ADDR] -> LVL3L (identity map) */
                        mov     $(LVL3L + 0x27), %ecx
                        mov     %ecx, LVL4 + 8 * ((LOAD_ADDR >> 39) & 0x1ff)

                        /* LVL3L[idx for LOAD_ADDR] -> LVL2L */
                        mov     $(LVL2L + 0x27), %ecx
                        mov     %ecx, LVL3L + 8 * ((LOAD_ADDR >> 30) & 0x1ff)

                        /* PML4[idx for LINK_ADDR] -> LVL3H (kernel high map) */
                        mov     $(LVL3H + 0x27), %ecx
                        mov     %ecx, LVL4 + 8 * ((LINK_ADDR >> 39) & 0x1ff)

                        /* LVL3H[idx for LINK_ADDR] -> LVL2H */
                        mov     $(LVL2H + 0x27), %ecx
                        mov     %ecx, LVL3H + 8 * ((LINK_ADDR >> 30) & 0x1ff)

                        /* Fill LVL2L with 2MB identity-map superpages starting at LOAD_ADDR */
                        mov     $0xe3, %ecx             /* Present+RW+User+PS+Accessed+Dirty */
                        mov     $LVL2L, %edi
1:                      mov     %ecx, (%edi)
                        add     $0x200000, %ecx         /* 2MB per entry */
                        add     $8, %edi                /* 8 bytes per PDE */
                        cmp     $LOAD_E, %ecx
                        jb      1b

                        /* Fill LVL2H with 2MB kernel-map superpages starting at LINK_P */
                        mov     $(LINK_P + 0x1e3), %ecx /* LINK_P + PS+Global+Present+RW+... */
                        mov     $(LVL2H + 8 * ((LINK_ADDR >> 21) & 0x1ff)), %edi
1:                      mov     %ecx, (%edi)
                        add     $0x200000, %ecx         /* 2MB per entry */
                        add     $8, %edi
                        cmp     $LINK_E, %ecx
                        jb      1b

                        /* ENABLE LONG MODE */

                        /* Set IA32_EFER.LME (bit 8) + SCE (bit 0, SYSCALL enable) */
                        mov     $0xc0000080, %ecx       /* IA32_EFER MSR */
                        rdmsr
                        or      $0x101, %eax
                        wrmsr

                        /* CR4: PAE (bit 5) required for long mode; keep PSE+PGE+OSFXSR+OSXMMEXCPT */
                        mov     $0x6b8, %edx            /* PAE+PSE+PGE+OSFXSR+OSXMMEXCPT */
                        mov     %edx, %cr4

                        /* Load PML4 physical address into CR3 */
                        mov     $PDBR, %edx
                        mov     %edx, %cr3

                        /* Enable paging (also activates long mode since LME is set) */
                        mov     $0x8001002b, %edx       /* PG+WP+NE+ET+MP+PE */
                        mov     %edx, %cr0

                        /* Far jump to 64-bit code segment -> CPU transitions to long mode */
                        lgdt    __boot_gdt + 2
                        ljmp    $SEL_KERN_CODE, $__start_paged

.code64
__start_paged:
                        /* LOAD SEGMENTS in 64-bit mode */
                        mov     $SEL_KERN_DATA, %ecx
                        mov     %ecx, %ss
                        mov     $SEL_USER_DATA, %ecx
                        mov     %ecx, %ds
                        mov     %ecx, %es
                        mov     %ecx, %fs
                        mov     %ecx, %gs
                        mov     $STACK, %rsp

                        jmp     1f

.text

1:                      mov     %rbx, %rdi              /* 1st arg (SysV ABI): MBI pointer */
                        call    init
                        mov     %rbx, %rdi              /* pass MBI to bootstrap() via rdi */
                        mov     $(KSTCK_ADDR + PAGE_SIZE), %rsp
                        jmp     bootstrap

.section .initdata
__serial_base:          .long 0
