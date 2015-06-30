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

// example syscall for yielding
unsigned long sys_yield()
{
    return syscall1(0xdeadbeaf /* TODO use correct syscall number*/);
}

extern "C" [[noreturn]]
void main_func()
{
    while (1)
        ;
}
