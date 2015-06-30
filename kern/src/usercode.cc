extern "C" [[gnu::regparm(1), gnu::section(".user"), noreturn]]
void usercode(unsigned)
{
    // TODO
    // - 1st : ud2 or force page fault here
    // - 2nd : reenter the kernel via sysenter
    // - 3rd : so some simple system calls, like adding two numbers

    while (1)
        ;
}
