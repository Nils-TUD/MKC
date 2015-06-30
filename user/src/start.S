.text
.global __start
__start:
    mov     $STACK_TOP, %rsp
    call    main_func
    ud2
