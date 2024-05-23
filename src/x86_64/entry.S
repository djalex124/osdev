#include <multiboot.h>

.code64
.text

.global _start64
_start64:
    movabsq $entry64, %rax
    callq *%rax

entry64:
    movq $0x1000, %rax
    movq $0x0, (%rax)
    invlpg (%rax) // clear out identity mapping

.extern kmain
    call kmain

    hlt