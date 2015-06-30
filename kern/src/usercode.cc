#include "compiler.h"

/*
 * x86-64 SYSCALL protocol:
 *   CPU saves: RIP -> RCX, RFLAGS -> R11 (then masks RFLAGS with IA32_FMASK)
 *   CPU loads: CS/SS from STAR MSR, RIP from LSTAR MSR
 *   After syscall returns (sysretq): RIP <- RCX, RFLAGS <- R11
 *
 * The kernel entry (entry_sysenter in entry.S) reads the syscall number from
 * the low 8 bits of RDI and the arguments from RSI/RDI.
 */

[[gnu::section(".user")]]
static unsigned long syscall1(unsigned long w0)
{
    asm volatile("syscall" : "+D"(w0) : : "rcx", "r11", "memory");
    return w0;
}

[[gnu::section(".user")]]
static unsigned long syscall3(unsigned long w0, unsigned long w1, unsigned long w2)
{
    asm volatile("syscall" : "+D"(w0) : "S"(w1), "d"(w2) : "rcx", "r11", "memory");
    return w0;
}

[[gnu::section(".user")]]
static unsigned long sys_nop()
{
    return syscall1(0);
}

[[gnu::section(".user")]]
static unsigned long sys_add(unsigned long a, unsigned long b)
{
    return syscall3(1, a, b);
}

extern "C" [[noreturn]] [[gnu::section(".user")]]
void usercode()
{
    sys_nop();
    sys_add(2, 3);

    while (1)
        ;
}
