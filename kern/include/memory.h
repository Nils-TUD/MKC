/*
 * Virtual-Memory Layout
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

#define PAGE_BITS 12
#define PAGE_SIZE (1 << PAGE_BITS)
#define PAGE_MASK (PAGE_SIZE - 1)

#define LOAD_ADDR 0x200000

#define USER_ADDR 0x00007ffffffff000

#define LINK_ADDR 0xffffffff81000000

#define KSTCK_ADDR 0xffffffffbfffd000
#define LAPIC_ADDR 0xffffffffbfffe000

#define IOBMP_SADDR 0xffffffffc0000000
#define IOBMP_EADDR (IOBMP_SADDR + PAGE_SIZE * 2)
#define REMAP_SADDR 0xffffffffdf000000
