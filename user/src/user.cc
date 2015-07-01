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

void sys_create_ec(void (*eip)(), void *esp)
{
    syscall3(1, reinterpret_cast<unsigned long>(eip), reinterpret_cast<unsigned long>(esp));
}

unsigned long sys_yield()
{
    return syscall1(2);
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
    char stack[512];

    for (int i = 1; i <= 8; i++) {
        sys_create_ec(thread, stack + i * 64);
        sys_yield();
    }

    while (1)
        sys_yield();
}
