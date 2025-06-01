.code64
.text

.global _start64
_start64:
    cli
    mov %rsi, %cr3
    movabsq $stack_top, %rsp
    movabsq $entry64, %rax
    callq *%rax

entry64: // jump to correct RIP
.extern kmain
    xor %rbp, %rbp
    call kmain

    hlt

.bss
.skip 0x1000
stack_top: