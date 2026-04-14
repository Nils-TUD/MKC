/*
 * Selectors
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

#define SEL_NULL_DESC 0x0
#define SEL_KERN_CODE 0x8
#define SEL_KERN_DATA 0x10
/* SYSRET derives the user selectors from IA32_STAR. */
#define SEL_USER_DATA 0x1b
#define SEL_USER_CODE 0x23
#define SEL_TSS_RUN   0x28
/* No SEL_TSS_DBF: double-fault uses IST in the run TSS */
#define SEL_MAX 0x38
