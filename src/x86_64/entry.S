#include <multiboot.h>

.code64
.text

.global _start64
_start64:
    movabsq $entry64, %rax
    callq *%rax

entry64: // jump to correct RIP
.extern kmain
    call kmain

    hlt