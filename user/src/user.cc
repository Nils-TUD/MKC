const unsigned long   UTCBS_START   = 0x10000000;
static unsigned long *UTCB_SENDER   = reinterpret_cast<unsigned long *>(UTCBS_START);
static unsigned long *UTCB_RECEIVER = reinterpret_cast<unsigned long *>(UTCBS_START + 4096);

unsigned long syscall1(unsigned long w0)
{
    asm volatile("syscall" : "+D"(w0) : : "rcx", "r11", "memory");
    return w0;
}

unsigned long syscall2(unsigned long w0, unsigned long w1)
{
    asm volatile("syscall" : "+D"(w0) : "S"(w1) : "rcx", "r11", "memory");
    return w0;
}

unsigned long syscall3(unsigned long w0, unsigned long w1, unsigned long w2)
{
    asm volatile("syscall" : "+D"(w0) : "S"(w1), "d"(w2) : "rcx", "r11", "memory");
    return w0;
}

unsigned long syscall4(unsigned long w0, unsigned long w1, unsigned long w2, unsigned long w3)
{
    asm volatile("syscall" : "+D"(w0) : "S"(w1), "d"(w2), "a"(w3) : "rcx", "r11", "memory");
    return w0;
}

void sys_create_ec(void (*eip)(), void *esp, unsigned long *utcb)
{
    syscall4(1,
             reinterpret_cast<unsigned long>(eip),
             reinterpret_cast<unsigned long>(esp),
             reinterpret_cast<unsigned long>(utcb));
}

void sys_yield()
{
    syscall1(2);
}

[[noreturn]]
void thread()
{
    while (1)
        sys_yield();
}

extern "C" [[noreturn]]
void main_func()
{
    char stack[128];

    sys_create_ec(thread, stack + 64 * 1, UTCB_SENDER);
    sys_create_ec(thread, stack + 64 * 2, UTCB_RECEIVER);

    while (1)
        sys_yield();
}
