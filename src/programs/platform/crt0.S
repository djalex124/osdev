.code64
.text

.global _start
_start:
    movq $0, %rbp
    pushq %rbp
    pushq %rbp
    movq %rsp, %rbp

    pushq %rsi
    pushq %rdi

    # eventually init c library
    # call init_c_lib

    call _init

    popq %rdi
    popq %rsi

    call main
    
    movq %rax, %rdi
    
    # also add an exit function to kill process
    # call exit

.size _start, . - _start