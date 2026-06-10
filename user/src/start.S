.text
.global __start
__start:
    mov     $STACK_TOP, %rsp
    call    main_func
    ud2

.global reply_tramp
reply_tramp:
    sub     $8, %rsp
    mov     $5, %rdi
    syscall
    ud2
