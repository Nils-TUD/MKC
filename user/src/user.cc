const unsigned long STACK_SIZE      = 64;
const unsigned long STACK_WORDS     = STACK_SIZE / sizeof(unsigned long);

const unsigned long   UTCBS_START   = 0x10000000;
static unsigned long *UTCB_SENDER   = reinterpret_cast<unsigned long *>(UTCBS_START);
static unsigned long *UTCB_RECEIVER = reinterpret_cast<unsigned long *>(UTCBS_START + 4096);

const unsigned long  ROOT_PD_SEL    = 0;
static unsigned long next_sel       = 1;

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

unsigned long syscall4(unsigned long w0, unsigned long w1, unsigned long w2, unsigned long w3)
{
    asm volatile("syscall" : "+D"(w0) : "S"(w1), "d"(w2), "a"(w3) : "rcx", "r11", "memory");
    return w0;
}

unsigned long syscall6(unsigned long w0,
                       unsigned long w1,
                       unsigned long w2,
                       unsigned long w3,
                       unsigned long w4,
                       unsigned long w5)
{
    register unsigned long r8 asm("r8") = w5;
    asm volatile("syscall"
                 : "+D"(w0)
                 : "S"(w1), "d"(w2), "a"(w3), "b"(w4), "r"(r8)
                 : "rcx", "r11", "memory");
    return w0;
}

void sys_dump(unsigned long w0, unsigned long w1)
{
    asm volatile("" ::"S"(w0), "d"(w1) : "memory");
    syscall1(0);
}

unsigned long sys_create_ec(unsigned long pd, void (*eip)(), void *esp, unsigned long *utcb)
{
    unsigned long sel = next_sel++;
    syscall6(1,
             reinterpret_cast<unsigned long>(eip),
             reinterpret_cast<unsigned long>(esp),
             reinterpret_cast<unsigned long>(utcb),
             sel,
             pd);
    return sel;
}

void sys_yield()
{
    syscall1(2);
}

unsigned long sys_create_pt(void (*eip)(), unsigned long ec)
{
    unsigned long sel = next_sel++;
    syscall4(3, sel, reinterpret_cast<unsigned long>(eip), ec);
    return sel;
}

void sys_call(int portal_id)
{
    syscall2(4, portal_id);
}

unsigned long sys_create_pd()
{
    unsigned long sel = next_sel++;
    syscall2(6, sel);
    return sel;
}

unsigned long pt_sel;

[[noreturn]]
void sender()
{
    unsigned long *words = UTCB_SENDER;
    words[0]             = 1;
    words[1]             = 2;
    while (1) {
        sys_call(pt_sel);
        sys_dump(words[0], words[1]);
    }
}

void portal()
{
    unsigned long *words  = UTCB_RECEIVER;
    words[0]             += words[1];
}

extern "C" void reply_tramp();

unsigned long *local_ec_stack(unsigned long *stack)
{
    stack[-1] = reinterpret_cast<unsigned long>(reply_tramp);
    return stack - 1;
}

extern "C" [[noreturn]]
void main_func()
{
    unsigned long stack[STACK_WORDS * 2];

    // sender thread in root PD
    sys_create_ec(ROOT_PD_SEL, sender, stack + STACK_WORDS * 1, UTCB_SENDER);

    unsigned long pd_sel = sys_create_pd();
    unsigned long ptec_sel =
        sys_create_ec(pd_sel, 0, local_ec_stack(stack + STACK_WORDS * 2), UTCB_RECEIVER);
    pt_sel = sys_create_pt(portal, ptec_sel);

    while (1)
        sys_yield();
}
