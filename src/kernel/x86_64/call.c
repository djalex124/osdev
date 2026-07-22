#include <output/kterm.h>

#include <kernel/debug.h>

#include <x86_64/desc.h>
#include <x86_64/call.h>

DEFINE_SYSCALL1(add_one, 1, uint64_t);
DEFINE_SYSCALL0(test_print, 3);

kframe_int *kcall_handler(kframe_int *k)
{
    kdebug_outf("\nkdesc: syscall %d recieved", k->rax);
    uint64_t return_val = 0;

    // TODO:
    // - Check if pointers are valid for current process
    // - Save registers in case they change during call

    switch (k->rax)
    {
        case 1:
            kdebug_outf(" val %d", k->rdi);
            return_val = k->rdi + 1;
            break;
        case 3:
            kterm_putf("\ntest print :)");
            break;
    }

    k->rax = return_val;
    return k;
}

void kcall_init()
{
    kdesc_setinterruptfunc(32, &kcall_handler);
}