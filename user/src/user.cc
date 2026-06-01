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

void sys_dump(unsigned long w0, unsigned long w1)
{
    asm volatile("" ::"S"(w0), "d"(w1) : "memory");
    syscall1(0);
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

void sys_create_pt(int id, void (*eip)(), unsigned long *utcb)
{
    syscall4(3, id, reinterpret_cast<unsigned long>(eip), reinterpret_cast<unsigned long>(utcb));
}

void sys_call(unsigned long recv_utcb)
{
    syscall2(4, recv_utcb);
}

[[noreturn]]
void sys_reply()
{
    syscall1(5);
    __builtin_unreachable();
}

[[noreturn]]
void sender()
{
    unsigned long *words = UTCB_SENDER;
    words[0]             = 1;
    words[1]             = 2;
    while (1) {
        sys_call(0);
        sys_dump(words[0], words[1]);
    }
}

[[noreturn]]
void portal()
{
    unsigned long *words  = UTCB_RECEIVER;
    words[0]             += words[1];
    sys_reply();
}

extern "C" [[noreturn]]
void main_func()
{
    char stack[128];

    // sender therad
    sys_create_ec(sender, stack + 64 * 1, UTCB_SENDER);
    // receiver thread
    sys_create_ec(0, stack + 64 * 2, UTCB_RECEIVER);
    sys_create_pt(0, portal, UTCB_RECEIVER);

    while (1)
        sys_yield();
}
