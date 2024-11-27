#include <multiboot.h>
#include <mem.h>

.code64
.text

.global _start64
_start64:
    movabsq $entry64, %rax
    callq *%rax

entry64: // jump to correct RIP
    movabsq $stack_top, %rsp
.extern kmain
    xor %rbp, %rbp
    call kmain

    hlt